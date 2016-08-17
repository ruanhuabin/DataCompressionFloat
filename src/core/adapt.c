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


int count_file_num(const char *fname)
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


void print_file_container_info(file_container_t *fnames)
{
	int i;
	for (i = 0; i < fnames->size; i++)
	{
		printf("%s:%s\n", fnames->srcs[i], fnames->dsts[i]);
	}

}

void erase_space(char *buffer)
{
	int len = strlen(buffer);
	for(int i = len - 1; i >= 0; i --)
	{
		if(buffer[i] == ' ' || buffer[i] == '\n' || buffer[i] == '\t')
		{
			buffer[i] = '\0';
		}
		else
		{
			break;
		}

	}
}

int init_file_container(file_container_t *fnames, char *file_list_descriptor, char *prefix, char *suffix)
{
	int i, j, lines;
	char buf[1024];

	FILE *fp;
	int len;
	char *pname;

	memset(buf, '\0', sizeof(buf));
	lines = count_file_num(file_list_descriptor);

	fnames->idx = 0;
	fnames->size = 0;
	fnames->fileNum = lines;

	fnames->srcs = (char**) malloc(lines * sizeof(char*));
	fnames->dsts = (char**) malloc(lines * sizeof(char*));


	fp = fopen(file_list_descriptor, "r");
	for (i = 0, j = 0; i < lines; i++)
	{
		fgets(buf, 1024, fp);
		erase_space(buf);
		pname = buf;
		len = strlen(buf);


		fnames->srcs[j] = (char*) malloc(len + 1);
		fnames->dsts[j] = (char*) malloc(512);


		memcpy(fnames->srcs[j], pname, len);
		sprintf(fnames->dsts[j], "%s%d.%s", prefix, j, suffix);

		j++;
	}

	fnames->size = j;
	fclose(fp);

	pthread_mutex_init(&fnames->lock, NULL);
	return 0;
}


void reverse(char *str)
{
    int i = 0;
    int j = strlen(str) - 1;

    char temp;
    while (i < j)
    {
      temp = str[i];
      str[i] = str[j];
      str[j] = temp;
      i++;
      j--;
    }
}
void extract_suffix(const char *input, char *suffix)
{
    int idx = 0;
    int len = strlen(input);
    for(int i = len - 1; i >= 0; i --)
    {
        if(input[i] != '.')
        {
            suffix[idx] = input[i];
            idx ++;

        }
        else
        {
            break;
        }

    }

    suffix[idx] = '\0';
    reverse(suffix);

}

void extract_pure_file_name(const char *input, char *pureFileName)
{
    int len = strlen(input);
    int index = 0;
    for(int i = len - 1; i >= 0; i --)
    {
        if(input[i] != '/')
        {
            pureFileName[index] = input[i];
            index ++;
        }
        else
        {
            break;
        }

    }

    pureFileName[index] = '\0';

    reverse(pureFileName);

    printf("pureFileName = %s\n", pureFileName);
}


void extract_prefix_suffix(const char *input, char *prefix, char *suffix)
{
    char pureFileName[128];
    memset(pureFileName, '\0', sizeof(pureFileName));

    extract_pure_file_name(input, pureFileName);
    extract_suffix(pureFileName, suffix);

    int plen = strlen(pureFileName);
    int slen = strlen(suffix);
    int i = 0;
    for(i = 0; i < plen - slen - 1; i ++)
    {
        prefix[i] = pureFileName[i];
    }
    prefix[i] = '\0';
}



int init_file_container_ex(file_container_t *fnames, const char *ifcFile, const char *outputDir, char *opType)
{
	int i, j, lines;
	char buf[1024];

	FILE *fp;
	int len;
	char *pname;

	memset(buf, '\0', sizeof(buf));
	lines = count_file_num(ifcFile);

	fnames->idx = 0;
	fnames->size = 0;
	fnames->fileNum = lines;

	fnames->srcs = (char**) malloc(lines * sizeof(char*));
	fnames->dsts = (char**) malloc(lines * sizeof(char*));

	char prefix[128];
	char suffix[128];
	memset(prefix, '\0', sizeof(prefix));
	memset(suffix, '\0', sizeof(suffix));

	fp = fopen(ifcFile, "r");
	for (i = 0, j = 0; i < lines; i++)
	{
		fgets(buf, 1024, fp);
		erase_space(buf);
		pname = buf;
		len = strlen(buf);

		fnames->srcs[j] = (char*) malloc(len + 1);
		fnames->dsts[j] = (char*) malloc(512);


		memcpy(fnames->srcs[j], pname, len);

		extract_prefix_suffix(pname, prefix, suffix);

		if(strcmp(suffix, "mrc") == 0)
		{
			sprintf(fnames->dsts[j], "%s/%s.%s.%s", outputDir, prefix, suffix, "zip");
		}
		else if(strcmp(suffix, "zip") == 0)
		{
			sprintf(fnames->dsts[j], "%s/%s", outputDir, prefix);
		}
		else
		{
			fprintf(stderr, "[%s:%d] Error: Only file with suffix [mrc | zip] can be processed\n", __FILE__, __LINE__);
			exit(-1);
		}

		j++;
	}

	fnames->size = j;
	fclose(fp);

	pthread_mutex_init(&fnames->lock, NULL);
	return 0;
}

void free_file_container(file_container_t *fnames)
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

int get_next_file(file_container_t *fnames, int *idx)
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

