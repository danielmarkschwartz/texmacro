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

//Becomes the ENV value of the given key
//\env#1{<value}
static void handle_env(struct tex_parser* p, struct tex_val m){
	struct tex_parser a;
	tex_init_parser(&a);
	tex_input_str(&a, "-", "#1 {");
	struct tex_token *arglist = tex_parse_arglist(&a);
	tex_free_parser(&a);

	tex_block_enter(p);
	tex_parse_arguments(p, arglist);

	tex_input_str(p, "<env>", "}");
	char* env = getenv(tex_tokenlist_as_str(p->block->parameter[0]));
	if(env) tex_input_str(p, "<env>", env);
}

//Opens the given number stream (0-15) for writing to given filename
//\openout<num>=<filname>
static void handle_openout(struct tex_parser* p, struct tex_val m){
	int n = tex_read_num(p);
	if(n < 0 || n > 15)
		p->error("output stream must be between 0-15");

	struct tex_token t = tex_read_token(p);
	if(t.c != '=')
		p->error("\\openout expects = after file number");

	char *filename = tex_read_filename(p);

	if(p->out[n]) fclose(p->out[n]);
	p->out[n] = fopen(filename, "w");
	if(p->out[n] == NULL)
		p->error("could not open \"%s\" for writing", filename);
}

//Writes a balanced block to given output stream (0-15)
static void handle_write(struct tex_parser* p, struct tex_val m){
	int n = tex_read_num(p);
	if(n < 0 || n > 15)
		p->error("output stream must be between 0-15");

	if(p->out[n] == NULL)
		//NOTE: in TeX, this case would just be stdout by default
		p->error("output stream %i is not open", n);

	struct tex_token *block = tex_read_block(p);
	if(!block)
		p->error("expected block after \\write");

	char *out = tex_tokenlist_as_str(block);
	size_t outlen = strlen(out);

	while(outlen > 0) {
		fwrite(p->out[n],
	}
}

int main(int argc, const char *argv[]) {
	signal(SIGSEGV, handler);   // install our handler

	struct tex_parser p;

	char *input =
		"\\openout0=testout\n"
		"\\write0{hello world}\n"
		;

	tex_init_parser(&p);
	//tex_input_file(&p, "<stdin>", stdin);
	tex_define_macro_func(&p, "def", tex_handle_macro_def);
	tex_define_macro_func(&p, "par", tex_handle_macro_par);
	tex_define_macro_func(&p, "env", handle_env);
	tex_define_macro_func(&p, "openout", handle_openout);
	tex_define_macro_func(&p, "write", handle_write);
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

