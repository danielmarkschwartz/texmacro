#ifndef TEX_H
#define TEX_H

//Max length of control sequence
#define CS_MAX 1024

enum tex_category {
	TEX_OTHER,
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
	TEX_ALL,
	TEX_HANDLER_NUM,
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
	char *macro, *replacement;
};

struct tex_parser {
	char *input;

	char (*handler[TEX_HANDLER_NUM]) (struct tex_parser*, struct tex_token);
	char cat[128];  //Category code for ASCII characters
			//Note: 0 (esc) is switched with 12 (other)
			//internally for simplicity
	struct tex_macro macros[MACRO_MAX];
	size_t macros_n;

	enum tex_state state;

	struct tex_token next_token;
	int has_next_token;
};

void tex_init_parser(struct tex_parser *p, char *input);
void tex_set_handler(struct tex_parser *p, enum tex_category type, void *handler);
void tex_parse(struct tex_parser *p, char *buf, size_t n);
void tex_free_parser(struct tex_parser *p);
struct tex_token tex_read_token(struct tex_parser *p);
void tex_unread_token(struct tex_parser *p, struct tex_token tok);

#endif /* end of include guard: TEX_H */