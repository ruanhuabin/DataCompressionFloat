/***********************************************
 Author: LT songbin
 Created Time: Thu Jun 27 10:33:32 2013
 File Name: nzips.c
 Description: 
 **********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "zlib.h"
#include "lz4.h"
#include "lz4hc.h"
#include "nzips.h"

#include "common.h" //now_sec(), fsize_fp()

#define ZIP_WINDOW_BITS (-15)
//#define ZIP_STRATEGY Z_FILTERED 
//#define ZIP_STRATEGY Z_RLE
//#define ZIP_STRATEGY 0
#define LEVEL (6)

#ifndef TAG
#define TAG {printf("Error:%s %d\n", __FILE__, __LINE__);}
#endif
/*-------- lz4  ------- */

void _lz4_def(mzip_t *zip, char **p, int *len)
{  
  if(*len > 0)
  {
    *p = zip->out;
    pack_header(*p, COMPRESSED, *len);
    *len += HDR_SIZE;
  }
  else
  {
    *p = zip->in;
    pack_header(*p, RAW, zip->inlen);
    *len = zip->inlen + HDR_SIZE;
  }

  //TIP:for accounting
  zip->fsz += zip->inlen;
  zip->zfsz += *len;
}

int mlz4_def(mzip_t *zip, char **p, int *len)
{
  //*len is 0 if fails
  *len = LZ4_compress_limitedOutput(zip->zin,
                                    zip->zout,
                                    zip->inlen,
                                    zip->inlen);

  _lz4_def(zip, p, len);
  return 0;
}

int mlz4hc_def(mzip_t *zip, char **p, int *len)
{
  *len = LZ4_compressHC_limitedOutput(zip->zin,
                                    zip->zout,
                                    zip->inlen,
                                    zip->inlen);
  _lz4_def(zip, p, len);
  return 0;
}

int mlz4_inf(mzip_t *zip, 
             btype_t btype, int len, char **p)
{
  if(btype == RAW)
  {
    *p = zip->in;
  }
  else
  {
    LZ4_uncompress(zip->in, zip->out, len);
    *p = zip->out;
  }

  //TIP: for accounting
  zip->fsz += len;
  zip->zfsz += zip->inlen;
  return 0;
}

/*-------- zlib --------*/
int _mzlib_init(z_stream* strm)
{  
  //minor
  strm->zalloc = Z_NULL;
  strm->zfree = Z_NULL;
  strm->opaque = Z_NULL;
  strm->data_type = Z_BINARY; //no difference

  //the z_stream memory
  strm->next_in = NULL;
  strm->avail_in = 0;
  strm->total_in = 0;
  strm->next_out = NULL;
  strm->avail_out = 0;
  strm->total_out = 0;

  return 0;
}

int zlib_def_init(z_stream *strm, int level, int strategy)
{  
  int ret;
  int window, memlevel;

  //strategy = ZIP_STRATEGY;//Z_DEFAULT_STRATEGY, 
  //strategy = Z_DEFAULT_STRATEGY;//Z_DEFAULT_STRATEGY, 
  window = ZIP_WINDOW_BITS;
  memlevel = 9;
  ret = deflateInit2(strm, level, Z_DEFLATED, window, memlevel, strategy);
  if(ret != Z_OK)
  {
    fprintf(stderr, "zlib_init failed\n");
    return ret;
  }
  return 0;
}

z_stream* new_zlib_def(int level, int strategy)
{
  z_stream *strm = malloc(sizeof(z_stream));
  if(strm == NULL)
  {
    fprintf(stderr, "fail malloc\n");
    return NULL;
  }

  _mzlib_init(strm);
  zlib_def_init(strm, level, strategy);
  return strm;
}

z_stream* new_zlib_inf()
{
  int window, ret;
  z_stream *strm = malloc(sizeof(z_stream));
  if(strm == NULL)
  {
    fprintf(stderr, "fail malloc\n");
    return NULL;
  }

  _mzlib_init(strm);

  window = ZIP_WINDOW_BITS;
  ret = inflateInit2(strm, window);
  if(ret != Z_OK)
  {
    fprintf(stderr, "zlib_init failed\n");
    return NULL;
  }
  return strm;
}

int mzlib_def(mzip_t *zip, char **p, int *len)
{
  z_stream *strm = (z_stream*)zip->zipper;

  strm->next_in = (unsigned char*)zip->zin;
  strm->avail_in = (unsigned)zip->inlen;

  strm->next_out = (unsigned char*)zip->zout;
  strm->avail_out = zip->chk;

  deflate(strm, Z_FULL_FLUSH);

  *len = zip->chk - strm->avail_out;
  if(zip->inlen > *len+HDR_SIZE)
  {
    //compressable
    *p = zip->out;
    pack_header(*p, COMPRESSED, *len);
    *len += HDR_SIZE;
  }
  else //not compressable
  {
    *p = zip->in;
    pack_header(*p, RAW, zip->inlen);
    *len = zip->inlen + HDR_SIZE;
  }

  //TIP:for accounting
  zip->fsz += zip->inlen;
  zip->zfsz += *len;
  return 0;
}

void print_zerror(int ret)
{
  switch (ret)
  {
    case Z_NEED_DICT:
      fprintf(stderr, "Dict Error\n");
      break;
    case Z_DATA_ERROR:
      fprintf(stderr, "Data Error\n");
      break;
    case Z_MEM_ERROR:
      fprintf(stderr, "Mem Error\n");
      break;
    case Z_BUF_ERROR:
      fprintf(stderr, "Buf Error\n");
      break;
    default:
      fprintf(stderr, "unknown Error\n");
  }
}

int mzlib_inf(mzip_t *zip, btype_t btype, int len, char **p)
{
  if(btype == RAW)
  {
    *p = zip->in;
  }
  else
  {
    z_stream *strm = (z_stream*)zip->zipper;

    strm->next_in = (unsigned char*)(zip->in);
    strm->avail_in = (unsigned)zip->inlen;

    strm->next_out = (unsigned char*)(zip->out);
    strm->avail_out = zip->chk;

    inflate(strm, Z_FINISH);
    //if(len != (zip->chk - strm->avail_out))
    //{
    //  printf("len:%d, %d\n", len, (zip->chk - strm->avail_out));
    //  TAG;
    //}
    //if(ret != Z_OK)
    //{
    //  print_zerror(ret);
    //  fprintf(stderr, "ZLIB:%s\n", strm->msg);
    //  return -1;
    //}

    *p = zip->out;
    //printf("inlen, len, chk, avout: %d, %d, %d, %d\n",
    //        zip->inlen, *len, zip->chk, strm->avail_out);
  }

  //TIP: for accounting
  zip->fsz += len;
  zip->zfsz += zip->inlen;
  return 0;
}


/*-------- mzip --------*/

int mzip_init(mzip_t *zip, uint32_t chk, ztype_t ztype, int strategy)
{
  zip->ztype = ztype;
  switch (ztype)
  {
    case ZLIB_DEF:
      zip->zipper = new_zlib_def(LEVEL, strategy);
      zip->zipfun = mzlib_def;
      zip->unzipfun = NULL;
      break;
    case ZLIB_INF:
      zip->zipper = new_zlib_inf();
      zip->zipfun = NULL;
      zip->unzipfun = mzlib_inf;
      break;
    case LZ4_DEF:
    case LZ4_INF:
      zip->zipper = NULL;
      zip->zipfun = mlz4_def;
      zip->unzipfun = mlz4_inf;
      break;
    case LZ4HC_DEF:
    case LZ4HC_INF:
      zip->zipper = NULL;
      zip->zipfun = mlz4hc_def;
      zip->unzipfun = mlz4_inf;
      break;
    default:
      fprintf(stderr, "unkown zip type:%d\n", ztype);
      return -1;
  }

  if(chk >= MAX_BLOCK_SIZE)
  {
    fprintf(stderr, "too large chunk size\n");
    return -2;
  }

  zip->chk = chk;
  zip->inlen = 0;
  zip->outlen = 0;

  zip->in = malloc(HDR_SIZE+chk);
  zip->out = malloc(HDR_SIZE+chk);

  zip->zin = zip->in + HDR_SIZE;
  zip->zout = zip->out + HDR_SIZE;

  //accounting
  zip->fnum = 0;
  zip->fsz = 0;
  zip->zfsz = 0;
  zip->time1 = 0.0;
  zip->time2 = 0.0;
  return 0;
}

void mzip_term(mzip_t *zip)
{
  switch (zip->ztype)
  {
    case ZLIB_DEF:
      deflateEnd((z_stream*)zip->zipper);
      break;
    case ZLIB_INF:
      inflateEnd((z_stream*)zip->zipper);
      break;
    case LZ4_DEF:
    case LZ4_INF:
    case LZ4HC_DEF:
    case LZ4HC_INF:
      break;
    default:
      fprintf(stderr, "unkown zip type:%d\n", zip->ztype);
  }

  if(zip->zipper)
    free(zip->zipper);
  free(zip->in);
  free(zip->out);
}

int mzip_def(mzip_t *zip, char **p, int *len)
{
  return zip->zipfun(zip, p, len);
}

int mzip_inf(mzip_t *zip, btype_t btype, int len, char **p)
{
  return zip->unzipfun(zip, btype, len, p);
}


int mzip_def_test0(mzip_t *zip)
{ 
  int len;
  int flag = 0;
  if(zip->ztype != ZLIB_DEF)
  {
    fprintf(stderr, "wront type\n");
    TAG;
    return 0;
  }

  len =  LZ4_compress_limitedOutput(zip->zin,
                                   zip->zout,
                                   zip->inlen,
                                   zip->inlen);
  if(len < 1)
    flag = 1; 
  else if ((double)len / zip->inlen > 0.95)
    flag = 1;

  //printf("len:%d, %f\n", len, (double)(len+0.0001) / zip->inlen);
  if(flag == 1)
  {
    //TAG;
    //fprintf(stderr, "change to LZ4\n");
    deflateEnd((z_stream*)zip->zipper);
    free(zip->zipper);
    zip->zipper = NULL;

    zip->ztype = LZ4_DEF;
    zip->zipfun = mlz4_def;
    zip->unzipfun = mlz4_inf;
  }

  return flag;
}

void mzip_def_test1(mzip_t *zip)
{
  //dummy function, for debug only
  return ;
}

int mzip_def_test(mzip_t *zip, char **p, int *len)
{
  //int len;
  int flag = 0;
  if(zip->ztype != ZLIB_DEF)
  {
    fprintf(stderr, "wront type\n");
    TAG;
    return 1;
  }

  *len =  LZ4_compress_limitedOutput(zip->zin,
                                   zip->zout,
                                   zip->inlen,
                                   zip->inlen);
  if(*len < 1)
    flag = 1; 
  else if ((double)(*len) / zip->inlen > 0.90)
    flag = 1;

  if(flag == 1)
  {
    deflateEnd((z_stream*)zip->zipper);
    free(zip->zipper);
    zip->zipper = NULL;

    zip->ztype = LZ4_DEF;
    zip->zipfun = mlz4_def;
    zip->unzipfun = mlz4_inf;

    _lz4_def(zip, p, len);
  }
  else
   mzlib_def(zip, p, len);

  return 0;
}
/* ---------------- zip/unzip file  ------------ */
int write_fheader(FILE *fp, fheader_t *head)
{
  fwrite(&(head->fsize), sizeof(uint64_t), 1, fp);
  fwrite(&(head->chk), sizeof(uint32_t), 1, fp);
  fwrite(&(head->ztype), sizeof(char), 1, fp);
  //fwrite(&(head->level), sizeof(char), 1, fp);
  return 0;
}

int read_fheader(FILE *fp, fheader_t *head, char *bhead)
{
  fread(&(head->fsize), sizeof(uint64_t), 1, fp);
  fread(&(head->chk), sizeof(uint32_t), 1, fp);
  fread(&(head->ztype), sizeof(char), 1, fp);
  //fread(&(head->level), sizeof(char), 1, fp);
  if(bhead != NULL)
  {
    fread(bhead, 1, HDR_SIZE, fp);
  }
  return 0;
}

void assign_fheader(mzip_t *zip, fheader_t *fhd)
{
  fhd->chk = zip->chk;
  fhd->ztype = zip->ztype;
  //fhd->level = zip->level;
}

int check_fheader(mzip_t *zip, fheader_t *fhd)
{
  int flag = 0;

  if(fhd->chk != zip->chk)
  {
    fprintf(stderr, "chk: %d, %d\n", 
            fhd->chk, zip->chk);
    flag = 1;
  }
  
  if((fhd->ztype+1) != zip->ztype)
  {
    fprintf(stderr, "ztype:%d, %d\n",
            fhd->ztype, zip->ztype);
    flag = 2;
  }


  return flag;
}

int mzip_def_file(mzip_t *zip,
                  const char *src, 
                  const char *dst)
{
  FILE *fin, *fout;
  fheader_t fhd;
  char *p;
  int len;
  double begin;

  fin = fopen(src, "rb");
  if(fin == NULL)
  {
    fprintf(stderr, "fail open:%s\n", src);
    return -1;
  }
  fout = fopen(dst, "wb");

  fhd.fsize = fsize_fp(fin);
  assign_fheader(zip, &fhd);

  write_fheader(fout, &fhd);
  zip->inlen = fread(zip->zin, 1, zip->chk, fin);
  while(zip->inlen > 0)
  {
    begin = now_sec();
    zip->zipfun(zip, &p, &len);

    zip->time1 += (now_sec() - begin);

    fwrite(p, 1, len, fout);
    zip->inlen = fread(zip->zin, 1, zip->chk, fin);
  }

  zip->fnum += 1;

  fclose(fout);
  fclose(fin);

  return 0;
}

int mzip_inf_file(mzip_t *zip,
                  const char *src, 
                  const char *dst)
{
  FILE *fin, *fout;
  fheader_t fhd;
  uint32_t i, chk_num;

  btype_t btype;
  int len;
  char *p;
  double begin;

  fin = fopen(src, "rb");
  if(fin == NULL)
  {
    fprintf(stderr, "fail open:%s\n", src);
    return -1;
  }
  fout = fopen(dst, "wb");

  read_fheader(fin, &fhd, zip->in);
  check_fheader(zip, &fhd);
  zip->chk = fhd.chk;
  chk_num = fhd.fsize / fhd.chk;

  for(i=0, len=zip->chk; i<chk_num; i++)
  {
    unpack_header(zip->in, &btype, &(zip->inlen));
    fread(zip->in, 1, zip->inlen + HDR_SIZE, fin);

    begin = now_sec();
    zip->unzipfun(zip, btype, len, &p);
    memcpy(zip->in, zip->in + zip->inlen, HDR_SIZE);

    zip->time2 += (now_sec() - begin);

    fwrite(p, 1, len, fout);
  }

  //last additional chunk
  len = fhd.fsize % fhd.chk;
  if(len > 0)
  {
    unpack_header(zip->in, &btype, &zip->inlen);
    fread(zip->in, 1, zip->inlen + HDR_SIZE, fin);

    begin = now_sec();
    zip->unzipfun(zip, btype, len, &p);
    zip->time2 += (now_sec() - begin);

    fwrite(p, 1, len, fout);
  }

  fclose(fout);
  fclose(fin);
  return 0;
}

void split_float(const char *buf, int len, char *out[])
{
  int i;
  char *p0, *p1, *p2, *p3;

  p0 = out[0];
  p1 = out[1];
  p2 = out[2];
  p3 = out[3];

  for(i = 0; i < len; i ++)
  {
    *p0 ++ = *buf++;
    *p1 ++ = *buf++;
    *p2 ++ = *buf++;
    *p3 ++ = *buf++;
  }
}

void merge_float(char *buf, int len, char **in)
{
  int i;
  char *p0, *p1, *p2, *p3;

  p0 = in[0];
  p1 = in[1];
  p2 = in[2];
  p3 = in[3];

  for(i = 0; i < len; i ++)
  {
    *buf ++ = *p0 ++;
    *buf ++ = *p1 ++;
    *buf ++ = *p2 ++;
    *buf ++ = *p3 ++;
  }
}

int mzip_def_array(mzip_t zips[], 
                   const char *src, 
                   const char *dst)
{
  int j, chk, step;
  FILE *fin, *fout;
  char *buf;
  char *tmp[4];
  char *out[4];
  int  len[4];
  double begin;
  fheader_t fhd;

  fin = fopen(src, "rb");
  if(fin == NULL)
  {
    fprintf(stderr, "fail open:%s\n", src);
    return -1;
  }
  fout = fopen(dst, "wb");

  chk = zips[0].chk;
  buf = malloc(sizeof(float)*chk);

  fhd.fsize = fsize_fp(fin)/4;
  for(j = 0; j < 4; j ++)
  {
    assign_fheader(&zips[j], &fhd);
    write_fheader(fout, &fhd);

    tmp[j] = zips[j].zin;
  }

  step = fread(buf, 4, chk, fin);
  while(step > 0)
  {
    begin = now_sec();
    split_float(buf, step, tmp);

    for(j = 0; j < 4; j ++)
    {
      zips[j].inlen = step;
      zips[j].zipfun(&(zips[j]), &out[j], &len[j]);
    }

    zips[0].time1 += (now_sec() - begin);

    for(j = 0; j < 4; j ++)
    {
      fwrite(out[j], 1, len[j], fout);
    }

    step = fread(buf, 4, chk, fin);
  }

  free(buf);

  zips[0].fnum += 1;

  fclose(fin);
  fclose(fout);
  return 0;
}

int mzip_inf_array(mzip_t *zips, 
                   const char *src, const char *dst)
{
  int i, j, chk;
  FILE *fin, *fout;
  char *buf;
  char bhd[HDR_SIZE] = {0};
  char *out[4];
  btype_t btypes[4];
  int len, chk_num;
  double begin;
  fheader_t fhd;

  fin = fopen(src, "rb");
  if(fin == NULL)
  {
    fprintf(stderr, "fail open:%s\n", src);
    return -1;
  }
  fout = fopen(dst, "wb");

  chk = zips[0].chk;
  buf = malloc(sizeof(float)*chk);

  for(j = 0; j < 4; j ++)
  {
    read_fheader(fin, &fhd, NULL);
    check_fheader(&zips[j], &fhd);
  }

  chk_num = fhd.fsize / fhd.chk;
  for(i=0, len=chk; i<chk_num; i++)
  {
    for(j = 0; j < 4; j ++)
    {
      fread(bhd, 1, HDR_SIZE, fin);
      unpack_header(bhd, &(btypes[j]), &(zips[j].inlen));
      fread(zips[j].in, 1, zips[j].inlen, fin);
    }

    begin = now_sec();
    for(j = 0; j < 4; j ++)
    {
      zips[j].unzipfun(&(zips[j]), btypes[j], len, &out[j]);
    }
    merge_float(buf, len, out);
    zips[0].time2 += (now_sec() - begin);

    fwrite(buf, 4, len, fout);
  }

  len = fhd.fsize % fhd.chk;
  if(len > 0)
  {
    for(j = 0; j < 4; j ++)
    {
      fread(bhd, 1, HDR_SIZE, fin);
      unpack_header(bhd, &(btypes[j]), &(zips[j].inlen));
      fread(zips[j].in, 1, zips[j].inlen, fin);
    }

    begin = now_sec();
    for(j = 0; j < 4; j ++)
    {
      zips[j].unzipfun(&(zips[j]), btypes[j], len, &out[j]);
    }

    merge_float(buf, len, out);
    zips[0].time2 += (now_sec() - begin);

    fwrite(buf, 4, len, fout);
  }

  zips[0].fnum += 1;

  free(buf);
  fclose(fout);
  fclose(fin);
  return 0;
}

/*----------- for header  ------------------- */
void pack_header(char *nbuf, btype_t btype, uint32_t len)
{
  unsigned char *buf = (unsigned char*)nbuf;
  buf[0] = len & 0xFF;
  buf[1] = (len >> 8) & 0xFF;
  buf[2] = (len >> 16) & 0xFF;
  buf[3] = (len >> 24) & 0xFF;
  buf[3] |= (btype << 7);

  //unpack_header(buf, &btype, &len);
  //printf("btype:%d, len:%d\n", btype, len);
}

void unpack_header(const char *nbuf, btype_t *btype, uint32_t *len)
{
  unsigned char *buf = (unsigned char*)nbuf;
  *btype = (buf[3] & 0x80) >> 7;
  *len = buf[0] | (buf[1] << 8) | (buf[2] << 16) | ((buf[3] & 0x7f) << 24);

  //if(*len == 0)
  //  *len = MAX_BLOCK_SIZE;
}


void statis_ztime(mzip_t *zip, 
              uint32_t zchk, double begin, double end)
{
  zip->fsz += zip->inlen;
  zip->zfsz += zchk;
  zip->time1 += (end - begin);
}

void statis_uztime(mzip_t *zip, 
              uint64_t chk, double begin, double end)
{
  zip->fsz += chk;
  zip->zfsz += zip->inlen;
  zip->time2 += (end - begin);
}

void print_result(mzip_t *zip)
{
  double m = 1024*1024.0;
  //double m = 1.0;
  printf("----------------results----------------\n");
  printf("%d files, (MB)original:compressed: ratio\n",
         zip->fnum);
  printf("%.2f\t%.2f\t%f\n", zip->fsz/m, zip->zfsz/m,
                        (double)zip->zfsz/zip->fsz);
  if(zip->time1 > 0.001)
    printf("deflate speed: %f MB/s\n", 
            zip->fsz/(m *zip->time1));

  if(zip->time2 > 0.001)
    printf("inflate speed: %f MB/s\n",
            zip->fsz/(m*zip->time2));

  printf("---------------------------------------\n");
}

void print_results(mzip_t *zips, int n)
{
  int i;
  double m;
  uint64_t fsz, zfsz;
  double zipTime, unzipTime;

  fsz    = 0.0;
  zfsz   = 0.0;
  zipTime  = 0.0;
  unzipTime = 0.0;
  m      = 1024.0 * 1024.0;
  printf("-------------------Compress Information--------------\n");

  const char *firstColumnHeader = "[ByteStreamIndex]   ";
  const char *secondColumnHeader = "[Before Compress(Bytes)]   ";
  const char *thirdColumnHeader = "[After Compress(Bytes)]   ";
  const char *fourthColumnHeader = "[Compress Ratio]   ";
  printf("%s%s%s%s\n",firstColumnHeader, secondColumnHeader, thirdColumnHeader,fourthColumnHeader);

  int firstColumnLen = strlen(firstColumnHeader);
  int secondColumnLen = strlen(secondColumnHeader);
  int thirdColumnLen = strlen(thirdColumnHeader);
  int fourthColumnLen = strlen(fourthColumnHeader);
  double compressRatio = 0.0;

  /**
   *  Print compress ration for each byte stream
   */
  for(i = 0; i < n; i ++)
  {
    fsz       += zips[i].fsz;
    zfsz      += zips[i].zfsz;

    zipTime   += zips[i].time1;
    unzipTime += zips[i].time2;

    compressRatio = (double)(zips[i].zfsz) / (double)(zips[i].fsz);
    printf("%-*d%-*ld%-*ld%-*.*f\n", firstColumnLen, i, secondColumnLen, zips[i].fsz, thirdColumnLen, zips[i].zfsz, fourthColumnLen,4, compressRatio);
    /*
     *printf("[%d]: %.4f\n", i, (double)zips[i].zfsz/zips[i].fsz);
     */
  }

/*
 *  printf("%d files, (MB)original:compressed: ratio\n",
 *         zips[0].fnum);
 *  printf("%.2f\t%.2f\t%f\n", fsz/m, zfsz/m,
 *                        (double)zfsz/fsz);
 *
 */

  printf("%-*s", firstColumnLen, "Whole File");
  printf("%-*ld%-*ld%-*.*f\n", secondColumnLen, fsz, thirdColumnLen, zfsz, fourthColumnLen, 4, (double)(zfsz)/(double)(fsz));

  if(zipTime > 0.001)
  {
    printf("Compression speed: %f MB/s\n", fsz/(m *zipTime));
  }

  if(unzipTime > 0.001)
    printf("Decompression speed: %f MB/s\n", fsz/(m*unzipTime));

  printf("---------------------------------------\n");
  printf("zipTime = %f, unzipTime = %f, fsz = %ld, n = %d\n", zipTime, unzipTime,fsz, n);
}

void displayResults(mzip_t *zips, int n, const char *hintMsg)
{
  int i;
  double m;
  uint64_t fsz, zfsz;
  double zipTime, unzipTime;

  fsz    = 0.0;
  zfsz   = 0.0;
  zipTime  = 0.0;
  unzipTime = 0.0;
  m      = 1024.0 * 1024.0;
  printf("-------------------%s Information--------------\n", hintMsg);

  const char *firstColumnHeader = "[ByteStreamIndex]   ";
  const char *secondColumnHeader = "[Before Compress(Bytes)]   ";
  const char *thirdColumnHeader = "[After Compress(Bytes)]   ";
  const char *fourthColumnHeader = "[Compress Ratio]   ";
  printf("%s%s%s%s\n",firstColumnHeader, secondColumnHeader, thirdColumnHeader,fourthColumnHeader);

  int firstColumnLen = strlen(firstColumnHeader);
  int secondColumnLen = strlen(secondColumnHeader);
  int thirdColumnLen = strlen(thirdColumnHeader);
  int fourthColumnLen = strlen(fourthColumnHeader);
  double compressRatio = 0.0;

  /**
   *  Print compress ration for each byte stream
   */
  for(i = 0; i < n; i ++)
  {
    fsz       += zips[i].fsz;
    zfsz      += zips[i].zfsz;

    zipTime   += zips[i].time1;
    unzipTime += zips[i].time2;

    compressRatio = (double)(zips[i].zfsz) / (double)(zips[i].fsz);
    printf("%-*d%-*ld%-*ld%-*.*f\n", firstColumnLen, i, secondColumnLen, zips[i].fsz, thirdColumnLen, zips[i].zfsz, fourthColumnLen,4, compressRatio);
    /*
     *printf("[%d]: %.4f\n", i, (double)zips[i].zfsz/zips[i].fsz);
     */
  }

/*
 *  printf("%d files, (MB)original:compressed: ratio\n",
 *         zips[0].fnum);
 *  printf("%.2f\t%.2f\t%f\n", fsz/m, zfsz/m,
 *                        (double)zfsz/fsz);
 *
 */

  printf("%-*s", firstColumnLen, "Whole File");
  printf("%-*ld%-*ld%-*.*f\n", secondColumnLen, fsz, thirdColumnLen, zfsz, fourthColumnLen, 4, (double)(zfsz)/(double)(fsz));

  if(zipTime > 0.001)
  {
    printf("Compression speed: %f MB/s\n", fsz/(m *zipTime));
  }

  if(unzipTime > 0.001)
    printf("Decompression speed: %f MB/s\n", fsz/(m*unzipTime));

  printf("---------------------------------------\n");
  printf("%s Overall: zipTime = %f, unzipTime = %f, original file size = %ld, compressed file size = %ld, reduce = %.4f\n", hintMsg, zipTime, unzipTime,fsz, zfsz, 1.0 - (double)(zfsz)/(double)(fsz));

}


