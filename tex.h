#pragma once

#include <stdio.h>
#include <stdlib.h>

//Max length of control sequence
#define CS_MAX 1024
#define MAX_TOKEN_LIST 1024

struct tex_parser;

enum tex_category {
	//TODO: improve error handling
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

#define MACRO_MAX 128

struct tex_token {
	enum tex_category cat;
	union {
		char c;
		char *s;
	};
};

struct tex_macro {
	char *cs; //Macro control sequence string
	struct tex_token_stream *arglist, *replacement; //TEX_INVALID terminated token lists
	void (*handler)(struct tex_parser *, struct tex_macro);
};

enum tex_char_stream_type {
	TEX_STRING,
	TEX_FILE
};

struct tex_char_stream {
	enum tex_char_stream_type type;
	char *name;
	int line, col;
	union {
		char *str;
		FILE *file;
	};
	struct tex_char_stream *next;
	char next_char;
	int has_next_char;
};

struct tex_token_stream {
	struct tex_token tokens[MAX_TOKEN_LIST];
	size_t n, i;
	struct tex_token_stream *next;
};

struct tex_parser {
	struct tex_char_stream *char_stream;	//Stream of input characters
	struct tex_token_stream *token_stream;	//Stream of saved tokens (read before character input)

	char cat[128];  //Category code for ASCII characters
			//Note: 0 (esc) is switched with 12 (other)
			//internally for simplicity
	struct tex_macro macros[MACRO_MAX];
	size_t macros_n;

	enum tex_state state;
};

void tex_init_parser(struct tex_parser *p, char *input);
void tex_parse(struct tex_parser *p, char *buf, size_t n);
void tex_free_parser(struct tex_parser *p);
struct tex_token tex_read_token(struct tex_parser *p);
struct tex_token tex_read_char(struct tex_parser *p);
void tex_unread_char(struct tex_parser *p);
void tex_define_macro(struct tex_parser *p, char *cs,struct tex_token_stream *arglist,struct tex_token_stream *replacement);
char *tex_read_control_sequence(struct tex_parser *p);

//Token related functions
void tex_token_free(struct tex_token t);
struct tex_token_stream *tex_token_stream_alloc();
struct tex_token tex_token_stream_read(struct tex_token_stream **ts);
void tex_token_stream_append(struct tex_token_stream *ts, struct tex_token t);
