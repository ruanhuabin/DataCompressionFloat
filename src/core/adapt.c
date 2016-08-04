/***********************************************
 Author: LT songbin
 Created Time: Wed Jul 10 22:17:15 2013
 File Name: adapt.c
 Description: 
 **********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zlib.h"
#include "common.h"
#include "workers.h"
#include "adapt.h"

#define ZERO (0.0000002)
#define MINI_PRE (0.00195)
#define SUBF(a,b) (a>=b ? a-b:b-a)
//#define PSUBF(a,b) (SUBF(a,b)/(a+ZERO))
//#define EQUAL(a,b) (PSUBF(a,b) < ZERO)

#ifndef TAG
#define TAG {printf("%s:%d\n", __FILE__,  __LINE__);}
#endif

#define XOR_EQUAL(a,b) (((a)^(b)) < 1)
#define XOR_PEQUAL(a,b) (((a)^(b)) < (1<<16))
//#define XOR_PEQUAL(a,b) (!(((a)^(b)) & (0xFFFF0000)))
/*-----------------------------------------*/

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


int learn(FILE *fin,
          ctx_t *ctx, 
          front_t *front, 
          map_t *map, 
          unsigned dims[])
{
  unsigned i, j, k;
  unsigned n, step;
  int offset, len;
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
          unsigned dims[], FILE *fin, FILE *fout)
{
  char flag;
  int type;

  flag = map_decision(map);
  type = map->type;

  map_convert(map);
  //map_print(map);

  //TODO: debug only, remove it
  //flag = 0;

  printf("xxx\t%d\t%d\t%s\n", flag, type, __FILE__); 
  switch (flag)
  {
    case 0:
      //printf("no prediction, no steady\n");
      compress_0(fin, ctx, fout);
      break;
    case 1:
      //printf("with prediction:%s, but no steady\n", 
      //        names[type]);
      compress_1(fin, ctx, front, map, fout);
      break;
    case 2:
      //printf("no prediction, but with steady\n");
      compress_2(fin, ctx, front, map, fout);
      break;
    case 3:
      //printf("with prediction:%s, and with steady\n", 
      //       names[type]);
      compress_3(fin, ctx, front, map, fout);
      break;
    default:
      printf("wrong decision\n");
  }
  
  return 0;
}

int learn_compress(ctx_t *ctx,
                   const char *src, const char *dst,
                   unsigned dims[])
{
  FILE *fin, *fout;
  
  //ctx_t mctx;
  front_t front;
  map_t map;

  fin = fopen(src, "rb");
  fout = fopen(dst, "wb");
  if(fin == NULL || fout ==NULL)
  {
    TAG;
    if(fin == NULL)
      fprintf(stderr, "[ERROR]: fail open:%s\n", src);
    else
      fprintf(stderr, "[ERROR]: fail open:%s\n", dst);

    return -1;
  }

  //ctx_init(&mctx);
  if(dims[3] > 1)
  {
    dims[1] = dims[1] * dims[2];
    dims[2] = dims[3];
  }
  front_init(&front, dims[0], dims[1]);
  map_init(&map, dims[0], dims[1]);

  ctx->fnum += 1;
  ctx->fsz += fsize_fp(fin);
  learn(fin, ctx, &front, &map, dims);
  my_compress(ctx, &map, &front, dims, fin, fout);

  //mctx.fnum += 1;
  //mctx.fsz += fsize_fp(fin);
  //learn(fin, &mctx, &front, &map, dims);
  //my_compress(&mctx, &map, &front, dims, fin, dst);

  ////ctx_print(&mctx);
  //ctx_add(ctx, &mctx);

  map_term(&map);
  front_term(&front);
  fclose(fout);
  fclose(fin);
  return 0;
}

int nz_uncompress(ctx_t *ctx,
                  const char *src, const char *dst)
{
  int ret;
  FILE *fin, *fout;
  map_t map;
  nz_header hd;

  fin = fopen(src, "rb");
  fout = fopen(dst, "wb");
  if(fin == NULL || fout ==NULL)
  {
    TAG;
    fprintf(stderr, "[ERROR]: fail open:%s\n", src);
    return -1;
  }

  ctx->fnum += 1;

  nz_header_init(&hd, 0);
  hd.map = &map;
  ret = nz_header_read(fin, &hd);
  if(ret != 0)
  {
    nz_header_term(&hd);
    fclose(fout);
    fclose(fin);
    return -1;
  }

  //nz_header_print(&hd);
  
  //TODO: debug only, remove it
  //hd.type = 3;

  switch (hd.type)
  {
    case 0:
      printf("no prediction, no steady\n");
      uncompress_0(fin, ctx, &hd, fout);
      break;
    case 1:
      printf("with prediction:%s, but no steady\n", 
              names[(int)(map.type)]);
      uncompress_1(fin, ctx, &hd, fout);
      break;
    case 2:
      printf("no prediction, but with steady\n");
      uncompress_2(fin, ctx, &hd, fout);
      break;
    case 3:
      printf("with prediction:%s, and with steady\n", 
              names[(int)(map.type)]);
      uncompress_3(fin, ctx, &hd, fout);
      break;
  }

  ////ctx_print(ctx);
  nz_header_term(&hd);
  fclose(fout);
  fclose(fin);
  return 0;
}

//int main(int argc, char *argv[])
//{
//  printf("ZLIB:%s\n", zlib_version);
//  if(argc < 7)
//  {
//    printf("Usage: %s -oz/u <infile> <outfile> <nx> <ny> <nz> [nt]\n", argv[0]);
//    return -1;
//  }
//
//  unsigned dims[3] = {0};
//  dims[0] = atol(argv[4]);
//  if(argc > 7)
//  {
//    dims[1] = atol(argv[5]) * atol(argv[6]);
//    dims[2] = atol(argv[7]);
//  }
//  else
//  {
//    dims[1] = atol(argv[5]);
//    dims[2] = atol(argv[6]);
//  }
//
//  if(strcmp(argv[1], "-oz") == 0)
//    learn_compress(argv[2], argv[3], dims);
//  else
//    nz_uncompress(argv[2], argv[3]);
//  return 0;
//}
//

int line_count(const char *fname)
{
  FILE *fp;
  int lines;
  char buf[1024];

  fp = fopen(fname, "r");
  if(fp == NULL)
  {
    fprintf(stderr, "fail open %s read\n", fname);
    return -1;
  }

  lines = 0;
  while(fgets(buf, 1024, fp) != NULL)
  {
    lines ++;
  }

  fclose(fp);
  return lines;
}

int is_white(char *p)
{
  char ch = *p;
  if(ch == ' ' || ch == '\t' || ch == '\x0b')
    return 1;

  return 0;
}

int parse_line(char *buf, char **pname, int *len, uint32_t dims[])
{
  int num;
  char *p = buf;
  char *end = p + strlen(buf);

  while((p != end)  && is_white(p))
    p++;

  if(buf[0] == '#')
    return 0;

  *pname = p;
  while((p != end)  && !is_white(p))
    p++;

  *len = p - (*pname);

  num = sscanf(p, "%u %u %u %u", 
               &(dims[0]), &(dims[1]), &(dims[2]), &(dims[3]));

  if(num < 3)
    return 0;

  if(num == 3)
    dims[3] = 1;
  
  return 1;
}

void fnames_print(fnames_t *fnames)
{
  int i;
  for(i=0; i<fnames->size; i++)
  {
    printf("%s:%s:", fnames->srcs[i], fnames->dsts[i]);
    printf("%u %u %u %u\n", fnames->dims[i][0],
                         fnames->dims[i][1],
                         fnames->dims[i][2],
                         fnames->dims[i][3]);
  }

}

#define NAME_LEN (512)
int fnames_init(fnames_t *fnames, char *fname, 
                char *prefix, char *suffix)
{
  int i, j, lines;
  char buf[1024] = {0};

  FILE *fp;
  int len;
  char *pname;
  uint32_t dims[4] = {0};

  lines = line_count(fname);

  fnames->idx = 0;
  fnames->size = 0;
  fnames->num = lines;

  fnames->srcs = (char**)malloc(lines * sizeof(char*));
  fnames->dsts = (char**)malloc(lines * sizeof(char*));
  fnames->dims = (uint32_t**)malloc(lines * sizeof(uint32_t*));

  fp = fopen(fname, "r");
  for(i=0, j=0; i<lines; i++)
  {
    fgets(buf, 1024, fp);
    if(parse_line(buf, &pname, &len, dims))
    {
      fnames->srcs[j] = (char*)malloc(len+1);
      fnames->dsts[j] = (char*)malloc(256);
      fnames->dims[j] = (uint32_t*)malloc(sizeof(uint32_t) * 4);

      memcpy(fnames->srcs[j], pname, len); 
      sprintf(fnames->dsts[j], "%s%d.%s", prefix, j, suffix);

      //if(prefix[0] == '/')
      //  sprintf(fnames->dsts[j], "%s%d.%s", prefix, j, suffix);
      //else
      //  sprintf(fnames->dsts[j], "%s%d.%s", prefix, j, suffix);

      fnames->dims[j][0] = dims[0];
      fnames->dims[j][1] = dims[1];
      fnames->dims[j][2] = dims[2];
      fnames->dims[j][3] = dims[3];

      j++;
    }
  }

  fnames->size = j;
  fclose(fp);

  pthread_mutex_init(&fnames->lock, NULL);
  return 0;
}

void fnames_term(fnames_t *fnames)
{
  int i;
  for(i=0; i<fnames->size; i++)
  {
    free(fnames->srcs[i]);
    free(fnames->dsts[i]);
    free(fnames->dims[i]);
  }

  free(fnames->srcs);
  free(fnames->dsts);
  free(fnames->dims);

  pthread_mutex_destroy(&fnames->lock);
}

int fnames_next(fnames_t *fnames, int *idx)
{
  pthread_mutex_lock(&fnames->lock);
  if(fnames->idx < fnames->size)
  {
    *idx = fnames->idx;
    fnames->idx ++;
  }
  else
  {
    *idx = -1;
  }
  pthread_mutex_unlock(&fnames->lock);

  return *idx;
}


