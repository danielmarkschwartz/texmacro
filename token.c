#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "tex.h"

struct tex_token *tex_token_alloc(struct tex_token t) {
	struct tex_token *ret = malloc(sizeof(*ret));
	assert(ret != NULL);

	ret->cat = t.cat;
	if(t.cat == TEX_ESC)
		ret->s = strdup(t.s);
	else
		ret->c = t.c;

	ret->next = 0;
	ret->prev = 0;

	return ret;
}

void tex_token_free(struct tex_token *t) {
	if(t == NULL) return;

	if(t->cat == TEX_ESC)
		free(t->s);

	//TODO: this is a bad idea
	tex_token_free(t->next);
	free(t);
}

struct tex_token *tex_token_join(struct tex_token *before, struct tex_token *after) {
	if(before == NULL) return after;
	if(after == NULL) return before;

	struct tex_token *ret = before;

	while(before->next != NULL)
		before = before->next;

	before->next = after;
	after->prev = before;

	return ret;
}

struct tex_token *tex_token_append(struct tex_token *before, struct tex_token t) {
	return tex_token_join(before, tex_token_alloc(t));
}

struct tex_token *tex_token_prepend(struct tex_token t, struct tex_token *after) {
	return tex_token_join(tex_token_alloc(t), after);
}

void tex_token_print(struct tex_token t) {
	if(t.cat == TEX_ESC)
		printf("\\%s",t.s);
	else
		printf("%c_%i", t.c, t.cat);
}

int tex_token_eq(struct tex_token a, struct tex_token b) {
	if(a.cat != b.cat) return 0;
	if(a.cat == TEX_ESC) {
		return strcmp(a.s, b.s) == 0;
	}
	return a.c == b.c;
}
