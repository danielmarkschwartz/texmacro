#pragma once

#include <stdio.h>
#include <stdlib.h>

//Max length of control sequence
#define CS_MAX 1024
#define BUFSIZE 1024

#define TRUE 1
#define FALSE 0

struct tex_parser;

enum tex_category {
	TEX_STACK_POP = -2,
	TEX_ERROR = -1,
	TEX_OTHER = 0,
	TEX_BEGIN_GROUP,
	TEX_END_GROUP,
	TEX_MATH,
	TEX_ALIGN,
	TEX_EOL,
	TEX_PARAMETER,
	TEX_SUPER,
	TEX_SUB,
	TEX_IGNORE,
	TEX_SPACE,
	TEX_LETTER,
	TEX_ESC,
	TEX_ACTIVE,
	TEX_COMMENT,
	TEX_INVALID,

	TEX_CAT_NUM,
};

enum tex_state {
	TEX_NEWLINE,
	TEX_MIDLINE,
	TEX_SKIPSPACE
};

#define VAL_MAX 128

struct tex_token {
	enum tex_category cat;
	union {
		char c;
		char *s;
	};

	struct tex_token *next, *prev;
};

enum tex_val_type {
	TEX_MACRO,	//A macro as defined by \def or equivalent construction
	TEX_VAR		//A variable either set in TeX code or C code
};

struct tex_val {
	enum tex_val_type type;		//Type of value
	struct tex_token cs;		//Control sequence that invokes this value (must be TEX_ESC)

	//TEX_INVALID terminated token lists
	struct tex_token *arglist;	//MACRO ONLY: format of expected argument list
	struct tex_token *replacement;	//Tokens that should be evaluated in place of cs

	//MACRO ONLY: handler function
	struct tex_token *(*handler)(struct tex_parser *, struct tex_val);
};

enum tex_char_stream_type {
	TEX_BUF,
	TEX_FILE
};

struct tex_char_buf {
	char *buf;
	size_t i,n;
};

struct tex_char_stream {
	enum tex_char_stream_type type;
	char *name;
	int line, col;
	union {
		struct tex_char_buf buf;
		FILE *file;
	};

	char last;

	struct tex_char_stream *next;
};

struct tex_block {
	char cat[128];  //Category code for ASCII characters
			//Note: 0 (esc) is switched with 12 (other)
			//internally for simplicity
	struct tex_val vals[VAL_MAX];
	size_t vals_n;

	struct tex_block *parent;
};

struct tex_stack {
	struct tex_token macro;
	struct tex_token *parameter[9];
	struct tex_stack *parent;
};

struct tex_parser {
	struct tex_char_stream *char_stream;	//Stream of input characters
	struct tex_token *token;		//Stream of saved tokens (read before character input)
	struct tex_block *block;		//Hierarchy of namespaces
	struct tex_stack *stack;		//Hierarchy of macro replacements
	enum tex_state state;			//Current state of tokenizer

	//Error handler in printf style, should not return
	void (*error)(struct tex_parser *, char *fmt, ...);

	FILE *in[16], *out[16];			//Input/output streams

	int in_global;
};

//Parser related functions
void tex_init_parser(struct tex_parser *p);
void tex_parse(struct tex_parser *p, char *buf, size_t n);
void tex_free_parser(struct tex_parser *p);

void tex_input(struct tex_parser *p, char *filename);
void tex_input_file(struct tex_parser *p, char *name, FILE *file);
void tex_input_str(struct tex_parser *p, char *name, char *input);
void tex_input_token(struct tex_parser *p, struct tex_token t);
void tex_input_tokens(struct tex_parser *p, struct tex_token *ts, size_t n);


void tex_block_enter(struct tex_parser *p);
void tex_block_exit(struct tex_parser *p);

void tex_define_macro_tokens(struct tex_parser *p, char *cs, struct tex_token *arglist, struct tex_token *replacement);
void tex_define_macro_func(struct tex_parser *p, char *cs, struct tex_token * (*handler)(struct tex_parser*, struct tex_val));

struct tex_token *tex_handle_macro_par(struct tex_parser* p, struct tex_val m);
struct tex_token *tex_handle_macro_def(struct tex_parser* p, struct tex_val m);
struct tex_token *tex_handle_macro_edef(struct tex_parser* p, struct tex_val m);
struct tex_token *tex_handle_macro_global(struct tex_parser* p, struct tex_val m);
struct tex_token *tex_handle_macro_input(struct tex_parser* p, struct tex_val m);
struct tex_token *tex_handle_macro_dollarsign(struct tex_parser* p, struct tex_val m);
struct tex_token *tex_handle_macro_hash(struct tex_parser* p, struct tex_val m);
struct tex_token *tex_handle_macro_space(struct tex_parser* p, struct tex_val m);
struct tex_token *tex_handle_macro_iffalse(struct tex_parser* p, struct tex_val m);
struct tex_token *tex_handle_macro_iftrue(struct tex_parser* p, struct tex_val m);

struct tex_token tex_read_token(struct tex_parser *p);
struct tex_token tex_read_char(struct tex_parser *p);
void tex_unread_char(struct tex_parser *p);
char *tex_read_control_sequence(struct tex_parser *p);
int tex_read_num(struct tex_parser *p);
char *tex_read_filename(struct tex_parser *p);
void tex_parse_arguments(struct tex_parser *p, struct tex_token *arglist);
struct tex_token *tex_parse_arglist(struct tex_parser *p);
struct tex_token *tex_read_block(struct tex_parser *p);
int tex_read(struct tex_parser *p, char *buf, int n);
struct tex_token *tex_expand_token(struct tex_parser *p, struct tex_token t);

struct tex_val *tex_val_find(struct tex_parser *p, struct tex_token t);
void tex_val_set(struct tex_parser *p, struct tex_val v);

//Char stream related functions
struct tex_char_stream *tex_char_stream_str(char *input, struct tex_char_stream *next);

//Token related functions
struct tex_token *tex_token_alloc(struct tex_token t);
struct tex_token *tex_token_copy(struct tex_token *t);
void tex_token_free(struct tex_token *t);
struct tex_token *tex_token_join(struct tex_token *before, struct tex_token *after);
struct tex_token *tex_token_append(struct tex_token *before, struct tex_token t);
struct tex_token *tex_token_prepend(struct tex_token t, struct tex_token *after);
int tex_token_eq(struct tex_token a, struct tex_token b);
void tex_token_print(struct tex_token t);
void tex_tokenlist_print(struct tex_token *t);
char *tex_tokenlist_as_str(struct tex_token *t);
struct tex_token *tex_str_as_tokenlist(char *s);
size_t tex_tokenlist_len(struct tex_token *t);

struct tex_token *tex_macro_replace(struct tex_parser *p, struct tex_token t);
struct tex_token *tex_parameter_replace(struct tex_parser *p, struct tex_token t);
