#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>

#include "common.h"

char *names[] =
{ "ltime", "lpos", "lorenzo", "steady", "unsteady" };

uint64_t get_file_size(FILE *fp)
{
	if (fp == NULL)
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
	return (tim.tv_sec + (tim.tv_usec / 1000000.0));
}

void init_ctx(ctx_t *ctx)
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

void print_context_info(ctx_t *ctx, const char *hintMsg)
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
	printf("%s%s%s%s\n", firstColumHeader, secondColumHeader, thirdColumHeader,
			fourthColumHeader);

	double zipTime = ctx->zipTime;
	double unzipTime = ctx->unzipTime;
	double time = zipTime > 0.001 ? zipTime : unzipTime;

	uint64_t fileSize = ctx->fsz;

	double speed = fileSize / (time * 1024.0 * 1024.0);

	const char * operation = zipTime > 0.001 ? "(zip)" : "(unzip)";

	char timeInfo[128];
	memset(timeInfo, '\0', sizeof(timeInfo));

	sprintf(timeInfo, "%.4f%s", time, operation);

	printf("%-*ld%-*ld%-*s%-*.*f\n", firstColumnLen, ctx->fsz, secondColumnLen,
			ctx->zfsz, thirdColumnLen, timeInfo, fourthColumnLen, 4, speed);
}

void print_ctx(ctx_t *ctx)
{
	double m = 1024 * 1024.0;

	printf("%lu\t%lu\t%.4f", ctx->fsz, ctx->zfsz,
			(double) ctx->zfsz / ctx->fsz);

	if (ctx->zipTime > 0.001)
		printf("\t[%.2f", ctx->fsz / (m * ctx->zipTime));
	else
		printf("\t[%.2f", 0.0);

	if (ctx->unzipTime > 0.001)
		printf("\t%.2f]", ctx->fsz / (m * ctx->unzipTime));
	else
		printf("\t%.2f]", 0.0);

	printf(" MB/s\n");
}

void ctx_print_more(ctx_t *ctx, const char *key)
{
	printf("-------------------------\n");
	printf("[%s]:", key);
	print_ctx(ctx);
}

void update_context(ctx_t *dst, ctx_t *src)
{
	dst->fnum += src->fnum;
	dst->fsz += src->fsz;
	dst->zfsz += src->zfsz;
	dst->zipTime += src->zipTime;
	dst->unzipTime += src->unzipTime;
}

void init_mrczip_header(nz_header *hd, char type)
{
	hd->type = type;
	hd->fsz = 0;
	hd->chk = 0;
	memset(hd->ztypes, 0, 5);
}

void nz_header_term(nz_header *hd)
{
}

void print_mrczip_header(nz_header *hd, const char *hintMsg)
{

	printf(
			"[%s]: Original file size = %ld, chunk size = %d, compresstion type = %d\n",
			hintMsg, hd->fsz, hd->chk, hd->type);
}

int read_mrczip_header(FILE *fin, nz_header *hd)
{
	int i;
	if (fread(&(hd->fsz), sizeof(uint64_t), 1, fin) < 1)
	{
		fprintf(stderr, "[ERROR]:Failed to read file\n");
		return -1;
	}
	fread(&(hd->chk), sizeof(uint32_t), 1, fin);
	fread(&(hd->type), sizeof(char), 1, fin);
	for (i = 0; i < 5; i++)
	{
		fread(&(hd->ztypes[i]), sizeof(char), 1, fin);
	}

	return 0;
}

int nz_header_read(FILE *fin, nz_header *hd)
{
	int i;

	if (fread(&(hd->fsz), sizeof(uint64_t), 1, fin) < 1)
	{
		TAG
		;
		fprintf(stderr, "[ERROR]:Failed to read file\n");
		return -1;
	}
	fread(&(hd->chk), sizeof(uint32_t), 1, fin);
	fread(&(hd->type), sizeof(char), 1, fin);
	for (i = 0; i < 5; i++)
	{
		fread(&(hd->ztypes[i]), sizeof(char), 1, fin);
	}

	return 0;
}

int nz_header_write(FILE *fout, nz_header *hd)
{
	int i;
	fwrite(&(hd->fsz), sizeof(uint64_t), 1, fout);
	fwrite(&(hd->chk), sizeof(uint32_t), 1, fout);
	fwrite(&(hd->type), sizeof(char), 1, fout);
	for (i = 0; i < 5; i++)
	{
		fwrite(&(hd->ztypes[i]), sizeof(char), 1, fout);
	}

	return 0;
}
