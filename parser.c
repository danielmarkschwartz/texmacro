#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "tex.h"

static void error(char *fmt, ...){
	va_list ap;
	va_start(ap, fmt);

	fprintf(stderr, "ERROR: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");

	exit(1);
}

//Prepend contents of given filename to char stream
//  Filename may be a full path, a file in the CWD, or a file in
//  the library path. Filename may optionally omit the ".tex" extension
void tex_input(struct tex_parser *p, char *filename){
	//TODO: implement
}

//Prepend contents of file stream to char stream
void tex_input_file(struct tex_parser *p, char *name, FILE *file){
	assert(p);
	assert(file);

	struct tex_char_stream *s = malloc(sizeof *s);
	if(!s) p->error("Could not allocate memory");

	*s = (struct tex_char_stream){TEX_FILE, .name=name, .file=file, .next=p->char_stream};

	p->char_stream = s;
}

//Prepend a buffer of characters to the character stream
void tex_input_buf(struct tex_parser *p, char *name, char *buf, size_t n) {
	assert(p);
	assert(buf);

	struct tex_char_stream *s = malloc(sizeof *s);
	if(!s) p->error("Could not allocate memory");

	void *mybuf = malloc(n);
	memcpy(mybuf, buf, n);

	*s = (struct tex_char_stream){TEX_BUF, .name=name, .buf.buf=mybuf, .buf.n=n, .next=p->char_stream};

	p->char_stream = s;
}

//Prepend string to top of character input
void tex_input_str(struct tex_parser *p, char *name, char *input){
	tex_input_buf(p, name, input, strlen(input));
}

//Prepend one token to start of token input
void tex_input_token(struct tex_parser *p, struct tex_token t) {
	p->token = tex_token_prepend(t, p->token);
}

//Input at most n tokens from token stream to start of token input
void tex_input_tokens(struct tex_parser *p, struct tex_token *ts, size_t n) {
	if(n == 0) return;

	size_t x = n;
	while(ts && ts->next && --x > 0)
		ts = ts->next;

	while(n-- > 0) {
		tex_input_token(p, *ts);
		ts = ts->prev;
	}
}

// Initializes parser with default settings
void tex_init_parser(struct tex_parser *p){
	assert(p);
	memset(p, 0, sizeof *p);

	p->block = malloc(sizeof *p->block);
	if(!p->block) p->error("Could not allocate memory");
	memset(p->block, 0, sizeof *p->block);

	//Set default character codes
	//Note: 0 -> other, 12 -> esc internally
	p->block->cat['{'] = TEX_BEGIN_GROUP;
	p->block->cat['}'] = TEX_END_GROUP;
	p->block->cat['$'] = TEX_MATH;
	p->block->cat['&'] = TEX_ALIGN;
	p->block->cat['\n'] = TEX_EOL;
	p->block->cat['#'] = TEX_PARAMETER;
	p->block->cat['^'] = TEX_SUPER;
	p->block->cat['_'] = TEX_SUB;
	p->block->cat['\0'] = TEX_INVALID;
	p->block->cat[' '] = TEX_SPACE;
	p->block->cat['\\'] = TEX_ESC;
	p->block->cat['~'] = TEX_ACTIVE;
	p->block->cat['%'] = TEX_COMMENT;
	p->block->cat[127] = TEX_INVALID;

	char c;
	for (c = 'A'; c <= 'Z'; c++) p->block->cat[(size_t)c] = TEX_LETTER;
	for (c = 'a'; c <= 'z'; c++) p->block->cat[(size_t)c] = TEX_LETTER;

	p->error = error;
}

void tex_block_enter(struct tex_parser *p) {
	struct tex_block *b = malloc(sizeof *b);
	if(!b) p->error("Could not allocate memory");
	memset(b, 0, sizeof *b);

	for(int i = 0; i < 127; i++)
		b->cat[i] = p->block->cat[i];

	b->parent = p->block;
	p->block = b;
}

void tex_block_exit(struct tex_parser *p) {
	assert(p && p->block);  //There should always be a block defined
	if(!p->block->parent)
		p->error("Extraneous group close");

	struct tex_block *b = p->block;
	p->block = b->parent;

	for(int i = 0; i < 9; i++)
		if(b->parameter[i])
			tex_token_free(b->parameter[i]);

	free(b);
}


void tex_define_macro_func(struct tex_parser *p, char *cs, void (*handler)(struct tex_parser*, struct tex_val)){
	assert(p);
	assert(p->block->vals_n < VAL_MAX);

	p->block->vals[p->block->vals_n++] = (struct tex_val){TEX_MACRO, (struct tex_token){TEX_ESC, .s=cs}, .handler=handler};
}

//Parses macro arguments from parser input based on the given arglist and writes the
//arguments to the supplied buffer, which must be large enough to accomidate all numbered
//arguments in the arglist
void tex_parse_arguments(struct tex_parser *p, struct tex_token *arglist) {
	assert(p && p->block);

	//Nothing to do for empty arglist
	if(!arglist) return;

	struct tex_token t;
	while(arglist && arglist->cat != TEX_PARAMETER){
		t = tex_read_token(p);
		if(t.cat == TEX_INVALID)
			p->error("Input ends while reading macro arguments");

		if(!tex_token_eq(*arglist, t))
			p->error("Macro usage does not match definition");

		arglist = arglist->next;
	}

	struct tex_token *bound_start = arglist;
	size_t i = 0, /*arg number*/
	       n = 0, /*bounding characters matched*/
	       s = 0; /*start of rewind*/
	while(arglist != NULL) {
		t = tex_read_token(p);
		if(t.cat == TEX_INVALID)
			p->error("Input ends while reading macro arguments");
		if(t.cat == TEX_PARAMETER)
			p->error("Parameter used outside of macro definition");

		if(arglist->cat == TEX_PARAMETER){
			i++;
			n = 0;
			s = 0;
			arglist = arglist->next;
			bound_start = arglist;
		}

		if(arglist && tex_token_eq(*arglist, t)){
			//Token matches bound, keep reading
			arglist = arglist->next;
			if(s == 0 && n > 0 && tex_token_eq(t, *bound_start))
				s = n;
			n++;
		} else {
			if(n == 0) {
				//Just append the token if no match so far
				p->block->parameter[i-1] = tex_token_append(p->block->parameter[i-1], t);
			}else{
				//Rewind to bound_start
				arglist = bound_start;

				//Copy s items (or all if no s) to the argument
				s = s?s:n;
				for(int t = s; t > 0; t--) {
					p->block->parameter[i-1] = tex_token_append(p->block->parameter[i-1], *arglist);
					arglist = arglist->next;
				}

				//Replay and uncopied items and this one
				tex_input_token(p, t);
				tex_input_tokens(p, arglist, n-s);

				//Start looking for bound again
				arglist = bound_start;
				n = 0;
				s = 0;
			}
		}
	}
}

//Returns an arglist parsed from the parser input. This is what would follow a \def, as in
//\def\cs<arglist>{<replacement>}
struct tex_token *tex_parse_arglist(struct tex_parser *p) {
	int pn = 1; //parameter number
	struct tex_token t, *ts = NULL;

	while((t = tex_read_token(p)).cat != TEX_BEGIN_GROUP) {
		if(t.cat == TEX_INVALID) break;
		if(t.cat == TEX_PARAMETER && t.c != pn++)
			p->error("Paramater numbers should increase sequentially");
		ts = tex_token_append(ts, t);
	}

	tex_input_token(p, t);

	return ts;
}

//Reads one balanced block of tokens, or NULL if next token is not a TEX_BEGIN_GROUP
struct tex_token *tex_read_block(struct tex_parser *p) {
	struct tex_token t = tex_read_token(p);
	if(t.cat != TEX_BEGIN_GROUP) return NULL;

	struct tex_token *ts = NULL;

	while((t = tex_read_token(p)).cat != TEX_END_GROUP)
		ts = tex_token_append(ts, t);

	return ts;
}

//Handle a general purpose macro, such as those previously defined by \def
void tex_handle_macro_general(struct tex_parser* p, struct tex_val m){
	tex_block_enter(p);
	p->block->macro = m.cs;

	tex_parse_arguments(p, m.arglist);

	//TODO: close out the block correctly
	tex_input_token(p, (struct tex_token){TEX_END_GROUP, .c='}'});
	p->token = tex_token_join(m.replacement, p->token);
}

void tex_handle_macro_par(struct tex_parser* p, struct tex_val m){
	assert(p);
	p->token = tex_token_prepend((struct tex_token){TEX_OTHER, .c='\n'}, p->token);
	p->token = tex_token_prepend((struct tex_token){TEX_OTHER, .c='\n'}, p->token);
}

//Handle \def macros
void tex_handle_macro_def(struct tex_parser* p, struct tex_val m){
	struct tex_token cs = tex_read_token(p);
	assert(cs.cat == TEX_ESC);

	struct tex_token *arglist = tex_parse_arglist(p);
	struct tex_token *replacement = tex_read_block(p);
	assert(replacement);

	tex_define_macro_tokens(p, cs.s, arglist, replacement);
}


void tex_define_macro_tokens(struct tex_parser *p, char *cs, struct tex_token *arglist, struct tex_token *replacement) {
	assert(p);
	assert(p->block->vals_n < VAL_MAX);

	p->block->vals[p->block->vals_n++] = (struct tex_val){TEX_MACRO, (struct tex_token){TEX_ESC, .s=cs}, arglist, replacement, tex_handle_macro_general};
}

char *tex_read_control_sequence(struct tex_parser *p) {
	assert(p);

	struct tex_token tok = tex_read_char(p);
	char *cs = NULL;

	if(tok.cat == TEX_EOL) {
		cs = strdup("");
		assert(cs);
		p->state = TEX_MIDLINE;
	} else if (tok.cat != TEX_LETTER) {
		cs = malloc(2*sizeof(char));
		assert(cs);
		cs[0] = tok.c;
		cs[1] = 0;
		p->state = TEX_SKIPSPACE;
	} else { // Must be a TEX_LETTER
		char buf[CS_MAX+1];
		size_t n = 0;

		do {
			assert(n < CS_MAX);
			buf[n++] = tok.c;
		} while ((tok = tex_read_char(p)).cat == TEX_LETTER);

		tex_unread_char(p);

		buf[n++] = 0;
		cs = strdup(buf);

		p->state = TEX_SKIPSPACE;
	}

	return cs;
}

//Unread the last character
void tex_unread_char(struct tex_parser *p) {
	assert(p);

	//Having no stream means we are unreading an EOF
	//Since we will always read EOF, there is nothing to do
	if(!p->char_stream) return;

	assert(p->char_stream->type == TEX_BUF || p->char_stream->type == TEX_FILE);
	assert(p->char_stream->buf.i > 0);

	if(p->char_stream->type == TEX_BUF)
		p->char_stream->buf.i--;
	else //TEX_FILE
		ungetc(p->char_stream->last, p->char_stream->file);

	//NOTE: this doesn't set the correct column or line
}

struct tex_token tex_read_char(struct tex_parser *p) {
	assert(p);

	struct tex_char_stream *s;
	char c;

	for(;;){
		s = p->char_stream;

		//Empty streams return TEX_INVALID
		//TODO: should return EOF or something
		if(s == NULL)
			return (struct tex_token){TEX_INVALID};

		assert(s->type == TEX_BUF || s->type == TEX_FILE);

		//Try to read a new character
		if(s->type == TEX_BUF) {
			if(s->buf.i >= s->buf.n) {
				//TODO: free old stream
				p->char_stream = s->next;
				continue;
			}

			c = s->buf.buf[s->buf.i++];
			break;
		}

		int i = fgetc(s->file);
		if(i == EOF) {
			//TODO: free old stream
			p->char_stream = s->next;
			continue;
		}

		c = (char)i;
		break;
	}

	s->last = c;

	//Update file position
	if(c == '\n'){
		s->line++;
		s->col=0;
	}else
		s->col++;

	//Read character category
	char cat = p->block->cat[(size_t)c];
	assert(cat <= TEX_CAT_NUM);

	return (struct tex_token){cat, .c=c};
}

//Read the next token from the parser input
struct tex_token tex_read_token(struct tex_parser *p) {
	struct tex_token t;

	//Try to read a token from the token stream
	if(p->token) {
		t = *p->token;
		p->token = p->token->next;
		return t;
	}

	//Otherwise, try to parse at token from the character stream
	t = tex_read_char(p);

	assert(p->state == TEX_NEWLINE || p->state == TEX_SKIPSPACE || p->state == TEX_MIDLINE);

	switch(t.cat){

	case TEX_SPACE:
		if(p->state == TEX_NEWLINE || p->state == TEX_SKIPSPACE)
			return tex_read_token(p);

		p->state = TEX_SKIPSPACE;
		return (struct tex_token){TEX_OTHER, .c=' '};

	case TEX_EOL:
		switch(p->state){
		case TEX_NEWLINE:	t = (struct tex_token){TEX_ESC, .s=strdup("par")}; break; //Return \par
		case TEX_SKIPSPACE:	t = (struct tex_token){TEX_IGNORE}; break;  //Skip space
		case TEX_MIDLINE:	t = (struct tex_token){TEX_OTHER, .c=' '}; break; // Convert to space
		default: assert(p->state == TEX_NEWLINE || p->state == TEX_SKIPSPACE || p->state == TEX_MIDLINE);
		}

		p->state = TEX_NEWLINE;
		return t;

	case TEX_COMMENT:
		while(tex_read_char(p).cat != TEX_EOL);
		return tex_read_token(p);

	case TEX_PARAMETER:
		t = tex_read_char(p);
		if(t.cat == TEX_PARAMETER)
			return (struct tex_token){TEX_OTHER, .c=t.c};

		assert(isdigit(t.c));
		p->state = TEX_MIDLINE;
		return (struct tex_token){TEX_PARAMETER, .c=t.c-'0'};

	case TEX_ESC:
		t.s = tex_read_control_sequence(p);
		p->state = TEX_SKIPSPACE;
		break;

	case TEX_IGNORE:
		return tex_read_token(p);

	default:
		p->state = TEX_MIDLINE;
	}

	return t;
}

struct tex_val *tex_val_find(struct tex_parser *p, struct tex_token t) {
	struct tex_block *b = p->block;
	while(b) {
		size_t n = 0;
		while(n < b->vals_n)
			if(strcmp(b->vals[n].cs.s, t.s) == 0) break;
			else n++;

		if(n < b->vals_n)
			return &b->vals[n];

		b = b->parent;
	}
	return NULL;
}

void tex_macro_replace(struct tex_parser *p, struct tex_token t) {
	struct tex_val *m = tex_val_find(p, t);
	if(!m) p->error("Macro '\\%s' not found", t.s);

	assert(m->handler);
	m->handler(p, *m);
}

void tex_parameter_replace(struct tex_parser *p, struct tex_token t) {
	assert(p && p->block);
	assert(t.cat == TEX_PARAMETER);

	//TODO: this should actually look up the hierarchy for a macro block

	//ERROR: parameter found not in macro replacement
	assert(p->block->macro.cat == TEX_ESC);

	struct tex_token *para = p->block->parameter[(size_t)t.c-1];
	//ERROR: Undefined parameter
	assert(para);

	p->token = tex_token_join(para, p->token);
}

#define CHAR_MAX_LEN 12

int tex_read_num(struct tex_parser *p) {
	char s[CHAR_MAX_LEN+1], *end;
	size_t n = 0;
	struct tex_token t;

	while(n < CHAR_MAX_LEN) {
		t = tex_read_token(p);
		if(t.cat == TEX_ESC || t.c < '0' || t.c > '9')
			break;
		s[n++] = t.c;
	}

	tex_unread_char(p);

	if(n == 0)
		p->error("Expected an integer value, but no numbers have been found");


	s[n] = 0;
	int i = strtol(s, &end, 10);
	if(end == s)
		p->error("\"%s\"could not be parsed as integer", s);

	return i;
}

char *tex_read_filename(struct tex_parser *p) {
	char filename[FILENAME_MAX+1];
	struct tex_token t;
	size_t n = 0;

	while(n < FILENAME_MAX){
		t = tex_read_token(p);
		if((t.cat != TEX_LETTER && t.cat != TEX_OTHER)|| t.c == ' ')
			break;
		filename[n++] = t.c;
	}

	tex_input_token(p, t);
	filename[n] = 0;

	return strdup(filename);

}

//Write the next n characters to the output buffer, returns the number written.
int tex_read(struct tex_parser *p, char *buf, int n) {
	assert(n>=0);

	int i;
	for(i = 0; i < n; i++) {
		struct tex_token tok = tex_read_token(p);
		if(tok.cat == TEX_INVALID)
			break;

		switch(tok.cat) {
		case TEX_IGNORE: i--; continue;
		case TEX_ESC: tex_macro_replace(p, tok); i--; continue;
		case TEX_PARAMETER: tex_parameter_replace(p, tok); i--; continue;
		case TEX_BEGIN_GROUP: tex_block_enter(p); i--; continue;
		case TEX_END_GROUP: tex_block_exit(p); i--; continue;
		default: buf[i] = tok.c;
		}
	}

	return i;
}

void tex_free_parser(struct tex_parser *p){
	//TODO: actually free the parser
}

