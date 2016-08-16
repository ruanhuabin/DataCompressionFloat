#ifndef _LT_WORKERS_H_
#define _LT_WORKERS_H_

#ifdef __cplusplus
extern "C"
{
#endif
///////////////////////////////////////////////////////////////

#include "common.h" //prediction_t

/* --- no predition, no steady points --- */
int run_compress(FILE *fin, ctx_t *ctx, FILE *fout, const int compressPrecision);
int run_uncompress(FILE *fin, ctx_t *ctx, mrczip_header_t *hd, FILE *fout);

///////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif

#endif //_LT_WORKERS_H_
