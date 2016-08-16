#ifndef _ZIP_COMMON_H_
#define _ZIP_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif
///////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdint.h>
#include <nzips.h>


typedef struct _context_t
{
	uint32_t fileCount;
	uint64_t allFileSize;
	uint64_t allZipFileSize;

	double zipTime; //zip ztime;
	double unzipTime; //unzip uztime;
} ctx_t;

void init_context(ctx_t *ctx);
void reset_context(ctx_t *ctx);
void update_context(ctx_t *dst, ctx_t *src);
void print_ctx(ctx_t *ctx);
void ctx_print_more(ctx_t *ctx, const char *prompt);


typedef struct _mrczip_header_t
{
	uint64_t fsz;
	uint32_t chk;
	char type; //compress strategy
	char ztypes[4]; //compress method for each byte stream
} mrczip_header_t;

void init_mrczip_header(mrczip_header_t *hd, char type);
int write_mrczip_header(FILE *fout, mrczip_header_t *hd);
double now_sec();
uint64_t get_file_size(FILE *fp);

/**
 *  Following function is added by ruanhuabin
 */

/**
 *  Use to replace nz_header_read
 */
int read_mrczip_header(FILE *fin, mrczip_header_t *hd);

/**
 *  Use to replace nz_header_print
 */
void print_mrczip_header(mrczip_header_t *hd, const char *hintMsg);

/**
 *  Use to replace ctx_print
 */
void print_context_info(ctx_t *ctx, const char *hintMsg);
void print_result(mzip_t *zips, int n, const char *hintMsg);


#ifdef __cplusplus
}
#endif

#endif //_ZIP_COMMON_H_
