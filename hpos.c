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
/*-----------------------------------------*/

int equal_ste(float a, float b)
{
    //return a == b;
    return (PSUBF(a,b) < ZERO);
}

void get_sample(unsigned dims[], int *off, int *len)
{  
    unsigned tmp;

    *len = 100;
    tmp = dims[2]/100;
    if(tmp < 10)
        *len = 10;

    *off = dims[2]/10;
}

/*-----------------------------------------*/
unsigned char eval_pre_XOR(float real, float pre[])
{
    uint32_t *pval = (uint32_t*)&real;
    uint32_t *p = (uint32_t*)pre;

    unsigned char i = LTIME;
    if(SUBF(real, pre[i]) > SUBF(real, pre[LPOS]))
        i = LPOS;
    if(SUBF(real, pre[i]) > SUBF(real, pre[LORENZO]))
        i = LORENZO;

    if(!XOR_PEQUAL(*pval, p[i]))
        i = UNSTEADY;

    return i;
}


unsigned char eval_pre(float real, float pre[])
{
    unsigned char i = LTIME;
    if(SUBF(real, pre[i]) > SUBF(real, pre[LPOS]))
        i = LPOS;
    if(SUBF(real, pre[i]) > SUBF(real, pre[LORENZO]))
        i = LORENZO;
    if(PSUBF(real, pre[i]) > MINI_PRE)
        i = UNSTEADY;

    return i;
}

int learn_x(FILE *fin,
        ctx_t *ctx, 
        front_t *front, 
        map_t *map, 
        unsigned dims[])
{
    unsigned i, j, k;
    unsigned idx, n, step;
    unsigned offset, len;
    float val, pre[3];
    float *buf = NULL;

    double begin = 0.0; // for timing

    idx = LTIME;
    step = dims[0] * dims[1];
    front_reset(front);

    begin = now_sec();

    get_sample(dims, &offset, &len);
    buf = (float*)malloc(len*step*sizeof(float));
    ctx->time1 += (now_sec() - begin);

    fseek(fin, offset*step*sizeof(float), SEEK_SET);
    fread(buf, sizeof(float), len*step, fin);

    begin = now_sec();
    for(n=0, j=0; j<dims[1]; j++)
        for(k=0; k<dims[0]; k++)
            front_push(front, j, k, buf[n++]);

    //init the steady points
    front_switch(front);
    for(j=0; j<dims[1]; j++)
        for(k=0; k<dims[0]; k++)
        {
            val = buf[n++];
            front_push(front, j, k, val);

            if(equal_ste(val, front_ltime(front, j, k)))
                map->map[j][k] = STEADY;
        }

    for(i=2; i<dims[2] && i<len; i++)
    {
        front_switch(front);
        for(j=0; j<dims[1]; j++)
            for(k=0; k<dims[0]; k++)
            {
                val = buf[n++];
                pre[LTIME] = front_ltime(front, j, k);
                pre[LPOS] = front_lpos(front, j, k);
                pre[LORENZO] = front_lorenzo(front, j, k);
                front_push(front, j, k, val);

                if(map->map[j][k] == STEADY)
                {
                    if(!equal_ste(val, pre[LTIME]))
                        map->map[j][k] = UNSTEADY;
                }

                if(map->map[j][k] != STEADY)
                {
                    map->map[j][k] = eval_pre_XOR(val, pre);
                }
            }
    }

    free(buf);
    front_reset(front);

    ctx->time1 += (now_sec() - begin);

    fseek(fin, 0, SEEK_SET);
    return 0;
}

int learn(FILE *fin, ctx_t *ctx, front_t *front, map_t *map, unsigned dims[])
{
    unsigned i, j, k;
    unsigned n, step;
    unsigned offset, len;
    float *buf = NULL;

    float val, pre[3];
    uint32_t *pval, *ppre;

    double begin = 0.0; // for timing

    pval = (uint32_t*)&val;
    ppre = (uint32_t*)pre;

    step = dims[0] * dims[1];
    front_reset(front);

    begin = now_sec();

    get_sample(dims, &offset, &len);
    buf = (float*)malloc(len*step*sizeof(float));
    ctx->time1 += (now_sec() - begin);

    fseek(fin, offset*step*sizeof(float), SEEK_SET);
    fread(buf, sizeof(float), len*step, fin);

    begin = now_sec();
    for(n=0, j=0; j<dims[1]; j++)
        for(k=0; k<dims[0]; k++)
            front_push(front, j, k, buf[n++]);

    //init the steady points
    front_switch(front);
    for(j=0; j<dims[1]; j++)
        for(k=0; k<dims[0]; k++)
        {
            val = buf[n++];
            front_push(front, j, k, val);
            pre[0] = front_ltime(front, j, k);

            //if(equal_ste(val, front_ltime(front, j, k)))
            if(XOR_EQUAL(*pval, ppre[0]))
                map->map[j][k] = STEADY;
        }

    for(i=2; i<dims[2] && i<len; i++)
    {
        front_switch(front);
        for(j=0; j<dims[1]; j++)
            for(k=0; k<dims[0]; k++)
            {
                val = buf[n++];
                pre[LTIME] = front_ltime(front, j, k);
                pre[LPOS] = front_lpos(front, j, k);
                pre[LORENZO] = front_lorenzo(front, j, k);
                front_push(front, j, k, val);

                if(map->map[j][k] == STEADY)
                {
                    //if(!equal_ste(val, pre[LTIME]))
                    if(!XOR_EQUAL(*pval, ppre[LTIME]))
                        map->map[j][k] = UNSTEADY;
                }
                else
                {
                    map->map[j][k] = eval_pre_XOR(val, pre);
                }
            }
    }

    free(buf);
    front_reset(front);

    ctx->time1 += (now_sec() - begin);

    fseek(fin, 0, SEEK_SET);
    return 0;
}

int my_compress(ctx_t *ctx, map_t *map, front_t *front,
        unsigned dims[], FILE *fin, const char *dst)
{
    char flag;
    FILE *fout;
    int type;

    flag = map_decision(map);
    type = map->type;

    map_convert(map);
    //map_print(map);

    fout = fopen(dst, "wb");

    //TODO: debug only, remove it
    //flag = 0;

    switch (flag)
    {
        case 0:
            printf("no prediction, no steady\n");
            compress_0(fin, ctx, fout);
            break;
        case 1:
            printf("with prediction:%s, but no steady\n", 
                    names[type]);
            compress_1(fin, ctx, front, map, fout);
            break;
        case 2:
            printf("no prediction, but with steady\n");
            compress_2(fin, ctx, front, map, fout);
            break;
        case 3:
            printf("with prediction:%s, and with steady\n", 
                    names[type]);
            compress_3(fin, ctx, front, map, fout);
            break;
        default:
            printf("wrong decision\n");
    }

    fclose(fout);
    return 0;
}

int learn_compress(const char *src, const char *dst,
        unsigned dims[])
{
    FILE *fin;

    ctx_t ctx;
    front_t front;
    map_t map;

    if((fin = fopen(src, "rb")) == NULL)
    {
        fprintf(stderr, "Error: [%s:%d]: Failed to  open input file :%s\n", __FILE__, __LINE__, src);
        exit(-1);
    }

    ctx_init(&ctx);
    front_init(&front, dims[0], dims[1]);
    map_init(&map, dims[0], dims[1]);

    ctx.fnum += 1;
    ctx.fsz += fsize_fp(fin);
    learn(fin, &ctx, &front, &map, dims);
    //map_print(&map);
    my_compress(&ctx, &map, &front, dims, fin, dst);

    ctx_print_more(&ctx, "Deflated");

    map_term(&map);
    front_term(&front);
    fclose(fin);
    return 0;
}

int nz_uncompress(const char *src, const char *dst)
{
    FILE *fin, *fout;
    ctx_t ctx;
    map_t map;
    nz_header hd;

    fin = fopen(src, "rb");
    fout = fopen(dst, "wb");
    if(fin == NULL || fout ==NULL)
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

    //TODO: debug only, remove it
    //hd.type = 3;

    switch (hd.type)
    {
        case 0:
            printf("no prediction, no steady\n");
            uncompress_0(fin, &ctx, &hd, fout);
            break;
        case 1:
            printf("with prediction:%s, but no steady\n", 
                    names[map.type]);
            uncompress_1(fin, &ctx, &hd, fout);
            break;
        case 2:
            printf("no prediction, but with steady\n");
            uncompress_2(fin, &ctx, &hd, fout);
            break;
        case 3:
            printf("with prediction:%s, and with steady\n", 
                    names[map.type]);
            uncompress_3(fin, &ctx, &hd, fout);
            break;
    }

    ctx_print_more(&ctx, "Inflate");
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
        nz_uncompress(inputFile, outputFile);
    }
    else
    {
        unsigned dims[3] = {0};
        dims[0] = atol(argv[4]);
        dims[1] = atol(argv[5]);
        dims[2] = atol(argv[6]);

        learn_compress(inputFile, outputFile, dims);
    }
    return 0;
}

