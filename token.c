#include <assert.h>

#include "tex.h"

void tex_token_free(struct tex_token t) {
	if(t.cat == TEX_ESC)
		free(t.s);
}

struct tex_token_stream *tex_token_stream_alloc() {
	struct tex_token_stream *ts = calloc(1,sizeof(*ts));
	assert(ts);
	return ts;
}


struct tex_token tex_token_stream_read(struct tex_token_stream **ts) {
	assert(ts);

	while(*ts != NULL){
		if((*ts)->i >= (*ts)->n) {
			(*ts) = (*ts)->next;
			continue;
		}
		return (*ts)->tokens[(*ts)->i++];
	}

	return (struct tex_token){TEX_INVALID};
}

void tex_token_stream_append(struct tex_token_stream *ts, struct tex_token t){
	assert(ts);
	while(ts->next != NULL) ts = ts->next;
	if(ts->n >= MAX_TOKEN_LIST) {
		ts->next = tex_token_stream_alloc();
		ts = ts->next;
	}
	ts->tokens[ts->n++] = t;
}

