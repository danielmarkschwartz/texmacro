/* tex.c
*
* A basic, extensibile, TeX parser written in C.
*
*/

#define TRUE 1
#define FALSE 0

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tex.h"

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>


void handler(int sig) {
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, 2);
	exit(1);
}


void tex_token_free(struct tex_token t) {
	if(t.cat == TEX_ESC)
		free(t.s);
}

void tex_init_parser(struct tex_parser *p, char *input){
	char c;

	assert(input);

	memset(p, 0, sizeof *p);

	//Set default character codes
	//Note: 0 -> other, 12 -> esc internally
	p->cat['{'] = TEX_BEGIN_GROUP;
	p->cat['}'] = TEX_END_GROUP;
	p->cat['$'] = TEX_MATH;
	p->cat['&'] = TEX_ALIGN;
	p->cat['\n'] = TEX_EOL;
	p->cat['#'] = TEX_PARAMETER;
	p->cat['^'] = TEX_SUPER;
	p->cat['_'] = TEX_SUB;
	p->cat['\0'] = TEX_INVALID;
	p->cat[' '] = TEX_SPACE;
	for (c = 'A'; c <= 'Z'; c++) p->cat[(size_t)c] = TEX_LETTER;
	for (c = 'a'; c <= 'z'; c++) p->cat[(size_t)c] = TEX_LETTER;
	p->cat['\\'] = TEX_ESC;
	p->cat['~'] = TEX_ACTIVE;
	p->cat['%'] = TEX_COMMENT;
	p->cat[127] = TEX_INVALID;

	p->input = malloc(sizeof(*p->input));
	assert(p->input);

	*p->input = (struct tex_input){TEX_STRING, .name="", .line=0, .col=0, .str=input, .next=NULL};
}

void tex_set_handler(struct tex_parser *p, enum tex_category type, void *handler){
	assert(p);
	assert(type >= 0 && type < TEX_HANDLER_NUM);
	p->handler[type] = handler;
}

void tex_define_macro_func(struct tex_parser *p, char *cs, struct tex_token (*handler)(struct tex_parser*, struct tex_macro)){
	assert(p);
	assert(p->macros_n < MACRO_MAX);

	p->macros[p->macros_n++] = (struct tex_macro){cs, .handler=handler};
}

//Parses macro arguments from parser input based on the given arglist and writes the
//arguments to the supplied buffer, which must be large enough to accomidate all numbered
//arguments in the arglist
void tex_parse_arguments(struct tex_parser *p, struct tex_token *arglist, char **args) {

}

//Returns an arglist parsed from the parser input. This is what would follow a \def, as in
//\def\cs<arglist>{<replacement>}
struct tex_token *tex_parse_arglist(struct tex_parser *p) {
	return NULL;
}

//Reads one balanced block of tokens, or just the next token if not a block
struct tex_token *tex_read_block(struct tex_parser *p) {
	return NULL;
}

//Handle a general purpose macro, such as those previously defined by \def
struct tex_token tex_handle_macro_general(struct tex_parser* p, struct tex_macro m){
	//TODO: parse arguments base on input list
	return (struct tex_token){TEX_OTHER, .c='%'};
}

//Handle \def macros
struct tex_token tex_handle_macro_def(struct tex_parser* p, struct tex_macro m){
	struct tex_token cs = tex_read_token(p);
	fprintf(stderr, "Got token = {%i, '%c', \"%s\"}\n", cs.cat, cs.c, cs.s);
	assert(cs.cat == TEX_ESC);

	fprintf(stderr, "Got CS = '%s'\n", cs.s);
	struct tex_token *arglist = tex_parse_arglist(p);
	struct tex_token *replacement = tex_read_block(p);

	tex_define_macro(p, cs.s, arglist, replacement);

	return (struct tex_token){TEX_INVALID};
}

void tex_define_macro(struct tex_parser *p, char *cs, struct tex_token *arglist, struct tex_token *replacement) {
	assert(p);
	assert(p->macros_n < MACRO_MAX);

	p->macros[p->macros_n++] = (struct tex_macro){cs, arglist, replacement, tex_handle_macro_general};
}

char *tex_read_control_sequence(struct tex_parser *p) {
	assert(p);
	p->is_parsing_cs = TRUE;

	struct tex_token tok = tex_read_token(p);
	char *cs;

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
		} while ((tok = tex_read_token(p)).cat == TEX_LETTER);

		tex_unread_char(p);

		buf[n++] = 0;
		cs = strdup(buf);

		p->state = TEX_SKIPSPACE;
	}

	p->is_parsing_cs = FALSE;

	return cs;
}

//Unread the last character
void tex_unread_char(struct tex_parser *p) {
	assert(!p->has_next_char);
	p->has_next_char = TRUE;
}

//Read the next token from the parser input
struct tex_token tex_read_token(struct tex_parser *p) {
	char c;

	if(p->has_next_char){
		p->has_next_char = 0;
		c = p->next_char;
	} else {
		if(p->input == NULL)
			return (struct tex_token){TEX_INVALID};

		assert(p->input->type == TEX_STRING);
		//TODO: handle file inputs

		if(*p->input->str == '\0') {
			struct tex_input *old = p->input;
			p->input = p->input->next;
			free(old);
			if(p->input == NULL)
				return (struct tex_token){TEX_INVALID};
		}

		c = *(p->input->str++);
		p->next_char = c;
	}

	char cat = p->cat[(size_t)c];
	assert(cat <= TEX_HANDLER_NUM);

	if(cat == TEX_ESC && !p->is_parsing_cs) {
		char *cs = tex_read_control_sequence(p);
		return (struct tex_token){TEX_ESC, {.s= cs}};
	}

	return (struct tex_token){cat,{.c=c}};
}

//Write the next n characters to the output buffer, returns the number written.
int tex_read(struct tex_parser *p, char *buf, int n) {
	assert(n>=0);

	int i;
	for(i = 0; i < n; i++) {
		struct tex_token tok = tex_read_token(p);
		if(tok.cat == TEX_INVALID)
			break;

		if(p->handler[tok.cat]) {
			tok = p->handler[tok.cat](p, tok);
			if(tok.cat == TEX_INVALID) {
				i--;
				continue;
			}
		} else {
			p->state = TEX_MIDLINE;
		}

		//Handles build in escape sequences
		if(tok.cat == TEX_ESC) {
			if(strcmp(tok.s, "par") == 0) {
				tex_token_free(tok);
				tok = (struct tex_token){TEX_OTHER, .c='\n'};
			} else {
				//printf("Unknown token %s\n", tok.s);
				//assert(tok.cat != TEX_ESC); //Unhandled tex macro
				continue;
			}
		}

		buf[i] = tok.c;

	}

	return i;
}

void tex_free_parser(struct tex_parser *p){
	//Nothing to be done, yet
}

/*
struct tex_token handle_esc(struct tex_parser *p, struct tex_token t){
	//Look for user defined macro
}
*/

int main(int argc, const char *argv[]) {
	signal(SIGSEGV, handler);   // install our handler

	struct tex_parser p;

	char *input =
		"\\def\\testa{Test A}\n"
		"\\def\\test#1#2{Test #1 #2}\n\n"
		"\\testa\n\n"
		"\\test B {CC}\n";

	tex_init_parser(&p, input);

	tex_define_macro_func(&p, "def", tex_handle_macro_def);

	tex_set_handler(&p, TEX_ESC, handle_esc);
	tex_set_handler(&p, TEX_SPACE, handle_space);
	tex_set_handler(&p, TEX_EOL, handle_eol);
	tex_set_handler(&p, TEX_IGNORE, handle_ignore);
	tex_set_handler(&p, TEX_COMMENT, handle_comment);

	//NOTE: TEX_INVALID characters do continue with a warning, as in regular tex,
	//but instead indicated end of input. By default only '\0' and '\127' are INVALID,
	//and this is by design to accomidate C strings gracefully

	//QUESTION: is it better to remove '\0' delimiter for input, to allow partial reads, or
	//just read all the input files in at once?

#define BUF_SIZE 10
	char buf[BUF_SIZE];

	size_t n;
	do {
		n =  tex_read(&p, buf, BUF_SIZE);
		fwrite(&buf, sizeof(char), n, stdout);
	}while(n == BUF_SIZE);

	tex_free_parser(&p);

	return 0;
}

