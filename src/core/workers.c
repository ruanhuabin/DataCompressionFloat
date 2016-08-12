/***********************************************
Author: LT songbin
Created Time: Tue 18 Jun 2013 04:52:16 PM CST
File Name: workers.h
Description: compress the streams with 
different zips 
 **********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "workers.h"
#include "nzips.h"

#define ZIP_FAST Z_RLE
#define ZIP_GOOD Z_DEFAULT_STRATEGY

#define CHUNK (6*1048576)
#define EWIDTH (4)  //sizeof(float) = 4
#define ADAPT_NUM (2)   //compress 3, 2, 1
#define ADAPT_NUMZ (0)  //compress 0


/* ---------------- aulix ----------------- */
void init_zips(mzip_t zips[], int chk)
{
    mzip_init(&zips[0], chk, ZLIB_DEF, ZIP_FAST);
    mzip_init(&zips[1], chk, ZLIB_DEF, ZIP_FAST);
    mzip_init(&zips[2], chk, ZLIB_DEF, ZIP_FAST);
    mzip_init(&zips[3], chk, ZLIB_DEF, ZIP_FAST);
}

void term_zips(mzip_t zips[])
{
    int i;
    for(i = 0; i < 4; i ++)
        mzip_term(&zips[i]);
}

int get_chk(unsigned dx, unsigned dy)
{
    int chk, base;

    base = dx * dy;
    chk = base;
    while (chk < CHUNK)
    {
        chk += base;
    }

    return chk;
}

void check_bheader(char bhd[], int n)
{
    int i;
    btype_t btype;
    uint32_t len;
    for(i = 0; i < n; i ++)
    {
        unpack_header(bhd+i*HDR_SIZE, &btype, &len);
        printf("btype:%d, len:%d\n", btype, len);
    }
}


/* ------- these two for uncompress ---------- 
 *  unzip_1 is for uncompress_0 and 1
 *  unzip_3 is for uncompress_2 and 3
 */
void _unzip_1(FILE *fin, 
        mzip_t zips[], 
        char *outs[], uint32_t chk)
{
    int j;
    char *p;
    char bhd[HDR_SIZE*4] = {0};
    btype_t btypes[4];

    double begin; //Timing

    //zip-block headers
    fread(bhd, 1, HDR_SIZE*4, fin);
    for(j = 0, p = bhd; j < 4; j++)
    {
        unpack_header(p, &(btypes[j]), &(zips[j].inlen));
        p += HDR_SIZE;

        fread(zips[j].in, 1, zips[j].inlen, fin);
    }

    for(j = 0; j < 4; j++)
    {
        begin = now_sec();
        zips[j].unzipfun(&(zips[j]), btypes[j], 
                chk, &(outs[j]));
        zips[j].time2 += (now_sec() - begin);
    }

    return;
}

void _unzip_3(FILE *fin, mzip_t zips[], char *outs[], 
        uint32_t chk1, uint32_t chk0)
{
    int j, ret;
    char *p;
    char bhd[HDR_SIZE*5] = {0};
    btype_t btypes[5];

    double begin;

    //zip-block headers
    ret = fread(bhd, 1, HDR_SIZE*5, fin);
    if(ret != HDR_SIZE*5)
    {
        TAG;
        fprintf(stderr, "[ERROR]: fail read!\n");
        exit(-1);
    }
    for(j = 0, p = bhd; j < 5; j++)
    {
        unpack_header(p, &(btypes[j]), &(zips[j].inlen));
        p += HDR_SIZE;

        //TAG;
        //printf("%u\n", zips[j].inlen);
        fread(zips[j].in, 1, zips[j].inlen, fin);
    }

    for(j = 0; j < 4; j++)
    {
        begin = now_sec();
        zips[j].unzipfun(&(zips[j]), btypes[j], chk1, &(outs[j]));
        zips[j].time2 += (now_sec() - begin);
    }

    begin = now_sec();
    zips[j].unzipfun(&(zips[j]), btypes[j], 
            EWIDTH*chk0, &(outs[j])); //j=4
    zips[j].time2 += (now_sec() - begin);

    return;
}

/* -----------------------------------------*/
void _convert3(float *buf, int num, 
        front_t *front, 
        map_t *map,
        char *zins[])
{
    int i, j, k;
    int n, z, d, type;
    float val, pre, diff;
    uint32_t *pval, *ppre, *pdiff;

    char *pd = (char*)(&diff);
    float *p0 = (float*)zins[4];

    n = 0;
    z = 0;
    d = 0;
    type = map->type;

    pval = (uint32_t*)&val;
    ppre = (uint32_t*)&pre;
    pdiff = (uint32_t*)&diff;

    for(i = 0; i < num; i ++)
    {
        front_switch(front);
        for(j = 0; j < map->dy; j++)
            for(k = 0; k < map->dx; k ++)
            {
                val = buf[n++];

                if(map->map[j][k] == 1) //STEADY points
                {
                    pre = front_ltime(front, j, k);
                    *pdiff = DO_XOR4(*pval, *ppre);
                    p0[z++] = diff;

                    //TODO: RR nothing to do with steady points
                    //p0[z++] = val;
                }
                else
                {
                    if(type == LORENZO)
                        pre = front_lorenzo(front, j, k);
                    else if(type == LTIME)
                        pre = front_ltime(front, j, k);
                    else
                        pre = front_lpos(front, j, k);

                    //diff = float_xor2(val, pre);
                    *pdiff = DO_XOR3(*pval, *ppre);

                    zins[0][d] = pd[0];
                    zins[1][d] = pd[1];
                    zins[2][d] = pd[2];
                    zins[3][d] = pd[3];
                    d ++;
                }

                front_push(front, j, k, val);
            }
    }

    return;
}

void _unconvert3(float *buf,
        int num,
        front_t *front,
        map_t *map,
        char *zouts[])
{
    int i, j, k;
    int n, z, d, type;
    float val, pre, diff;
    uint32_t *pval, *ppre, *pdiff;

    char *pd = (char*)(&diff);
    float *p0 = (float*)zouts[4];

    n = 0;
    z = 0;
    d = 0;
    type = map->type;

    pval = (uint32_t*)&val;
    ppre = (uint32_t*)&pre;
    pdiff = (uint32_t*)&diff;

    for(i = 0; i < num; i ++)
    {
        front_switch(front);
        for(j = 0; j < map->dy; j++)
            for(k = 0; k < map->dx; k ++)
            {
                if(map->map[j][k] == 1) //STEADY points
                {
                    diff = p0[z++]; 
                    pre = front_ltime(front, j, k);
                    *pval = DO_XOR4(*pdiff, *ppre);

                    //TODO: RR nothing to do with steady points
                    //val = p0[z++];
                }
                else
                {
                    if(type == LORENZO)
                        pre = front_lorenzo(front, j, k);
                    else if(type == LTIME)
                        pre = front_ltime(front, j, k);
                    else
                        pre = front_lpos(front, j, k);


                    pd[0] = zouts[0][d];
                    pd[1] = zouts[1][d];
                    pd[2] = zouts[2][d];
                    pd[3] = zouts[3][d];
                    d ++;

                    *pval = DO_XOR3(*pdiff, *ppre);
                }

                buf[n++] = val;
                front_push(front, j, k, val);
            }
    }

    return;
}

int compress_3(FILE *fin, 
        ctx_t *ctx,
        front_t *front,
        map_t *map, FILE *fout)
{
    int i,j;
    int base, num, chk, chk0, chk1;
    nz_header hd;
#ifdef _OUTPUT_ZIP_
    char bhd[5*HDR_SIZE] = {0};
#endif
    float *buf;

    mzip_t zips[5];
    char *zins[5], *zouts[5];
    int32_t zlens[5];

    int flag; // adapt: select the compressor

    double begin, zbegin; //for timing

    begin = now_sec();

    chk = get_chk(map->dx, map->dy);
    base = map->dx * map->dy;
    chk0 = map->zcnt * (chk / base);
    chk1 = chk - chk0;

    buf = (float*)malloc(sizeof(float) * chk);
    if(buf == NULL)
    {
        fprintf(stderr, "fail to alloc mem\n");
        return -1;
    }

    init_zips(zips, chk1);
    mzip_init(&zips[4], chk0*4, ZLIB_DEF, ZIP_FAST); 
    for(j = 0; j < 5; j++)
        zins[j] = zips[j].zin;


    /* begin to set zip file header */
    nz_header_init(&hd, 3);
    hd.chk = chk;
    hd.map = map;
    for(j = 0; j < 5; j++)
        hd.ztypes[j] = zips[j].ztype;

    ctx->zipTime += (now_sec() - begin);
    hd.fsz = fsize_fp(fin);
    ////nz_header_write(fout, &hd);
    /* finish zip file header */

    flag = 0;
    num = fread(buf, sizeof(uint32_t), chk, fin);
    while(num > 0)
    {
        i = num/base;
        //printf("[%d]:%d\n", i, num);

        begin = now_sec();
        _convert3(buf, i, front, map, zins);
        //ctx->time1 += (now_sec() - begin);

        for(j = 0; j < 4; j++)
            zips[j].inlen = i * (base - map->zcnt);
        zips[4].inlen = i*map->zcnt *4;
        ctx->zipTime += (now_sec() - begin);

        //adapt: select the compressor
        if(flag == 0)
        {
            begin = now_sec();
            for(j = 0; j < ADAPT_NUM; j++)
            {
                mzip_def_test0(&(zips[j]));
                hd.ztypes[j] = zips[j].ztype;
            }

            ctx->zipTime += (now_sec() - begin);
#ifdef _OUTPUT_ZIP_ 
            nz_header_write(fout, &hd);
#endif
            flag = 1;
        }
        //end of select compressors

        begin = now_sec();
        for(j = 0; j < 5; j++)
        {
            zbegin = now_sec();
            zips[j].zipfun(&(zips[j]), &zouts[j], &zlens[j]);
            zips[j].time1 += (now_sec() - zbegin);
        }
        ctx->zipTime += (now_sec() - begin);

#ifdef _OUTPUT_ZIP_
        for(j = 0; j < 5; j++)
            memcpy(bhd+j*HDR_SIZE, zouts[j], HDR_SIZE);
        fwrite(bhd, 1, 5*HDR_SIZE, fout);

        for(j = 0; j < 5; j++)
            fwrite(zouts[j]+HDR_SIZE, 1, zlens[j] - HDR_SIZE, fout);
#endif

        num = fread(buf, sizeof(float), chk, fin);
    }

    free(buf);

#ifdef _PRINT_ZIPS_
    print_results(zips, 5);
#endif

    for(j = 0; j < 5; j++)
        ctx->zfsz += zips[j].zfsz;

    for(j = 0; j < 5; j++)
        mzip_term(&zips[j]);
    return 0;
}

int uncompress_3(FILE *fin,
        ctx_t *ctx,
        nz_header *hd,
        FILE *fout)
{
    uint32_t i, j, num;
    uint32_t base, chk, chk0, chk1;
    uint64_t fsz;
    float *buf;
    front_t front;

    mzip_t zips[5];    /* 4 float zips, 1 zero zip */
    char *outs[5];
    map_t *map = hd->map;

    double begin;

    begin = now_sec();

    fsz = hd->fsz/4;
    chk = hd->chk;
    base = map->dx * map->dy;
    chk0 = (chk/base) * map->zcnt;
    chk1 = chk - chk0;

    buf = (float*)malloc(sizeof(float) * chk);
    front_init(&front, map->dx, map->dy);

    mzip_init(&(zips[4]), EWIDTH*chk0, 
            hd->ztypes[4] + 1, 0);
    for(j = 0; j < 4; j++)
        mzip_init(&(zips[j]), chk1, hd->ztypes[j] + 1, 0);

    ctx->unzipTime += (now_sec() - begin);

    num = fsz/chk;
    for(i = 0; i < num; i ++)
    {
        _unzip_3(fin, zips, outs, chk1, chk0);

        begin = now_sec();
        _unconvert3(buf, chk/base, &front, map, outs);
        ctx->unzipTime += (now_sec() - begin);

#ifdef _OUTPUT_UNZIP_
        fwrite(buf, sizeof(float), chk, fout);
#endif
    }

    chk = fsz % chk;
    chk0 = (chk/base) * map->zcnt;
    chk1 = chk - chk0;
    if(chk > 0)
    {
        _unzip_3(fin, zips, outs, chk1, chk0);

        begin = now_sec();
        _unconvert3(buf, chk/base, &front, map, outs);
        ctx->unzipTime += (now_sec() - begin);

#ifdef _OUTPUT_UNZIP_
        fwrite(buf, sizeof(float), chk, fout);
#endif
    }

    front_term(&front);
    free(buf);

#ifdef _PRINT_ZIPS_
    print_results(zips, 5);
#endif

    for(j = 0; j < 5; j++)
    {
        ctx->fsz += zips[j].fsz;
        ctx->zfsz += zips[j].zfsz;
        ctx->unzipTime += zips[j].time2;

        mzip_term(&(zips[j]));
    }

    return 0;
}

/* ------- for (un)compress_2 -------------*/
void _convert2(float *buf, int num, 
        front_t *front, 
        map_t *map,
        char *zins[])
{
    int i, j, k;
    int n, z, d, type;
    float val, pre, diff;
    uint32_t *pval, *ppre, *pdiff;

    char *pv = (char*)(&val);
    float *p0 = (float*)zins[4];

    n = 0;
    z = 0;
    d = 0;
    type = map->type;

    pval = (uint32_t*)&val;
    ppre = (uint32_t*)&pre;
    pdiff = (uint32_t*)&diff;

    for(i = 0; i < num; i ++)
    {
        front_switch(front);
        for(j = 0; j < map->dy; j++)
            for(k = 0; k < map->dx; k ++)
            {
                val = buf[n++];

                if(map->map[j][k] == 1) //STEADY points
                {
                    pre = front_ltime(front, j, k);
                    *pdiff = DO_XOR4(*pval, *ppre);
                    p0[z++] = diff;

                    //TODO: RR nothing to do with steady points
                    //p0[z++] = val;
                }
                else
                {
                    zins[0][d] = pv[0];
                    zins[1][d] = pv[1];
                    zins[2][d] = pv[2];
                    zins[3][d] = pv[3];
                    d ++;
                }

                front_push(front, j, k, val);
            }
    }

    return;
}

void _unconvert2(float *buf,
        int num,
        front_t *front,
        map_t *map,
        char *zouts[])
{
    int i, j, k;
    int n, z, d, type;
    float val, pre, diff;
    uint32_t *pval, *ppre, *pdiff;

    char *pv = (char*)(&val);
    float *p0 = (float*)zouts[4];

    n = 0;
    z = 0;
    d = 0;
    type = map->type;

    pval = (uint32_t*)&val;
    ppre = (uint32_t*)&pre;
    pdiff = (uint32_t*)&diff;

    for(i = 0; i < num; i ++)
    {
        front_switch(front);
        for(j = 0; j < map->dy; j++)
            for(k = 0; k < map->dx; k ++)
            {
                if(map->map[j][k] == 1) //STEADY points
                {
                    diff = p0[z++]; 
                    pre = front_ltime(front, j, k);
                    *pval = DO_XOR4(*pdiff, *ppre);

                    //TODO:RR nothing to do with steady points
                    //val = p0[z++];
                }
                else
                {
                    pv[0] = zouts[0][d];
                    pv[1] = zouts[1][d];
                    pv[2] = zouts[2][d];
                    pv[3] = zouts[3][d];
                    d ++;
                }

                buf[n++] = val;
                front_push(front, j, k, val);
            }
    }

    return;
}

int compress_2(FILE *fin, 
        ctx_t *ctx,
        front_t *front,
        map_t *map, FILE *fout)
{
    int i,j;
    int base, num, chk, chk0, chk1;
    nz_header hd;
#ifdef _OUTPUT_ZIP_
    char bhd[5*HDR_SIZE] = {0};
#endif
    float *buf;

    mzip_t zips[5];
    char *zins[5], *zouts[5];
    int32_t zlens[5];

    int flag; // adapt: select the compressor

    double begin, zbegin; //for timing

    begin = now_sec();
    chk = get_chk(map->dx, map->dy);
    base = map->dx * map->dy;
    chk0 = map->zcnt * (chk / base);
    chk1 = chk - chk0;

    buf = (float*)malloc(sizeof(float) * chk);
    if(buf == NULL)
    {
        fprintf(stderr, "fail to alloc mem\n");
        return -1;
    }

    init_zips(zips, chk1);
    mzip_init(&zips[4], chk0*4, ZLIB_DEF, ZIP_FAST); 
    for(j = 0; j < 5; j++)
        zins[j] = zips[j].zin;


    /* begin to write zip file header */
    nz_header_init(&hd, 2);
    hd.chk = chk;
    hd.map = map;
    for(j = 0; j < 5; j++)
        hd.ztypes[j] = zips[j].ztype;

    ctx->zipTime += (now_sec() - begin);
    hd.fsz = fsize_fp(fin);

    //nz_header_write(fout, &hd);
    /* finish zip file header */

    flag = 0;
    num = fread(buf, sizeof(uint32_t), chk, fin);
    while(num > 0)
    {
        i = num/base;
        //printf("[%d]:%d, %d\n", i, num, chk);

        begin = now_sec();
        _convert2(buf, i, front, map, zins);
        //ctx->time += (now_sec() - begin);

        for(j = 0; j < 4; j++)
            zips[j].inlen = i * (base - map->zcnt);
        zips[4].inlen = i*map->zcnt *4;
        ctx->zipTime += (now_sec() - begin);

        //adapt: select the compressor
        if(flag == 0)
        {
            begin = now_sec();
            for(j = 0; j < ADAPT_NUM; j++)
            {
                mzip_def_test0(&(zips[j]));
                hd.ztypes[j] = zips[j].ztype;
            }

            ctx->zipTime += (now_sec() - begin);
#ifdef _OUTPUT_ZIP_ 
            nz_header_write(fout, &hd);
#endif
            flag = 1;
        }
        //end of select compressors

        begin = now_sec();
        for(j = 0; j < 5; j++)
        {
            zbegin = now_sec();
            zips[j].zipfun(&(zips[j]), &zouts[j], &zlens[j]);
            zips[j].time1 += (now_sec() - zbegin);
        }
        ctx->zipTime += (now_sec() - begin);

#ifdef _OUTPUT_ZIP_
        for(j = 0; j < 5; j++)
            memcpy(bhd+j*HDR_SIZE, zouts[j], HDR_SIZE);
        fwrite(bhd, 1, 5*HDR_SIZE, fout);

        for(j = 0; j < 5; j++)
            fwrite(zouts[j]+HDR_SIZE, 1, zlens[j] - HDR_SIZE, fout);
#endif

        num = fread(buf, sizeof(float), chk, fin);
    }

    free(buf);

#ifdef _PRINT_ZIPS_
    print_results(zips, 5);
#endif

    for(j = 0; j < 5; j++)
        ctx->zfsz += zips[j].zfsz;

    for(j = 0; j < 5; j++)
        mzip_term(&zips[j]);
    return 0;
}

int uncompress_2(FILE *fin,
        ctx_t *ctx,
        nz_header *hd,
        FILE *fout)
{
    uint32_t i, j, num;
    uint32_t base, chk, chk0, chk1;
    uint64_t fsz;
    float *buf;
    front_t front;

    mzip_t zips[5];    /* 4 float zips, 1 zero zip */
    char *outs[5];
    map_t *map = hd->map;

    double begin;   // Timing

    begin = now_sec();

    fsz = hd->fsz/4;
    chk = hd->chk;
    base = map->dx * map->dy;
    chk0 = (chk/base) * map->zcnt;
    chk1 = chk - chk0;

    buf = (float*)malloc(sizeof(float) * chk);
    front_init(&front, map->dx, map->dy);

    mzip_init(&(zips[4]), EWIDTH*chk0, 
            hd->ztypes[4] + 1, 0);
    for(j = 0; j < 4; j++)
        mzip_init(&(zips[j]), chk1, hd->ztypes[j] + 1, 0);

    ctx->unzipTime += (now_sec() - begin);

    num = fsz/chk;
    for(i = 0; i < num; i ++)
    {
        _unzip_3(fin, zips, outs, chk1, chk0);

        begin = now_sec();
        _unconvert2(buf, chk/base, &front, map, outs);
        ctx->unzipTime += (now_sec() - begin);


#ifdef _OUTPUT_UNZIP_
        fwrite(buf, sizeof(float), chk, fout);
#endif
    }

    chk = fsz % chk;
    chk0 = (chk/base) * map->zcnt;
    chk1 = chk - chk0;
    if(chk > 0)
    {
        _unzip_3(fin, zips, outs, chk1, chk0);

        begin = now_sec();
        _unconvert2(buf, chk/base, &front, map, outs);
        ctx->unzipTime += (now_sec() - begin);

#ifdef _OUTPUT_UNZIP_
        fwrite(buf, sizeof(float), chk, fout);
#endif
    }

    front_term(&front);
    free(buf);

#ifdef _PRINT_ZIPS_
    print_results(zips, 5);
#endif

    for(j = 0; j < 5; j++)
    {
        ctx->fsz += zips[j].fsz;
        ctx->zfsz += zips[j].zfsz;
        ctx->unzipTime += zips[j].time2;

        mzip_term(&(zips[j]));
    }

    return 0;
}

/* ---------for (un)compress 1--------------- */
/* --- with predition, no steady points --- */

void _convert1(float *buf, int num, 
        front_t *front, 
        map_t *map,
        char *zins[])
{
    int i, j, k;
    int n, d, type;
    float val, pre, diff;
    uint32_t *pval, *ppre, *pdiff;

    char *pd = (char*)(&diff);

    n = 0;
    d = 0;
    type = map->type;

    pval = (uint32_t*)&val;
    ppre = (uint32_t*)&pre;
    pdiff = (uint32_t*)&diff;

    for(i = 0; i < num; i ++)
    {
        front_switch(front);
        for(j = 0; j < map->dy; j++)
            for(k = 0; k < map->dx; k ++)
            {
                val = buf[n++];

                if(type == LORENZO)
                    pre = front_lorenzo(front, j, k);
                else if(type == LTIME)
                    pre = front_ltime(front, j, k);
                else
                    pre = front_lpos(front, j, k);

                *pdiff = DO_XOR3(*pval, *ppre);

                zins[0][d] = pd[0];
                zins[1][d] = pd[1];
                zins[2][d] = pd[2];
                zins[3][d] = pd[3];
                d ++;

                front_push(front, j, k, val);
            }
    }

    return;
}

void _unconvert1(float *buf,
        int num,
        front_t *front,
        map_t *map,
        char *zouts[])
{
    int i, j, k;
    int n, d, type;
    float val, pre, diff;
    uint32_t *pval, *ppre, *pdiff;

    char *pd = (char*)(&diff);

    n = 0;
    d = 0;
    type = map->type;

    pval = (uint32_t*)&val;
    ppre = (uint32_t*)&pre;
    pdiff = (uint32_t*)&diff;

    for(i = 0; i < num; i ++)
    {
        front_switch(front);
        for(j = 0; j < map->dy; j++)
            for(k = 0; k < map->dx; k ++)
            {
                if(type == LORENZO)
                    pre = front_lorenzo(front, j, k);
                else if(type == LTIME)
                    pre = front_ltime(front, j, k);
                else
                    pre = front_lpos(front, j, k);

                pd[0] = zouts[0][d];
                pd[1] = zouts[1][d];
                pd[2] = zouts[2][d];
                pd[3] = zouts[3][d];
                d ++;

                *pval = DO_XOR3(*pdiff, *ppre);

                buf[n++] = val;
                front_push(front, j, k, val);
            }
    }

    return;
}

/* --- with predition, no steady points --- */
int compress_1(FILE *fin, 
        ctx_t *ctx,
        front_t *front,
        map_t *map, FILE *fout)
{
    int i,j;
    int base, num, chk;
    nz_header hd;
#ifdef _OUTPUT_ZIP_
    char bhd[4*HDR_SIZE] = {0};
#endif
    float *buf;

    mzip_t zips[4];
    char *zins[4], *zouts[4];
    int32_t zlens[4];

    int flag; // adapt: select the compressor

    double begin, zbegin; //for timing

    chk = get_chk(map->dx, map->dy);
    base = map->dx * map->dy;

    begin = now_sec();
    buf = (float*)malloc(sizeof(float) * chk);
    if(buf == NULL)
    {
        fprintf(stderr, "fail to alloc mem\n");
        return -1;
    }

    init_zips(zips, chk);
    for(j = 0; j < 4; j++)
        zins[j] = zips[j].zin;


    /* begin to write zip file header */
    nz_header_init(&hd, 1);
    hd.chk = chk;
    hd.map = map;
    for(j = 0; j < 4; j++)
        hd.ztypes[j] = zips[j].ztype;

    ctx->zipTime += (now_sec() - begin);
    hd.fsz = fsize_fp(fin);

    //nz_header_write(fout, &hd);
    /* finish zip file header */

    flag = 0;
    num = fread(buf, sizeof(uint32_t), chk, fin);
    while(num > 0)
    {
        i = num/base;
        //printf("[%d]:%d\n", i, num);

        begin = now_sec();
        _convert1(buf, i, front, map, zins);
        ctx->zipTime += (now_sec() - begin);

        //adapt: select the compressor
        if(flag == 0)
        {
            begin = now_sec();
            for(j = 0; j < ADAPT_NUM; j++)
            {
                mzip_def_test0(&(zips[j]));
                hd.ztypes[j] = zips[j].ztype;
            }

            ctx->zipTime += (now_sec() - begin);
#ifdef _OUTPUT_ZIP_ 
            nz_header_write(fout, &hd);
#endif
            flag = 1;
        }
        //end of select compressors

        begin = now_sec();
        for(j = 0; j < 4; j++)
        {
            zbegin = now_sec();
            zips[j].inlen = num;
            zips[j].zipfun(&(zips[j]), &zouts[j], &zlens[j]);
            zips[j].time1 += (now_sec() - zbegin);
        }
        ctx->zipTime += (now_sec() - begin);

#ifdef _OUTPUT_ZIP_
        for(j = 0; j < 4; j++)
            memcpy(bhd+j*HDR_SIZE, zouts[j], HDR_SIZE);
        fwrite(bhd, 1, 4*HDR_SIZE, fout);

        for(j = 0; j < 4; j++)
            fwrite(zouts[j]+HDR_SIZE, 1, zlens[j] - HDR_SIZE, fout);
#endif

        num = fread(buf, sizeof(float), chk, fin);
    }

    free(buf);

#ifdef _PRINT_ZIPS_
    print_results(zips, 4);
#endif

    for(j = 0; j < 4; j++)
        ctx->zfsz += zips[j].zfsz;

    for(j = 0; j < 4; j++)
        mzip_term(&zips[j]);
    return 0;
}


int uncompress_1(FILE *fin,
        ctx_t *ctx,
        nz_header *hd,
        FILE *fout)
{
    uint32_t i, j, num;
    uint32_t base, chk;
    uint64_t fsz;
    float *buf;
    front_t front;

    mzip_t zips[4];    /* 4 float zips, 1 zero zip */
    char *outs[4];
    map_t *map = hd->map;

    double begin;  // Timing

    fsz = hd->fsz/4;
    chk = hd->chk;
    base = map->dx * map->dy;

    begin = now_sec();

    buf = (float*)malloc(sizeof(float) * chk);
    front_init(&front, map->dx, map->dy);

    for(j = 0; j < 4; j++)
        mzip_init(&(zips[j]), chk, hd->ztypes[j] + 1, 0);

    ctx->unzipTime += (now_sec() - begin);

    num = fsz/chk;
    for(i = 0; i < num; i ++)
    {
        _unzip_1(fin, zips, outs, chk);

        begin = now_sec();
        _unconvert1(buf, chk/base, &front, map, outs);
        ctx->unzipTime += (now_sec() - begin);

#ifdef _OUTPUT_UNZIP_
        fwrite(buf, sizeof(float), chk, fout);
#endif
    }

    chk = fsz % chk;
    if(chk > 0)
    {
        _unzip_1(fin, zips, outs, chk);

        begin = now_sec();
        _unconvert1(buf, chk/base, &front, map, outs);
        ctx->unzipTime += (now_sec() - begin);

#ifdef _OUTPUT_UNZIP_
        fwrite(buf, sizeof(float), chk, fout);
#endif
    }

    front_term(&front);
    free(buf);

#ifdef _PRINT_ZIPS_
    print_results(zips, 4);
#endif

    for(j = 0; j < 4; j++)
    {
        ctx->fsz += zips[j].fsz;
        ctx->zfsz += zips[j].zfsz;
        ctx->unzipTime += zips[j].time2;

        mzip_term(&(zips[j]));
    }

    return 0;
}


/* ------- for (un)compress_0  -----------*/
/* --- no predition, no steady points --- */
void _convert0(const float *buf,
        int num,
        char *zins[])
{
    int i;
    char *p0, *p1, *p2, *p3;
    char *p = (char*)buf;

    p0 = zins[0];
    p1 = zins[1];
    p2 = zins[2];
    p3 = zins[3];
    for(i = 0; i < num; i ++)
    {
        *p0++ = *p++;
        *p1++ = *p++;
        *p2++ = *p++;
        *p3++ = *p++;
    }
}

void splitFloats(const float *buf, int num, char *zins[], const int compressPrecision)
{
    int i;
    char *p0, *p1, *p2, *p3;
    char *p = (char*)buf;

    p0 = zins[0];
    p1 = zins[1];
    p2 = zins[2];
    p3 = zins[3];
    for(i = 0; i < num; i ++)
    {
        switch(compressPrecision)
        {
            case 0:
                *p0++ = *p++;
                *p1++ = *p++;
                *p2++ = *p++;
                *p3++ = *p++;
                break;
            case 1:
                *p0 = '\0';
                p0 ++;
                p ++;
                *p1++ = *p++;
                *p2++ = *p++;
                *p3++ = *p++;
                break;
            case 2:
                *p0 = '\0';
                p0 ++;
                p  ++;
                *p1 = '\0';
                p ++;
                *p2++ = *p++;
                *p3++ = *p++;
                break;
            case 3:
                *p0 = '\0';
                p0 ++;
                p  ++;
                *p1 = '\0';
                p ++;
                *p2 = '\0';
                p ++;
                *p3++ = *p++;
                break;
            case 4:
                *p0 = '\0';
                p0 ++;
                p  ++;
                *p1 = '\0';
                p ++;
                *p2 = '\0';
                p ++;
                *p3 = '\0';
                p ++;
                break;
            default:
                *p0++ = *p++;
                *p1++ = *p++;
                *p2++ = *p++;
                *p3++ = *p++;
                break;
        }
    }
}
void _unconvert0(float *buf,
        int num,
        char *zouts[])
{
    int i;
    char *p0, *p1, *p2, *p3;
    char *p = (char*)buf;

    p0 = zouts[0];
    p1 = zouts[1];
    p2 = zouts[2];
    p3 = zouts[3];
    for(i = 0; i < num; i ++)
    {
        *p++ = *p0++;
        *p++ = *p1++;
        *p++ = *p2++;
        *p++ = *p3++;
    }
    return;
}


int runDecompression(FILE *fin,
        ctx_t *ctx,
        nz_header *hd,
        FILE *fout)
{
    uint32_t i, j, num;
    uint32_t chk;
    uint64_t fsz;
    float *buf;

    mzip_t zips[4];    /* 4 float zips, 1 zero zip */
    char *outs[4];

    double begin; // Timing

    fsz = hd->fsz/4;
    chk = hd->chk;

    begin = now_sec();

    buf = (float*)malloc(sizeof(float) * chk);

    for(j = 0; j < 4; j++)
        mzip_init(&(zips[j]), chk, hd->ztypes[j] + 1, 0);

    ctx->unzipTime += (now_sec() - begin);

    num = fsz/chk;
    for(i = 0; i < num; i ++)
    {
        _unzip_1(fin, zips, outs, chk);
        _unconvert0(buf, chk, outs);

#ifdef _OUTPUT_UNZIP_
        fwrite(buf, sizeof(float), chk, fout);
#endif
    }

    chk = fsz % chk;
    if(chk > 0)
    {
        _unzip_1(fin, zips, outs, chk);

        begin = now_sec();
        _unconvert0(buf, chk, outs);
        ctx->unzipTime += (now_sec() - begin);

#ifdef _OUTPUT_UNZIP_
        fwrite(buf, sizeof(float), chk, fout);
#endif
    }

    free(buf);


#ifdef _PRINT_ZIPS_
    displayResults(zips, 4, "Decompress");
#endif
    for(j = 0; j < 4; j++)
    {
        ctx->fsz += zips[j].fsz;
        ctx->zfsz += zips[j].zfsz;
        ctx->unzipTime += zips[j].time2;
        mzip_term(&(zips[j]));
    }

    return 0;
}


int runCompress(FILE *fin, ctx_t *ctx, FILE *fout, const int compressPrecision)
{
    int j;
    int num, chk;
    nz_header hd;
#ifdef _OUTPUT_ZIP_
    char bhd[4*HDR_SIZE] = {0};
#endif
    float *buf;

    mzip_t zips[4];
    char *zins[4], *zouts[4];
    int32_t zlens[4];

    double begin, zbegin; //for timing

    chk = CHUNK;

    begin = now_sec();
    buf = (float*)malloc(sizeof(float) * chk);
    if(buf == NULL)
    {
        fprintf(stderr, "[ERROR]: fail to alloc mem\n");
        exit(-1);
    }

    init_zips(zips, chk);
    for(j = 0; j < 4; j ++)
        zins[j] = zips[j].zin;


    /* begin to write zip file header */
    nz_header_init(&hd, 0);
    hd.chk = chk;
    hd.map = NULL;
    for(j = 0; j < 4; j++)
        hd.ztypes[j] = zips[j].ztype;

    ctx->zipTime += (now_sec() - begin);
    hd.fsz = fsize_fp(fin);


    num = fread(buf, sizeof(uint32_t), chk, fin);


    /**
     *  if we have  data need to be compressed, we will firstly write header into zip file, the zip file format is defined as following:
     *  8 bytes: original file size
     *  4 bytes: pre-defined chunk size
     *  1 byte: compressed type,like compress_0,compress_1, compress_2, compress_3
     *  5 bytes: compress method, like ZLIB_DEF, LZ4_DEF, LZ4HC_DEF，  for each byte stream
     *  N bytes compress data organized as compressed chunk list:
     *      each compressed chunk is organized as following:
     *          16 bytes chunk header: every 4 bytes indicate the length of each                    compressed byte stream
     *          4 compressed byte stream data: first compressed stream data is the first byte data in the incomming float number sequence, second compressed stream data is the second byte data in the incomming float number sequence, ...
     */
    if(num > 0)
    {
#ifdef _OUTPUT_ZIP_ 
        nz_header_write(fout, &hd);
#endif
    }

    while(num > 0)
    {

        begin = now_sec();
        /*
         *_convert0(buf, num, zins);
         */
        splitFloats(buf, num, zins, compressPrecision);
        ctx->zipTime += (now_sec() - begin);

        
        /**
         *  TODO:we can select the compress method for every byte stream here, for example:
         *  set the corresponding method value  in hd.ztypes[j]
         */

        //adapt: select the compressor
/*
 *        if(flag == 0)
 *        {
 *            begin = now_sec();
 *            for(j = 0; j < ADAPT_NUMZ; j++)
 *            {
 *                mzip_def_test0(&(zips[j]));
 *                hd.ztypes[j] = zips[j].ztype;
 *            }
 *
 *            ctx->time1 += (now_sec() - begin);
 *#ifdef _OUTPUT_ZIP_ 
 *            nz_header_write(fout, &hd);
 *#endif
 *            flag = 1;
 *        }
 */
        //end of select compressors

        begin = now_sec();
        for(j = 0; j < 4; j ++)
        {
            zbegin        = now_sec();
            zips[j].inlen = num;

            /**
             *  Here will invoke the mzlib_def() to compress the data 
             *  for each chunk;
             *    4 input  byte streams are pointed by zips[0..3];
             *    4 output compressed byte streams are pointed by zouts[0..3];
             *    the length of each compressed byte stream is stored in zlens[0..3]
             */
            zips[j].zipfun(&(zips[j]), &zouts[j], &zlens[j]);
            zips[j].time1 += (now_sec() - zbegin);
        }
        ctx->zipTime += (now_sec() - begin);

#ifdef _OUTPUT_ZIP_

        /**
         * The first 4 bytes data in zouts[j][0..3] is the length of each compressed byte stream, we extracted from zouts[0..3] respectly, and write the the header of each compress blocked  
         */
        for(j = 0; j < 4; j ++)
            memcpy(bhd+j * HDR_SIZE, zouts[j], HDR_SIZE);

        fwrite(bhd, 1, 4 * HDR_SIZE, fout);


        /**
         *  Write the compressed data of each chunk to the zip file, the start position of each compressed byte stream is from zouts[j] + HDR_SIZE, where HDR_SIZe = 4
         */
        for(j = 0; j < 4; j++)
            fwrite(zouts[j]+HDR_SIZE, 1, zlens[j] - HDR_SIZE, fout);
#endif

        num = fread(buf, sizeof(float), chk, fin);
    }

    free(buf);

#ifdef _PRINT_ZIPS_
    displayResults(zips, 4, "Compression");
#endif


    /**
     *  ctx->zfsz stored  the sum of the length of each compressed byte stream
     */
    for(j = 0; j < 4; j ++)
        ctx->zfsz += zips[j].zfsz;

    for(j = 0; j < 4; j ++)
        mzip_term(&zips[j]);
    return 0;
}


