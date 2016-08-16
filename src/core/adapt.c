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

int zip_compress(ctx_t *ctx, const char *src, const char *dst)
{
	FILE *fin, *fout;

	fin = fopen(src, "rb");
	fout = fopen(dst, "wb");

	if (fin == NULL)
	{
		fprintf(stderr, "[%s:%d] ERROR: fail open:%s\n",  __FILE__, __LINE__, src);
		exit(-1);
	}
	if(fout == NULL)
	{
		fprintf(stderr, "[%s:%d] ERROR: fail open:%s\n",  __FILE__, __LINE__, dst);
		exit(-1);
	}

	ctx->fileCount += 1;
	ctx->allFileSize += get_file_size(fin);

	run_compress(fin, ctx, fout, 0);

	fclose(fout);
	fclose(fin);
	return 0;
}

int zip_uncompress(ctx_t *ctx, const char *src, const char *dst)
{
	int ret;
	FILE *fin, *fout;
	mrczip_header_t hd;

	if (fin == NULL)
	{
		fprintf(stderr, "[%s:%d] ERROR: fail open:%s\n",  __FILE__, __LINE__, src);
		exit(-1);
	}
	if(fout == NULL)
	{
		fprintf(stderr, "[%s:%d] ERROR: fail open:%s\n",  __FILE__, __LINE__, dst);
		exit(-1);
	}

	ctx->fileCount += 1;

	init_mrczip_header(&hd, 0);

	ret = read_mrczip_header(fin, &hd);
	if (ret != 0)	{

		fclose(fout);
		fclose(fin);
		return -1;
	}

	run_uncompress(fin, ctx, &hd, fout);


	fclose(fout);
	fclose(fin);
	return 0;
}


int line_count(const char *fname)
{
	FILE *fp;
	int lines;
	char buf[1024];

	fp = fopen(fname, "r");
	if (fp == NULL)
	{
		fprintf(stderr, "fail open %s read\n", fname);
		return -1;
	}

	lines = 0;
	while (fgets(buf, 1024, fp) != NULL)
	{
		lines++;
	}

	fclose(fp);
	return lines;
}

int is_white(char *p)
{
	char ch = *p;
	if (ch == ' ' || ch == '\t' || ch == '\x0b')
		return 1;

	return 0;
}

int parse_line(char *buf, char **pname, int *len, uint32_t dims[])
{
	int num;
	char *p = buf;
	char *end = p + strlen(buf);

	while ((p != end) && is_white(p))
		p++;

	if (buf[0] == '#')
		return 0;

	*pname = p;
	while ((p != end) && !is_white(p))
		p++;

	*len = p - (*pname);

	num = sscanf(p, "%u %u %u %u", &(dims[0]), &(dims[1]), &(dims[2]),
			&(dims[3]));

	if (num < 3)
		return 0;

	if (num == 3)
		dims[3] = 1;

	return 1;
}

void fnames_print(fnames_t *fnames)
{
	int i;
	for (i = 0; i < fnames->size; i++)
	{
		printf("%s:%s\n", fnames->srcs[i], fnames->dsts[i]);
	}

}

int fnames_init(fnames_t *fnames, char *fname, char *prefix, char *suffix)
{
	int i, j, lines;
	char buf[1024] =
	{ 0 };

	FILE *fp;
	int len;
	char *pname;
	uint32_t dims[4] =
	{ 0 };

	lines = line_count(fname);

	fnames->idx = 0;
	fnames->size = 0;
	fnames->num = lines;

	fnames->srcs = (char**) malloc(lines * sizeof(char*));
	fnames->dsts = (char**) malloc(lines * sizeof(char*));


	fp = fopen(fname, "r");
	for (i = 0, j = 0; i < lines; i++)
	{
		fgets(buf, 1024, fp);
		if (parse_line(buf, &pname, &len, dims))
		{
			fnames->srcs[j] = (char*) malloc(len + 1);
			fnames->dsts[j] = (char*) malloc(256);


			memcpy(fnames->srcs[j], pname, len);
			sprintf(fnames->dsts[j], "%s%d.%s", prefix, j, suffix);

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
	for (i = 0; i < fnames->size; i++)
	{
		free(fnames->srcs[i]);
		free(fnames->dsts[i]);

	}

	free(fnames->srcs);
	free(fnames->dsts);


	pthread_mutex_destroy(&fnames->lock);
}

int fnames_next(fnames_t *fnames, int *idx)
{
	pthread_mutex_lock(&fnames->lock);
	if (fnames->idx < fnames->size)
	{
		*idx = fnames->idx;
		fnames->idx++;
	}
	else
	{
		*idx = -1;
	}

	printf("======>fnames->idx = %d, fnames->size = %d, idx = %d\n",
			fnames->idx, fnames->size, *idx);
	pthread_mutex_unlock(&fnames->lock);

	return *idx;
}

