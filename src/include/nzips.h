/***********************************************
 Author: LT songbin
 Created Time: Wed Jun 26 23:34:47 2013
 File Name: nzips.h
 Description: 
 **********************************************/
#ifndef _NZIPS_H_
#define _NZIPS_H_

#ifdef __cplusplus
extern "C" {
#endif
///////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdint.h>
#include "zlib.h"

/*block type */
typedef enum {COMPRESSED=0, RAW} btype_t;

/*compressor type */
typedef enum {ZLIB_DEF=0,   ZLIB_INF, 
              LZ4_DEF,      LZ4_INF, 
              LZ4HC_DEF,    LZ4HC_INF
             } ztype_t; 

typedef struct _mzip_t mzip_t;
typedef int(*zipfun_p)(mzip_t *zip, char **out, int *outlen);
typedef int(*zipfun_p_ex)(mzip_t *zip, char **out, int *outlen, int, FILE *);
typedef int(*unzipfun_p)(mzip_t *zip, btype_t, int outlen, char **out);

struct _mzip_t{
  uint32_t chk;
  uint32_t inlen;
  uint32_t outlen; //no use right now
  ztype_t ztype;

  void *zipper;

  char *in;
  char *out;
  char *zin;
  char *zout;

  zipfun_p zipfun;
  zipfun_p_ex zipfun_ex;
  unzipfun_p unzipfun;
 
  /*should move to somewhere else*/
  uint32_t fnum;
  uint64_t fsz;
  uint64_t zfsz;
  double time1; //zip
  double time2; //unzip
};

int mzip_init(mzip_t *zip, uint32_t chk, ztype_t ztype, int strategy);
void mzip_term(mzip_t *zip);

int mzip_def(mzip_t *zip, char **p, int *len);
int mzip_inf(mzip_t *zip, btype_t btype, int len, char **p);

int mzip_def_test0(mzip_t *zip);
void mzip_def_test1(mzip_t *zip);
int mzip_def_test(mzip_t *zip, char **p, int *len);

typedef struct _fheader_t {
  uint64_t fsize;         // original file size
  uint32_t chk;           // original chunk size
  char     ztype;         // compressor type:zlib(0), lz4(2), lz4hc(4)
} fheader_t;

int mzip_def_file(mzip_t *zip, const char *src, const char *dst);
int mzip_inf_file(mzip_t *zip, const char *src, const char *dst);
int mzip_def_array(mzip_t *zips, const char *src, const char *dst);
int mzip_inf_array(mzip_t *zips, const char *src, const char *dst);

int write_fheader(FILE *fp, fheader_t *head);
int read_fheader(FILE *fp, fheader_t *head, char *bhead);
void assign_fheader(mzip_t *zip, fheader_t *fhd);
int check_fheader(mzip_t *zip, fheader_t *fhd);

/* ------------ lz4 ------ */

int mlz4_def(mzip_t *zip, char **p, int *len);
int mlz4hc_def(mzip_t *zip, char **p, int *len);
int mlz4_inf(mzip_t *zip, btype_t btype, int len, char **p);

/* ------------ mzlib ---- */
int mzlib_def(mzip_t *zip, char **p, int *len);
int mzlib_inf(mzip_t *zip, btype_t btype, int len, char **p);

/* ------------ header information -----
 * Header size: 4 bytes
  one bit: block_type
  max 31 bit: block size;
  default block_size: BLOCK_SIZE
 */
#define BLOCK_BITS 31
#define MAX_BLOCK_SIZE (0x1 << BLOCK_BITS)
#define BLOCK_SIZE (0x1 << 26)
#define HDR_SIZE 4
#define BLOCK_MODE_MASK (0x1 << BLOCK_BITS)
#define BLOCK_SIZE_MASK 0x7FFFFFFF

void pack_header(char *buf, btype_t btype, uint32_t len);
void unpack_header(const char *buf, btype_t *btype, uint32_t *len);

void statis_ztime(mzip_t *zip, uint32_t zchk, double begin, double end);
void statis_time(mzip_t *zip, uint64_t chk, double begin, double end);

void print_result(mzip_t *zip);
void print_results(mzip_t *zips, int n);
void display_results(mzip_t *zips, int n, const char *hintMsg);

#define TAG {printf("%s:%d\n", __FILE__,  __LINE__);}

///////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif

#endif //_NZIPS_H_
