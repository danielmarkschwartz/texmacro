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

#include <execinfo.h>
#include <signal.h>


//Handle stack dumps
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

//Opens the given number stream (0-15) for writing to given filename
//\openout<num>=<filname>
static struct tex_token *handle_openout(struct tex_parser* p, struct tex_val m){
	int n = tex_read_num(p);
	if(n < 0 || n > 15)
		p->error(p, "output stream must be between 0-15");

	struct tex_token t = tex_read_token(p);
	if(t.c != '=')
		p->error(p, "\\openout expects = after file number");

	char *filename = tex_read_filename(p);

	if(p->out[n]) fclose(p->out[n]);
	p->out[n] = fopen(filename, "w");
	if(p->out[n] == NULL)
		p->error(p, "could not open \"%s\" for writing", filename);

	return NULL;
}

//Writes a balanced block to given output stream (0-15)
static struct tex_token *handle_write(struct tex_parser* p, struct tex_val m){
	int n = tex_read_num(p);
	if(n < 0 || n > 15)
		p->error(p, "output stream must be between 0-15");

	if(p->out[n] == NULL)
		//NOTE: in TeX, this case would just be stdout by default
		p->error(p, "output stream %i is not open", n);

	struct tex_token *block = tex_read_block(p);
	if(!block)
		p->error(p, "expected block after \\write");

	char *out = tex_tokenlist_as_str(block);
	size_t outlen = strlen(out);

	if(fwrite(out, 1, outlen, p->out[n]) < outlen)
		p->error(p, "could not finish writing to file stream %i", n);

	return NULL;
}

static struct tex_token *handle_ifdefined(struct tex_parser* p, struct tex_val m){
	struct tex_token c = tex_read_token(p);
	if(c.cat != TEX_ESC) p->error(p, "Expected macro after \\ifdefined");

	struct tex_val *v = tex_val_find(p, c);
	if(v) return tex_token_alloc((struct tex_token){TEX_ESC, .s="iftrue"});
	return tex_token_alloc((struct tex_token){TEX_ESC, .s="iffalse"});
}

static struct tex_token *handle_filename(struct tex_parser* p, struct tex_val m){
	assert(p);

	struct tex_char_stream *s = p->char_stream;
	if(!s || !s->name) return NULL;

	return tex_str_as_tokenlist(s->name);
}

static struct tex_token *handle_catname(struct tex_parser* p, struct tex_val m){
	assert(p);

	struct tex_char_stream *s = p->char_stream;
		if(!s || !s->name) return NULL;

	//Skip past first /
	char *n = s->name;
	while(*n && *n != '/') n++;
	n++;

	//Find next /
	char *e = n;
	while(*e && *e != '/') e++;

	char temp = *e;
	*e = 0;

	struct tex_token *ret = tex_str_as_tokenlist(n);

	*e = temp;

	return ret;
}

static struct tex_token *handle_uppercase(struct tex_parser* p, struct tex_val m){
	assert(p);

	struct tex_token *block = tex_read_block(p);
	if(!block)
		p->error(p, "expected block after \\uppercase");

	struct tex_token *ret = block;
	while(block){
		if(block->c >= 'a' && block->c <= 'z')
			block->c -= 'a' - 'A';
		block = block->next;
	}

	return ret;
}

static struct tex_token *handle_lowercase(struct tex_parser* p, struct tex_val m){
	assert(p);

	struct tex_token *block = tex_read_block(p);
	if(!block)
		p->error(p, "expected block after \\lowercase");

	struct tex_token *ret = block;
	while(block){
		if(block->c >= 'A' && block->c <= 'Z')
			block->c -= 'A' - 'a';
		block = block->next;
	}

	return ret;
}

static struct tex_token *handle_expandafter(struct tex_parser* p, struct tex_val m){
	assert(p);

	struct tex_token first, second, *expansion;

	//Read first token
	first = tex_read_token(p);

	//Expand second token
	second = tex_read_token(p);
	expansion = tex_expand_token(p, second);
	if(!expansion) expansion = tex_token_alloc(second);

	//Prepend first token and second expansion to token stream
	return tex_token_prepend(first, expansion);
}

static struct tex_token *handle_newline(struct tex_parser* p, struct tex_val m){
	return tex_token_alloc((struct tex_token){TEX_OTHER, .c='\n'});
}

void init_macros(struct tex_parser *p) {
	tex_define_macro_func(p, "def", tex_handle_macro_def);
	tex_define_macro_func(p, "edef", tex_handle_macro_edef);
	tex_define_macro_func(p, "global", tex_handle_macro_global);
	tex_define_macro_func(p, "input", tex_handle_macro_input);
	tex_define_macro_func(p, "par", tex_handle_macro_par);
	tex_define_macro_func(p, "$", tex_handle_macro_dollarsign);
	tex_define_macro_func(p, "'", tex_handle_macro_singlequote);
	tex_define_macro_func(p, "\"", tex_handle_macro_doublequote);
	tex_define_macro_func(p, "%", tex_handle_macro_percent);
	tex_define_macro_func(p, "#", tex_handle_macro_hash);
	tex_define_macro_func(p, " ", tex_handle_macro_space);
	tex_define_macro_func(p, "iffalse", tex_handle_macro_iffalse);
	tex_define_macro_func(p, "iftrue", tex_handle_macro_iftrue);
	tex_define_macro_func(p, "openout", handle_openout);
	tex_define_macro_func(p, "write", handle_write);
	tex_define_macro_func(p, "ifdefined", handle_ifdefined);
	tex_define_macro_func(p, "filename", handle_filename);
	tex_define_macro_func(p, "catname", handle_catname);
	tex_define_macro_func(p, "uppercase", handle_uppercase);
	tex_define_macro_func(p, "lowercase", handle_lowercase);
	tex_define_macro_func(p, "expandafter", handle_expandafter);
	tex_define_macro_func(p, "newline", handle_newline);
}

int main(int argc, const char *argv[]) {
	signal(SIGSEGV, handler);   // install our handler
	signal(SIGINT, handler);   // install our handler

	struct tex_parser p;
	tex_init_parser(&p);
	init_macros(&p);

	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-") == 0)
			tex_input_file(&p, "<stdin>", stdin);
		else
			tex_input(&p, (char *)argv[i]);
	}

	//NOTE: TEX_INVALID characters do continue with a warning, as in regular tex,
	//but instead indicated end of input. By default only '\0' and '\127' are INVALID,
	//and this is by design to accomidate C strings gracefully

	//QUESTION: is it better to remove '\0' delimiter for input, to allow partial reads, or
	//just read all the input files in at once?

#define BUF_SIZE 1024
	char buf[BUF_SIZE];

	size_t n;
	do {
		n =  tex_read(&p, buf, BUF_SIZE);
		fwrite(&buf, sizeof(char), n, stdout);
	}while(n == BUF_SIZE);

	tex_free_parser(&p);

	return 0;
}

