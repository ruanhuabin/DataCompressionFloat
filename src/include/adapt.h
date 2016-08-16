#ifndef _LT_ADAPT_H_
#define _LT_ADAPT_H_

#ifdef __cplusplus
extern "C"
{
#endif
///////////////////////////////////////////////////////////////
#include <pthread.h>
#include "common.h"

int zip_compress(ctx_t *ctx, const char *src, const char *dst);
int zip_uncompress(ctx_t *ctx, const char *src, const char *dst);

typedef struct _file_container_t
{
	char **srcs;
	char **dsts;
	int idx;
	int size;
	int fileNum; //size:[0, fileNum], idx~[0, size]
	pthread_mutex_t lock;
} file_container_t;

int init_file_container(file_container_t *file_container, char *file_list_descriptor, char *prefix, char *suffix);
void free_file_container(file_container_t *file_container);
void print_file_container_info(file_container_t *fnames);
int get_next_file_idx(file_container_t *fnames, int *idx);

///////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif

#endif //_LT_ADAPT_H_
