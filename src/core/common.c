
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include "common.h"

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
	struct timeval tm;
	gettimeofday(&tm, NULL);
	return (tm.tv_sec + (tm.tv_usec / 1000000.0));
}

void init_context(ctx_t *ctx)
{
	ctx->fileCount = 0;
	ctx->allFileSize = 0;
	ctx->allZipFileSize = 0;
	ctx->zipTime = 0.0;
	ctx->unzipTime = 0.0;
}

void reset_context(ctx_t *ctx)
{
	ctx->fileCount = 0;
	ctx->allFileSize = 0;
	ctx->allZipFileSize = 0;
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

	uint64_t fileSize = ctx->allFileSize;

	double speed = fileSize / (time * 1024.0 * 1024.0);

	const char * operation = zipTime > 0.001 ? "(zip)" : "(unzip)";

	char timeInfo[128];
	memset(timeInfo, '\0', sizeof(timeInfo));

	sprintf(timeInfo, "%.4f%s", time, operation);

	printf("%-*ld%-*ld%-*s%-*.*f\n", firstColumnLen, ctx->allFileSize, secondColumnLen,
			ctx->allZipFileSize, thirdColumnLen, timeInfo, fourthColumnLen, 4, speed);
}


void update_context(ctx_t *dst, ctx_t *src)
{
	dst->fileCount += src->fileCount;
	dst->allFileSize += src->allFileSize;
	dst->allZipFileSize += src->allZipFileSize;
	dst->zipTime += src->zipTime;
	dst->unzipTime += src->unzipTime;
}

void init_mrczip_header(mrczip_header_t *hd, char type)
{
	hd->type = type;
	hd->fsz = 0;
	hd->chk = 0;
	memset(hd->ztypes, 0, 4);
}


void print_mrczip_header(mrczip_header_t *hd, const char *hintMsg)
{

	printf("[%s]: Original file size = %ld, chunk size = %d, compresstion type = %d\n",
			hintMsg, hd->fsz, hd->chk, hd->type);
}

int read_mrczip_header(FILE *fin, mrczip_header_t *hd)
{
	if (fread(&(hd->fsz), sizeof(uint64_t), 1, fin) < 1)
	{
		fprintf(stderr, "[ERROR]:Failed to read file\n");
		return -1;
	}
	fread(&(hd->chk), sizeof(uint32_t), 1, fin);
	fread(&(hd->type), sizeof(char), 1, fin);
	for(int i = 0; i < 4; i++)
	{
		fread(&(hd->ztypes[i]), sizeof(char), 1, fin);
	}

	return 0;
}


int write_mrczip_header(FILE *fout, mrczip_header_t *hd)
{

	fwrite(&(hd->fsz), sizeof(uint64_t), 1, fout);
	fwrite(&(hd->chk), sizeof(uint32_t), 1, fout);
	fwrite(&(hd->type), sizeof(char), 1, fout);
	for(int i = 0; i < 4; i++)
	{
		fwrite(&(hd->ztypes[i]), sizeof(char), 1, fout);
	}

	return 0;
}
