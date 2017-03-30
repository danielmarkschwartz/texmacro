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

struct tex_token *tex_token_copy(struct tex_token *t) {
	struct tex_token *ret = NULL;
	while(t) {
		ret = tex_token_append(ret, *t);
		t = t->next;
	}
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
	switch(t.cat) {
	case TEX_ESC: printf("\\%s ",t.s); break;
	case TEX_PARAMETER: printf("#%i ", t.c); break;
	default:
		if(t.c == 0) printf("NULL_%i ", t.cat);
		else printf("%c_%i ", t.c, t.cat);
	}
}

void tex_tokenlist_print(struct tex_token *t) {
	while(t) {
		tex_token_print(*t);
		t = t->next;
	}
}

int tex_token_eq(struct tex_token a, struct tex_token b) {
	if(a.cat != b.cat) return 0;
	if(a.cat == TEX_ESC) {
		return strcmp(a.s, b.s) == 0;
	}
	return a.c == b.c;
}

size_t tex_tokenlist_len(struct tex_token *t) {
	size_t n = 0;
	while(t) {
		switch(t->cat){
		case TEX_ESC: n += strlen(t->s); break;
		case TEX_PARAMETER: n += 2; break;
		default: n++;
		}
		t = t->next;
	}
	return n;
}

char *tex_tokenlist_as_str(struct tex_token *t) {
	size_t len = tex_tokenlist_len(t);
	char *s, *ret = malloc(len+1);
	assert(ret);

	s = ret;
	while(t) {
		switch(t->cat){
		case TEX_ESC: *(s++) = '\\'; strcpy(s, t->s); s += strlen(t->s); break;
		case TEX_PARAMETER: *(s++) = '#'; *(s++) = '0'+t->c; break;
		default: *(s++) = t->c;
		}
		t = t->next;
	}
	*s = 0;
	return ret;
}

//Returns a token list where all the characters of s are tokenized as TEX_OTHER
struct tex_token *tex_str_as_tokenlist(char *s) {
	if(!s) return NULL;

	struct tex_token *t = NULL;
	while(*s) t = tex_token_append(t, (struct tex_token){TEX_OTHER, .c=*(s++)});

	return t;
}
