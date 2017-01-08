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

void tex_init_parser(struct tex_parser *p){
	char c;

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
	p->cat['\0'] = TEX_IGNORE;
	p->cat[' '] = TEX_SPACE;
	for (c = 'A'; c <= 'Z'; c++) p->cat[c] = TEX_LETTER;
	for (c = 'a'; c <= 'z'; c++) p->cat[c] = TEX_LETTER;
	p->cat['\\'] = TEX_ESC;
	p->cat['~'] = TEX_ACTIVE;
	p->cat['%'] = TEX_COMMENT;
	p->cat[127] = TEX_INVALID;
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

void tex_parse(struct tex_parser *p, char *buf, size_t n){
	size_t i;

	assert(p);
	assert(buf);

	for (i = 0; i < n; i++) {
		char cat = p->cat[buf[i]];
		assert(cat <= TEX_HANDLER_NUM);
		if(p->handler[cat])
			i += p->handler[cat](p, cat, &buf[i], n-i);
	}
}

void tex_free_parser(struct tex_parser *p){
	//Nothing to be done, yet
}

size_t handle_esc(struct tex_parser *p, enum tex_category cat, char *buf, size_t n) {
	size_t i, m;
	buf++;	//Remove leading ESC character

	for(i = 0; buf[i] != ' ' && buf[i] !='\\' && buf[i] != '\n' && i < n; i++);

	if(i == 0 && buf[i] == ' ') i++;

	char *macro = malloc(sizeof(char)*i);
	assert(macro != NULL);

	strncpy(macro, buf, i);

	for(m = 0; m < p->macros_n; m++)
		if(strcmp(p->macros[m].macro, macro) == 0)
			break;

	if(m >= p->macros_n)
		return i;

	//TODO: handle other macro replacement features, such as arguments
	char *replacement = p->macros[m].replacement;
	tex_parse(p, replacement, strlen(replacement));

	return i;
}

size_t handle_letter(struct tex_parser *p, enum tex_category cat, char *buf, size_t n) {
	printf("%c", buf[0]);
	return 0;
}

size_t handle_other(struct tex_parser *p, enum tex_category cat, char *buf, size_t n) {
	printf("%c", buf[0]);
	return 0;
}

size_t handle_eol(struct tex_parser *p, enum tex_category cat, char *buf, size_t n) {
	printf("\n");
	return 0;
}

int main(int argc, const char *argv[]) {

	struct tex_parser p;

	tex_init_parser(&p);

	tex_define_macro(&p, " ", " ");
	tex_define_macro(&p, "TeX", "TexMacro");
	tex_define_macro(&p, "Bango", "Bongo");

	tex_set_handler(&p, TEX_SPACE, handle_other);
	tex_set_handler(&p, TEX_ESC, handle_esc);
	tex_set_handler(&p, TEX_OTHER, handle_other);
	tex_set_handler(&p, TEX_LETTER, handle_letter);
	tex_set_handler(&p, TEX_EOL, handle_eol);
	char *input = "\\TeX\\ \\Bango\n\nPP 2\n";
	tex_parse(&p, input, strlen(input));
	tex_free_parser(&p);

	return 0;
}

