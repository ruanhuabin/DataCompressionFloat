/***********************************************
Author: LT songbin
Created Time: Tue 18 Jun 2013 05:07:46 PM CST
File Name: common.c
Description: 
 **********************************************/
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>

#include "common.h"

char *names[] = {"ltime", "lpos", 
    "lorenzo", "steady", "unsteady"};

//#define do_xor(v,p,m) (v ^ (p & m))

/* ------------------ float xor -------------------*/
unsigned do_xor(float real, float pred, unsigned mask)
{  
    unsigned *p1, *p2;
    unsigned result;

    //printf("<%f, %f> \n", real, pred);
    p1 = (unsigned*)(&real);
    p2 = (unsigned*)(&pred);

    result = ((*p1) ^ (*p2)) & mask;
    result = ((*p1) & (~mask)) | result;

    return result;
}

unsigned float_xor(float real, float pred)
{
    unsigned mask;
    mask = 0xff000000;

    return do_xor(real, pred, mask);
}

unsigned float_xor2(float real, float pred)
{
    unsigned mask;
    mask = 0xffff0000;

    return do_xor(real, pred, mask);
}

unsigned float_xor3(float real, float pred)
{
    unsigned mask;
    mask = 0xffffff00;

    return do_xor(real, pred, mask);
}

unsigned float_xor4(float real, float pred)
{
    unsigned mask;
    mask = 0xffffffff;

    return do_xor(real, pred, mask);
}

void open_files(const char *fname, FILE* hdls[], int n)
{
    char tbuf[1024] = {0};
    char list[] = {'0', '1', '2', '3', '4', '5'};
    int i, len;

    len = strlen(fname);
    strcpy(tbuf, fname);
    for(i=0; i<n; i++)
    {
        tbuf[len] = list[i];
        tbuf[len+1] = '\0';
        hdls[i] = fopen(tbuf, "wb");
    }
}

void close_files(FILE* hdls[], int n)
{
    int i;
    for(i=0; i<n; i++)
        fclose(hdls[i]);
}

/* --------------------------- */
uint64_t fsize_fp(FILE *fp)
{
    if(fp == NULL)
        return -1;

    uint64_t tmp, sz;

    tmp = ftell(fp);
    fseek(fp, 0L, SEEK_END);

    sz = ftell(fp);
    fseek(fp, tmp, SEEK_SET);

    return sz;
}

double now_sec()
{
    struct timeval tim;
    gettimeofday(&tim, NULL);
    return (tim.tv_sec + (tim.tv_usec/1000000.0));
}


/* ------------------ ctx -------------------*/
void ctx_init(ctx_t *ctx)
{
    ctx->fnum = 0;
    ctx->fsz = 0;
    ctx->zfsz = 0;
    ctx->zipTime = 0.0;
    ctx->unzipTime = 0.0;
}

void ctx_reset(ctx_t *ctx)
{
    ctx->fnum = 0;
    ctx->fsz = 0;
    ctx->zfsz = 0;
    ctx->zipTime = 0.0;
    ctx->unzipTime = 0.0;
}

void displayContext(ctx_t *ctx, const char *hintMsg)
{
    const char *firstColumHeader = "[Original File Size(Bytes)]    ";
    const char *secondColumHeader = "[Compressed File Size(Bytes)]    ";
    const char *thirdColumHeader = "[Zip/Unzip Time(s)]    ";
    const char *fourthColumHeader = "[Speed(MB/s)]    ";

    int firstColumnLen = strlen(firstColumHeader);
    int secondColumnLen = strlen(secondColumHeader);
    int thirdColumnLen = strlen(thirdColumHeader);
    int fourthColumnLen = strlen(fourthColumHeader);

    printf("-------------------%s--------------------\n", hintMsg);
    printf("%s%s%s%s\n", firstColumHeader, secondColumHeader, thirdColumHeader, fourthColumHeader);

    double zipTime = ctx->zipTime;
    double unzipTime = ctx->unzipTime;
    double time = zipTime > 0.001 ? zipTime:unzipTime;
    
    uint64_t fileSize = ctx->fsz;

    double speed = fileSize / (time * 1024.0 * 1024.0);

    const char * operation = zipTime > 0.001 ? "(zip)" : "(unzip)";

    char timeInfo[128];
    memset(timeInfo, '\0', sizeof(timeInfo));

    sprintf(timeInfo, "%.4f%s",time, operation );

    printf("%-*ld%-*ld%-*s%-*.*f\n", firstColumnLen, ctx->fsz, secondColumnLen, ctx->zfsz, thirdColumnLen, timeInfo, fourthColumnLen, 4, speed);
}

void ctx_print(ctx_t *ctx)
{
    double m = 1024*1024.0;

    printf("%lu\t%lu\t%.4f",
            ctx->fsz, ctx->zfsz,
            (double)ctx->zfsz/ctx->fsz);

    if(ctx->zipTime > 0.001)
        printf("\t[%.2f", ctx->fsz/(m*ctx->zipTime));
    else
        printf("\t[%.2f", 0.0);

    if(ctx->unzipTime > 0.001)
        printf("\t%.2f]", ctx->fsz/(m*ctx->unzipTime));
    else
        printf("\t%.2f]", 0.0);

    printf(" MB/s\n");
}

void ctx_print_more(ctx_t *ctx, const char *key)
{
    printf("-------------------------\n");
    printf("[%s]:", key);
    ctx_print(ctx);
}

void ctx_add(ctx_t *dst, ctx_t *src)
{
    dst->fnum += src->fnum;
    dst->fsz += src->fsz;
    dst->zfsz += src->zfsz;
    dst->zipTime += src->zipTime;
    dst->unzipTime += src->unzipTime;
}

/* ------------------ map -------------------*/
int _map_alloc(map_t *map, unsigned dx, unsigned dy)
{
    unsigned i;
    map->map = (unsigned char**)malloc(dy * sizeof(unsigned char*));
    for(i=0; i<dy; i++)
    {
        map->map[i] = (unsigned char *)calloc(dx, sizeof(unsigned char));
    }

    return 0;
}

int map_init(map_t *map, unsigned dx, unsigned dy)
{
    int i;
    _map_alloc(map, dx, dy);

    map->dx = dx;
    map->dy = dy;

    map->zcnt = 0;
    map->type = LTIME;
    for(i=0; i<PRE_COUNT; i++)
        map->stat[i] = 0.0;

    return 0;
}

void map_print(map_t *map)
{
    unsigned i, j;
    printf("*********************\n");
    for(i=0; i < map->dy; i++)
        for(j=0; j < map->dx; j++)
            if(map->map[i][j] == 1)
                printf("%u,%u\n", i, j);
    printf("/*********************\n");
}

void map_term(map_t *map)
{
    unsigned i;
    for(i=0; i < map->dy; i++)
        free(map->map[i]);

    free(map->map);
}

void map_statis(map_t *map, float stat[])
{
    unsigned i, j, x, y;

    x = map->dx;
    y = map->dy;

    for(i=0; i<y; i++)
        for(j=0; j<x; j++)
        {
            stat[map->map[i][j]] += 1.0;
        }

    //count of steady points
    map->zcnt = (unsigned)(stat[STEADY]);

    for(i=0; i<UNSTEADY+1; i++)
    {
        stat[i] = stat[i]/(x*y);

#ifdef _PRINT_DECISION_
        printf("[%d]:%s,%f\n", __LINE__, names[i], stat[i]);
#endif
    }
}

int map_write(FILE *fout, map_t *map)
{
    unsigned i;
    fwrite(&(map->type), sizeof(char), 1, fout);
    fwrite(&(map->zcnt), sizeof(uint32_t), 1, fout);
    fwrite(&(map->dx), sizeof(uint32_t), 1, fout);
    fwrite(&(map->dy), sizeof(uint32_t), 1, fout);
    for(i=0; i < map->dy; i++)
        fwrite(map->map[i], sizeof(char), map->dx, fout);
    return 0;
}

int map_read(FILE *fin, map_t *map)
{
    unsigned i;
    fread(&(map->type), sizeof(char), 1, fin);
    fread(&(map->zcnt), sizeof(uint32_t), 1, fin);
    fread(&(map->dx), sizeof(uint32_t), 1, fin);
    fread(&(map->dy), sizeof(uint32_t), 1, fin);

    _map_alloc(map, map->dx, map->dy); 

    for(i=0; i < map->dy; i++)
        fread(map->map[i], sizeof(char), map->dx, fin);
    return 0;
}

/* 
 *  To write out the Map
 *
 * if map[i][j] == STEADY:
 *    map[i][j] == 1;
 * else
 *    map[i][j] == 0;
 *    */
void map_convert(map_t *map)
{
    unsigned i, j;

    for(i=0; i<map->dy; i++)
        for(j=0; j < map->dx; j++)
            if(map->map[i][j] != STEADY)
                map->map[i][j] = 0;
            else
                map->map[i][j] = 1;
}

/* return value {flag}:
 * 0: no prediction, no steady
 * 1: with prediction, no steady
 * 2: no prediction, with steady
 * 3: with prediction, with steady
 * */
char map_decision(map_t *map)
{
    int i;
    float p, stat[20] = {0};
    char flag = 0;

    map_statis(map, stat);

    i = LTIME;
    if(stat[i] < stat[LPOS])
        i = LPOS;
    if(stat[i] + 0.02 < stat[LORENZO])
        i = LORENZO;

    //worth prediction
    p = 1 - stat[STEADY] - stat[UNSTEADY];
    if((p >= 0.14) || ((p>0.8) && (p/(1.001 - stat[STEADY]) >= 0.25)))
        flag += 1;
    //if((1-stat[STEADY] - stat[UNSTEADY]) > 0.10)
    //  flag += 1;
    //if(stat[STEADY] < 0.9)
    //  if(stat[i]/(1-stat[STEADY]) > 0.20)
    //    flag += 1;

    //worth steady
    if(stat[STEADY] > 0.10)
        flag += 2;

    map->type = i;
    return flag;
}

/* ------------------ front -------------------*/
float ** alloc_mem(unsigned dx, unsigned dy)
{
    float **p;
    unsigned i;

    p = (float**)calloc(dy+1, sizeof(float*));
    for(i=0; i<dy+1; i++)
        p[i] = (float*)calloc(dx+1, sizeof(float));

    return p;
}

void del_mem(float **p, unsigned dy)
{
    unsigned i;
    for(i=0; i<dy+1; i++)
        free(p[i]);

    free(p);
}

void reset_mem(float **p, unsigned dx, unsigned dy)
{
    unsigned i;
    for(i=0; i<dy+1; i++)
        memset(p[i], 0, (dx+1)*sizeof(float));
}


void front_init(front_t *front, unsigned dx, unsigned dy)
{
    front->a0 = alloc_mem(dx, dy);
    front->a1 = alloc_mem(dx, dy);

    front->dx = dx;
    front->dy = dy;
}

void front_term(front_t *front)
{
    del_mem(front->a0, front->dy);
    del_mem(front->a1, front->dy);
}

void front_reset(front_t *front)
{
    reset_mem(front->a0, front->dx, front->dy);
    reset_mem(front->a1, front->dx, front->dy);
}

void front_switch(front_t *front)
{
    float **tmp;
    tmp = front->a0;
    front->a0 = front->a1;
    front->a1 = tmp;
}

void front_push(front_t *front, unsigned y, unsigned x, float val)
{
    front->a1[y+1][x+1] = val;
}

float front_2d(front_t *front, unsigned y, unsigned x)
{
    return front->a1[y+1][x] + 
        front->a0[y][x+1] - front->a0[y][x];
}

/* last position: j-1 */
float front_lpos(front_t *front, unsigned y, unsigned x)
{
    return front->a1[y+1][x];
}

/* last time, same position */
float front_ltime(front_t *front, unsigned y, unsigned x)
{
    return front->a0[y+1][x+1];
}

/* lorenzo prediction */
float front_lorenzo(front_t *front, unsigned y, unsigned x)
{
    float result;
    unsigned rx, ry;
    rx = x+1;
    ry = y+1;

    result = front->a0[ry][rx] - front->a0[ry][rx-1] + 
        front->a0[ry-1][rx-1] - front->a0[ry-1][rx];
    result += front->a1[ry][rx-1] + 
        front->a1[ry-1][rx] - front->a1[ry-1][rx-1];

    return result;
}


/* --------------- nz_file header ----------------*/
void nz_header_init(nz_header *hd, char type)
{
    hd->type = type;
    hd->fsz = 0;
    hd->chk = 0;
    hd->map = NULL;
    memset(hd->ztypes, 0, 5);
}

void nz_header_term(nz_header *hd)
{
}

void nz_header_print(nz_header *hd)
{
    printf("fsz:%lu\n", hd->fsz);
    printf("chk:%u\n", hd->chk);
    printf("type:%u\n", hd->type);

    if(hd->type != 0)
    {
        printf("Predition type:%s\n", 
                names[(int)(hd->map->type)]);
        printf("dx, dy, zcnt: %u, %u, %u\n", 
                hd->map->dx, hd->map->dy,
                hd->map->zcnt);
    }
}
void displayHeader(nz_header *hd, const char *hintMsg)
{

    printf("[%s]: Original file size = %ld, chunk size = %d, compresstion type = %d\n", hintMsg, hd->fsz, hd->chk, hd->type);
}

int readHeader(FILE *fin, nz_header *hd)
{
    int i;
    if(fread(&(hd->fsz), sizeof(uint64_t), 1, fin) < 1)
    {
        fprintf(stderr, "[ERROR]:Failed to read file\n");
        return -1;
    }
    fread(&(hd->chk), sizeof(uint32_t), 1, fin);
    fread(&(hd->type), sizeof(char), 1, fin);
    for(i=0; i<5; i ++)
    {
        fread(&(hd->ztypes[i]), sizeof(char), 1, fin);
    }

    return 0;
}

int nz_header_read(FILE *fin, nz_header *hd)
{
    int i;
    map_t *map = hd->map;

    if(fread(&(hd->fsz), sizeof(uint64_t), 1, fin) < 1)
    {
        TAG;
        fprintf(stderr, "[ERROR]:Failed to read file\n");
        return -1;
    }
    fread(&(hd->chk), sizeof(uint32_t), 1, fin);
    fread(&(hd->type), sizeof(char), 1, fin);
    for(i=0; i<5; i ++)
    {
        fread(&(hd->ztypes[i]), sizeof(char), 1, fin);
    }

    switch (hd->type)
    {
        case 0:
            /* no predition, no steady points */
            break;
        case 1:
            /*with predition, no steady points */
            fread(&(map->type), sizeof(char), 1, fin);
            fread(&(map->dx), sizeof(uint32_t), 1, fin); 
            fread(&(map->dy), sizeof(uint32_t), 1, fin); 
            break;
        case 2:
            /* no predition, with steady points */
        case 3:
            /* with predition, with steady points */
            map_read(fin, map);
            break;
        default:
            fprintf(stderr, "unknown type\n");
    }

    return 0;
}

int nz_header_write(FILE *fout, nz_header *hd)
{
    int i;
    map_t *map = hd->map;
    fwrite(&(hd->fsz), sizeof(uint64_t), 1, fout);
    fwrite(&(hd->chk), sizeof(uint32_t), 1, fout);
    fwrite(&(hd->type), sizeof(char), 1, fout);
    for(i=0; i<5; i ++)
    {
        fwrite(&(hd->ztypes[i]), sizeof(char), 1, fout);
    }

    switch (hd->type)
    {
        case 0:
            /* no predition, no steady points */
            break;
        case 1:
            /*with predition, no steady points */
            fwrite(&(map->type), sizeof(char), 1, fout);
            fwrite(&(map->dx), sizeof(uint32_t), 1, fout); 
            fwrite(&(map->dy), sizeof(uint32_t), 1, fout); 
            break;
        case 2:
            /* no predition, with steady points */
        case 3:
            /* with predition, with steady points */
            map_write(fout, map);
            break;
        default:
            fprintf(stderr, "unknown type\n");
    }

    return 0;
}
