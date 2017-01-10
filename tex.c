/* tex.c
*
* A basic, extensibile, TeX parser written in C.
*
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tex.h"

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
	for (c = 'A'; c <= 'Z'; c++) p->cat[c] = TEX_LETTER;
	for (c = 'a'; c <= 'z'; c++) p->cat[c] = TEX_LETTER;
	p->cat['\\'] = TEX_ESC;
	p->cat['~'] = TEX_ACTIVE;
	p->cat['%'] = TEX_COMMENT;
	p->cat[127] = TEX_INVALID;

	p->input = input;
}

void tex_set_handler(struct tex_parser *p, enum tex_category type, void *handler){
	assert(p);
	assert(type >= 0 && type < TEX_HANDLER_NUM);
	p->handler[type] = handler;
}

void tex_define_macro(struct tex_parser *p, char *macro, char *replacement) {
	assert(p);
	assert(p->macros_n < MACRO_MAX);

	p->macros[p->macros_n++] = (struct tex_macro){macro, replacement};
}

char *tex_read_control_sequence(struct tex_parser *p) {
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

		tex_unread_token(p, tok);

		buf[n++] = 0;
		cs = strdup(buf);

		p->state = TEX_SKIPSPACE;
	}

	return cs;
}

void tex_unread_token(struct tex_parser *p, struct tex_token tok) {
	assert(!p->has_next_token);
	p->next_token = tok;
	p->has_next_token = 1;
}

//Read the next token from the parser input
struct tex_token tex_read_token(struct tex_parser *p) {

	if(p->has_next_token){
		p->has_next_token = 0;
		return p->next_token;
	}

	if(p->input == 0)
		return (struct tex_token){0, TEX_INVALID};

	char c = *(p->input++);
	char cat = p->cat[c];
	assert(cat <= TEX_HANDLER_NUM);

	if(cat == TEX_ESC) {
		char *cs = tex_read_control_sequence(p);
		return (struct tex_token){cat, {.s= cs}};
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
			char c = p->handler[tok.cat](p, tok);
			if(c == 0) i--;
			else buf[i] = c;
		} else {
			buf[i] = tok.c;
			p->state = TEX_MIDLINE;
		}

	}

	return i;
}

void tex_free_parser(struct tex_parser *p){
	//Nothing to be done, yet
}

//Ignored characters do nothing
char handle_ignore(struct tex_parser *p, struct tex_token t){
	return 0; //Indicates this handler returned no input
}

//Handle a comment by stripping the line and returning no input
char handle_comment(struct tex_parser *p, struct tex_token t){
	while(tex_read_token(p).cat != TEX_EOL);
	return 0; //Indicates this handler returned no input
}

//EOL behavior depends on the current parser state
char handle_eol(struct tex_parser *p, struct tex_token t){
	char c;

	switch(p->state){
	case TEX_NEWLINE:	c = '\n'; break; // '\n' represents a \par tag here
	case TEX_SKIPSPACE:	c = 0;    break; // Ignore newline
	case TEX_MIDLINE:	c = ' ';  break; // Convert to space character
	default: assert(p->state == TEX_NEWLINE || p->state == TEX_SKIPSPACE || p->state == TEX_MIDLINE);
	}

	p->state = TEX_NEWLINE;
	return c;
}

//TODO: delete this function
char handle_esc(struct tex_parser *p, struct tex_token t){

	//TODO: In what part of the code do I go about expanding macros?
	//Actually, probably in the tex_read() function directly
	printf("<%s>", t.s);
	//TODO: handle \def tokens special, parsing the paramater list

	//TODO: otherwise than \def, find macro in namespace, and try to match paramater list

	//TODO: create parser in macro namespace, and parse replacement

	return 0;
}

//SPACE behavior depends on the current parser state
char handle_space(struct tex_parser *p, struct tex_token t){
	assert(p->state == TEX_NEWLINE || p->state == TEX_SKIPSPACE || p->state == TEX_MIDLINE);

	if(p->state == TEX_NEWLINE || p->state == TEX_SKIPSPACE)
		return 0; //Ignore space

	p->state = TEX_SKIPSPACE;
	return ' ';
}

int main(int argc, const char *argv[]) {

	struct tex_parser p;

	char *input = "\\TeX\\ %\\Bango\n\nSome     more        text\n\nPP 2\n";
	tex_init_parser(&p, input);

	tex_define_macro(&p, " ", " ");
	tex_define_macro(&p, "TeX", "TexMacro");
	tex_define_macro(&p, "Bango", "Bongo");

	//TODO: handle ESC sequences
	//based on description in The Tex Book

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

