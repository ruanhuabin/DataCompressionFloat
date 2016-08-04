/***********************************************
 Author: LT songbin
 Created Time: Tue 18 Jun 2013 04:52:16 PM CST
 File Name: compressors.h
 Description: 
 **********************************************/

#ifndef _LT_WORKERS_H_
#define _LT_WORKERS_H_

#ifdef __cplusplus
extern "C" {
#endif
///////////////////////////////////////////////////////////////

#include "common.h" //prediction_t


/* --- no predition, no steady points --- */
int compress_0(FILE *fin, ctx_t *ctx, FILE *fout);
int uncompress_0(FILE *fin, ctx_t *ctx, nz_header *hd, FILE *fout);

/* --- with predition, no steady points --- */
int compress_1(FILE *fin, ctx_t *ctx, front_t *front, map_t *map, FILE *fout);
int uncompress_1(FILE *fin, ctx_t *ctx, nz_header *hd, FILE *fout);

/* --- no predition, with steady points --- */
int compress_2(FILE *fin, ctx_t *ctx, front_t *front, map_t *map, FILE *fout);
int uncompress_2(FILE *fin, ctx_t *ctx, nz_header *hd, FILE *fout);

/* --- with predition, with steady points --- */
int compress_3(FILE *fin, ctx_t *ctx, front_t *front, 
               map_t *map, FILE *fout);
int uncompress_3(FILE *fin, ctx_t *ctx, nz_header *hd, FILE *fout);

///////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif

#endif //_LT_WORKERS_H_
