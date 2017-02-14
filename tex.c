/* tex.c
 *
 * A basic, extensibile, TeX parser written in C.
 *
 */

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

static void handle_env(struct tex_parser* p, struct tex_val m){
	struct tex_parser a;
	tex_init_parser(&a);
	tex_input_str(&a, "-", "#1 {");

	tex_block_enter(p);
	p->block->macro = m.cs;

	struct tex_token *arglist = tex_parse_arglist(&a);
	tex_parse_arguments(p, arglist);

	tex_input_str(p, "<env>", getenv(tex_tokenlist_as_str(p->block->parameter[0])));

	//TODO: properly exit block
	//tex_block_exit(p);
	tex_free_parser(&a);
}

int main(int argc, const char *argv[]) {
	signal(SIGSEGV, handler);   // install our handler

	struct tex_parser p;

	char *input =
		"\\def\\a b#1c{X#1X}\n"
		"\\a b OEU c\n\n"
		"Hello \\env USER \n\n"
		"HOME = \\env HOME \n"
		;

	tex_init_parser(&p);
	//tex_input_file(&p, "<stdin>", stdin);
	tex_define_macro_func(&p, "def", tex_handle_macro_def);
	tex_define_macro_func(&p, "par", tex_handle_macro_par);
	tex_define_macro_func(&p, "env", handle_env);
	tex_input_str(&p, "<str>", input);

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

