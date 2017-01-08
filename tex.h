#ifndef TEX_H
#define TEX_H

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

#define MACRO_MAX 128

struct tex_macro {
	char *macro, *replacement;
};

struct tex_parser {
	size_t (*handler[TEX_HANDLER_NUM]) (struct tex_parser*, enum tex_category, char*, size_t);
	char cat[128];  //Category code for ASCII characters
			//Note: 0 (esc) is switched with 12 (other)
			//internally for simplicity
	struct tex_macro macros[MACRO_MAX];
	size_t macros_n;
};

void tex_init_parser(struct tex_parser *p);
void tex_set_handler(struct tex_parser *p, enum tex_category type, void *handler);
void tex_parse(struct tex_parser *p, char *buf, size_t n);
void tex_free_parser(struct tex_parser *p);

#endif /* end of include guard: TEX_H */
