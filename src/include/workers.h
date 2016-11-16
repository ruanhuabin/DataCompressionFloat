/*******************************************************************
 *       Filename:  workers.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2016年08月12日 11时50分40秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ruan Huabin
 *          Email:  ruanhuabin@tsinghua.edu.cn
 *        Company:  Dep. of CS, Tsinghua Unversity
 *
 *******************************************************************/


#ifndef _LT_WORKERS_H_
#define _LT_WORKERS_H_

#ifdef __cplusplus
extern "C"
{
#endif
///////////////////////////////////////////////////////////////

#include "common.h" //prediction_t

/* --- no predition, no steady points --- */
int run_compress(FILE *fin, ctx_t *ctx, FILE *fout, const int compressPrecision, const char *dataConvertedType);
int run_uncompress(FILE *fin, ctx_t *ctx, mrczip_header_t *hd, FILE *fout, const char *dataConvertedType);

///////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif

#endif //_LT_WORKERS_H_
