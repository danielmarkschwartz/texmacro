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


struct tex_char_stream *tex_char_stream_str(char *input, struct tex_char_stream *next){
	struct tex_char_stream *i = malloc(sizeof(*i));
	assert(i);

	*i = (struct tex_char_stream){TEX_STRING, .name="", .line=0, .col=0, .str=input, .next=next};
	return i;
}


int main(int argc, const char *argv[]) {
	signal(SIGSEGV, handler);   // install our handler

	struct tex_parser p;

	char *input =
		"\\def\\section is #1\\par{<h2>#1</h2>\\par}\n"
		"\\section is This as test section\n\n"
		"This is not\n"
		"\\def\\test#1ababc{#1}\n"
		"\\test abababc "
		;

	tex_init_parser(&p);
	tex_define_macro_func(&p, "def", tex_handle_macro_def);
	tex_define_macro_func(&p, "par", tex_handle_macro_par);
	tex_input_str(&p, input);

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

