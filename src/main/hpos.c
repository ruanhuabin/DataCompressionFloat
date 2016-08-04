/*******************************************************************
 *       Filename:  hpos.c                                     
 *                                                                 
 *    Description:  compress single precision float point number in mrc         
 *                                                                 
 *        Version:  1.0                                            
 *        Created:  2016年08月04日 14时41分34秒                                 
 *       Revision:  none                                           
 *       Compiler:  gcc                                           
 *                                                                 
 *         Author:  Ruan Huabin                                      
 *          Email:  ruanhuabin@gmail.com                                        
 *        Company:  HPC tsinghua                                      
 *                                                                 
 *******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zlib.h"
#include "common.h"
#include "workers.h"

//#define NUM (10)
#define BEGIN (1)
#define ZERO (0.0000002)
#define MINI_PRE (0.00195)
#define SUBF(a,b) (a>=b ? a-b:b-a)
#define PSUBF(a,b) (SUBF(a,b)/(a+ZERO))
#define EQUAL(a,b) (PSUBF(a,b) < ZERO)
#define TAG {printf("%s:%d\n", __FILE__,  __LINE__);}

#define XOR_EQUAL(a,b) (((a)^(b)) < 1)
#define XOR_PEQUAL(a,b) (((a)^(b)) < (1<<16))

static int simple_compress(FILE *inputFile, const char *outputFile, ctx_t *ctx)
{
    FILE *fout;
    fout = fopen(outputFile, "wb");

    if(fout == NULL)
    {
        fprintf(stderr, "Error: [%s:%d]: Failed to open file [%s] to write\n", __FILE__, __LINE__, outputFile);
        exit(-1);
    }

    compress_0(inputFile, ctx, fout);

    fclose(fout);

    return 0;
}


int mrc_compress(const char *src, const char *dst, unsigned dims[])
{
    FILE *fin;
    ctx_t ctx;
    if((fin = fopen(src, "rb")) == NULL)
    {
        fprintf(stderr, "Error: [%s:%d]: Failed to  open input file :%s\n", __FILE__, __LINE__, src);
        exit(-1);
    }

    ctx_init(&ctx);

    ctx.fnum += 1;
    ctx.fsz += fsize_fp(fin);
    simple_compress(fin, dst, &ctx);

    ctx_print_more(&ctx, "Deflated");

    fclose(fin);
    return 0;
}


int mrc_uncompress(const char *src, const char *dst)
{
    FILE *fin, *fout;
    ctx_t ctx;
    map_t map;
    nz_header hd;

    fin = fopen(src, "rb");
    fout = fopen(dst, "wb");
    if(fin == NULL || fout == NULL)
    {
        fprintf(stderr, "fail open file\n");
        return -1;
    }

    ctx_init(&ctx);
    ctx.fnum += 1;
    nz_header_init(&hd, 0);
    hd.map = &map;
    nz_header_read(fin, &hd);
    nz_header_print(&hd);
    uncompress_0(fin, &ctx, &hd, fout);
    ctx_print_more(&ctx, "Decompress");
    nz_header_term(&hd);
    fclose(fout);
    fclose(fin);
    return 0;
}

int main(int argc, char *argv[])
{
    printf("ZLIB:%s\n", zlib_version);
    if(argc < 4)
    {
        printf("Usage: %s -oz <infile> <outfile> <nx> <ny> <nz> [nt]\n", argv[0]);
        printf("Usage: %s -ou <infile> <outfile>\n", argv[0]);
        return -1;
    }

    char *inputFile  = argv[2];
    char *outputFile = argv[3];
    
    /**
     *  Just do uncompress
     */
    if(strcmp(argv[1], "-ou") == 0)
    {
        mrc_uncompress(inputFile, outputFile);
    }
    else
    {
        unsigned dims[3] = {0};
        dims[0] = atol(argv[4]);
        dims[1] = atol(argv[5]);
        dims[2] = atol(argv[6]);

        mrc_compress(inputFile, outputFile, dims);
    }
    return 0;
}

