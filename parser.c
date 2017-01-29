#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "tex.h"

//Prepend contents of given filename to char stream
//  Filename may be a full path, a file in the CWD, or a file in 
//  the library path. Filename may optionally omit the ".tex" extension
void tex_input(struct tex_parser *p, char *filename){
	//TODO: implement
}

//Prepend contents of file stream to char stream
void tex_input_file(struct tex_parser *p, FILE *file){
	//TODO: implement
}

//Prepend string to top of character input
void tex_input_str(struct tex_parser *p, char *input){
	assert(p);
	p->char_stream = tex_char_stream_str(input, p->char_stream);
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
	assert(p->block);
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
}

void tex_define_macro_func(struct tex_parser *p, char *cs, void (*handler)(struct tex_parser*, struct tex_macro)){
	assert(p);
	assert(p->block->macros_n < MACRO_MAX);

	p->block->macros[p->block->macros_n++] = (struct tex_macro){cs, .handler=handler};
}

//Parses macro arguments from parser input based on the given arglist and writes the
//arguments to the supplied buffer, which must be large enough to accomidate all numbered
//arguments in the arglist
//TODO: pass a new parser context, and set argument in the parser state
void tex_parse_arguments(struct tex_parser *p, struct tex_token *arglist, struct tex_token **args) {
	struct tex_token t;
	while(arglist && arglist->cat != TEX_PARAMETER){
		t = tex_read_token(p);
		//ERROR: premature end of input
		if(t.cat == TEX_INVALID) break;

		//TODO: this should throw an error
		assert(tex_token_eq(*arglist, t));

		arglist = arglist->next;
	}

	struct tex_token *bound_start = arglist;
	size_t i = 0, /*arg number*/
	       n = 0, /*bounding characters matched*/
	       s = 0; /*start of rewind*/
	while(arglist != NULL) {
		t = tex_read_token(p);
		assert(t.cat != TEX_INVALID); //ERROR: Premature end of input
		assert(t.cat != TEX_PARAMETER); //ERROR: shouldn't find parameters here

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
				args[i] = tex_token_append(args[i], t);
			}else{
				//Rewind to bound_start
				arglist = bound_start;

				//Copy s items (or all if no s) to the argument
				s = s?s:n;
				for(int t = s; t > 0; t--) {
					args[i] = tex_token_append(args[i], *arglist);
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
		//TODO: this should be an error, not assert
		assert(t.cat != TEX_PARAMETER || t.c == pn++);
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
void tex_handle_macro_general(struct tex_parser* p, struct tex_macro m){
	struct tex_token *parameters[9] = {};
	tex_parse_arguments(p, m.arglist, parameters);
	//TODO: handle parameters
	fprintf(stderr, "got parameter: \"");
	while(parameters[1]) {
		fprintf(stderr, "%c", parameters[1]->c);
		parameters[1] = parameters[1]->next;
	}
	fprintf(stderr, "\"\n");

	p->token = tex_token_join(m.replacement, p->token);
}

void tex_handle_macro_par(struct tex_parser* p, struct tex_macro m){
	assert(p);
	p->token = tex_token_prepend((struct tex_token){TEX_OTHER, .c='\n'}, p->token);
	p->token = tex_token_prepend((struct tex_token){TEX_OTHER, .c='\n'}, p->token);
}

//Handle \def macros
void tex_handle_macro_def(struct tex_parser* p, struct tex_macro m){
	struct tex_token cs = tex_read_token(p);
	assert(cs.cat == TEX_ESC);

	struct tex_token *arglist = tex_parse_arglist(p);
	struct tex_token *replacement = tex_read_block(p);
	assert(replacement);

	tex_define_macro(p, cs.s, arglist, replacement);
}


void tex_define_macro(struct tex_parser *p, char *cs, struct tex_token *arglist, struct tex_token *replacement) {
	assert(p);
	assert(p->block->macros_n < MACRO_MAX);

	p->block->macros[p->block->macros_n++] = (struct tex_macro){cs, arglist, replacement, tex_handle_macro_general};
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
	assert(p && p->char_stream);
	assert(!p->char_stream->has_next_char);
	p->char_stream->has_next_char = TRUE;
	if(p->char_stream->next_char == '\n')
		p->char_stream->line--;
		//NOTE: this doesn't set the correct column, but I don't think it should matter
}

struct tex_token tex_read_char(struct tex_parser *p) {
	char c;

	assert(p);
	if(p->char_stream == NULL)
		return (struct tex_token){TEX_INVALID};

	if(p->char_stream->has_next_char){
		p->char_stream->has_next_char = FALSE;
		c = p->char_stream->next_char;
	} else {
		assert(p->char_stream->type == TEX_STRING);
		//TODO: handle file inputs

		if(*p->char_stream->str == '\0') {
			struct tex_char_stream *old = p->char_stream;
			p->char_stream = p->char_stream->next;
			free(old);
			if(p->char_stream == NULL)
				return (struct tex_token){TEX_INVALID};
		}

		c = *(p->char_stream->str++);
		p->char_stream->next_char = c;
	}

	if(c == '\n'){
		p->char_stream->line++;
		p->char_stream->col=0;
	}else
		p->char_stream->col++;

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
			return (struct tex_token){TEX_IGNORE};

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
		return (struct tex_token){TEX_IGNORE};

	case TEX_PARAMETER:
		t = tex_read_char(p);
		if(t.cat == TEX_PARAMETER)
			return (struct tex_token){TEX_OTHER, .c=t.c};

		assert(isdigit(t.c));
		return (struct tex_token){TEX_PARAMETER, .c=t.c-'0'};

	case TEX_ESC:
		t.s = tex_read_control_sequence(p);
		p->state = TEX_SKIPSPACE;
		break;

	default:
		p->state = TEX_MIDLINE;
	}

	return t;
}

void tex_block_enter(struct tex_parser *p) {
	struct tex_block *b = malloc(sizeof *b);
	assert(b);
	memset(b, 0, sizeof *b);

	b->parent = p->block;
	p->block = b;
}

void tex_block_exit(struct tex_parser *p) {
	assert(p->block->parent); //Cannot exit global block
	p->block = p->block->parent;
}

struct tex_macro *tex_macro_find(struct tex_parser *p, struct tex_token t) {
	struct tex_block *b = p->block;
	while(b) {
		size_t n = 0;
		while(n < b->macros_n)
			if(strcmp(b->macros[n].cs, t.s) == 0) break;
			else n++;

		if(n < b->macros_n)
			return &b->macros[n];

		b = b->parent;
	}
	return NULL;
}

void tex_macro_replace(struct tex_parser *p, struct tex_token t) {
	struct tex_macro *m = tex_macro_find(p, t);
	//ERROR: macro not found
	assert(m);

	assert(m->handler);
	m->handler(p, *m);
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
		default: buf[i] = tok.c;
		}
	}

	return i;
}

void tex_free_parser(struct tex_parser *p){
	//TODO: actually free the parser
}

