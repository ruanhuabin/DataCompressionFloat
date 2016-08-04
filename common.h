/***********************************************
 Author: LT songbin
 Created Time: Tue 18 Jun 2013 05:01:00 PM CST
 File Name: common.h
 Description: 
 **********************************************/
#ifndef _ZIP_COMMON_H_
#define _ZIP_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif
///////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdint.h>

/* ------------- predictor types -----------------*/
typedef enum {LTIME=0, LPOS, LORENZO, 
              STEADY, UNSTEADY,
              PRE_COUNT} prediction_t; 
extern char *names[];

/* ------------------ map -------------------*/
typedef struct map_t{
  unsigned char **map;
  uint32_t dx;
  uint32_t dy;

  uint32_t zcnt;          //steady points count;
  char  type;             //LTIME, LPOS, or LORENZO
  float stat[PRE_COUNT];  // no use
}map_t;

int map_init(map_t *map, unsigned dx, unsigned dy);
void map_term(map_t *map);
void map_print(map_t *map);
void map_statis(map_t *map, float stat[]);
int map_write(FILE *fout, map_t *map);
int map_read(FILE *fin, map_t *map);

void map_convert(map_t *map);
char map_decision(map_t *map);

/* ------------------ front -------------------*/
typedef struct front_t{
  unsigned dx, dy;

  float **a0;
  float **a1;
}front_t;

void front_init(front_t *front, 
                unsigned dx, unsigned dy);
void front_term(front_t *front);
void front_reset(front_t *front);
void front_switch(front_t *front);
void front_push(front_t *front, unsigned y, unsigned x, float val);

/*  predictors  */
float front_2d(front_t *front, unsigned y, unsigned x);
/* last position: j-1 */
float front_lpos(front_t *front, unsigned y, unsigned x);
/* last time, same position */
float front_ltime(front_t *front, unsigned y, unsigned x);
/* lorenzo prediction */
float front_lorenzo(front_t *front, unsigned y, unsigned x);

/* ------------------ front -------------------*/
typedef struct _context_t {
  uint32_t fnum;
  uint64_t fsz;
  uint64_t zfsz;

  double time1; //zip ztime;
  double time2; //unzip uztime;
} ctx_t;

void ctx_init(ctx_t *ctx);
void ctx_reset(ctx_t *ctx);
void ctx_add(ctx_t *dst, ctx_t *src);
void ctx_print(ctx_t *ctx);
void ctx_print_more(ctx_t *ctx, const char *prompt);

/* ------------------ front -------------------*/
typedef struct _nz_header_t {
  uint64_t fsz;
  uint32_t chk;
  char type; //compress_0, 1, 2, 3
  char ztypes[5]; //four zips, zip0

  map_t *map;

} nz_header;

void nz_header_init(nz_header *hd, char type);
void nz_header_term(nz_header *hd);

void nz_header_print(nz_header *hd);
int nz_header_read(FILE *fin, nz_header *hd);
int nz_header_write(FILE *fout, nz_header *hd);

/* ------------------ others -------------------*/
/* v,p,m should be uint32_t */
#define DO_XOR(v,p,m) (v ^ (p & m))
#define DO_XOR4(v,p) (v ^ p)
#define DO_XOR3(v,p) (v ^ (p & 0xFFFFFF00))
#define DO_XOR2(v,p) (v ^ (p & 0xFFFF0000))
#define DO_XOR1(v,p) (v ^ (p & 0xFF000000))


void open_files(const char *fname, FILE* hdls[], int n);
void close_files(FILE* hdls[], int n);
unsigned float_xor(float real, float pred);
unsigned float_xor2(float real, float pred);
unsigned float_xor3(float real, float pred);
unsigned float_xor4(float real, float pred);

double now_sec();
uint64_t fsize_fp(FILE *fp);
#define TAG {printf("%s:%d\n", __FILE__,  __LINE__);}

#define _OUTPUT_ZIP_ (1)
#define _OUTPUT_UNZIP_ (1)
#define _PRINT_ZIPS_ (1)
#define _PRINT_DECISION_ (1)

///////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif

#endif //_ZIP_COMMON_H_
