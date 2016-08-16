#ifndef _LT_ADAPT_H_
#define _LT_ADAPT_H_

#ifdef __cplusplus
extern "C" {
#endif
///////////////////////////////////////////////////////////////
#include <pthread.h>
#include "common.h"

int learn_compress(ctx_t *ctx,
                   const char *src, const char *dst,
                   unsigned dims[]);
int nz_uncompress(ctx_t *ctx,
                  const char *src, const char *dst);


typedef struct _fnames_t {
  char **srcs;
  char **dsts;
  uint32_t **dims;
  int idx;
  int size;
  int num; //size:[0, num], idx~[0, size]
  pthread_mutex_t lock;
} fnames_t;

int fnames_init(fnames_t *fnames, char *fname, char *prefix, char *suffix);
void fnames_term(fnames_t *fnames);
void fnames_print(fnames_t *fnames);

int fnames_next(fnames_t *fnames, int *idx);


///////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif

#endif //_LT_ADAPT_H_
