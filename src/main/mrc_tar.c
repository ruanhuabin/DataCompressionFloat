/*******************************************************************
 *       Filename:  mrc_tar.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2016年08月04日 11时45分36秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ruan Huabin
 *          Email:  ruanhuabin@tsinghua.edu.cn
 *        Company:  Dep. of CS, Tsinghua Unversity
 *
 *******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zlib.h"
#include "common.h"
#include "workers.h"

int mrc_compress(const char *src, const char *dst, const int bitsToMask)
{
    FILE *fin = fopen(src, "rb");

    if (fin == NULL)
    {
        fprintf(stderr, "Error: [%s:%d]: Failed to  open input file :%s\n",
                __FILE__, __LINE__, src);
        exit(-1);
    }

    FILE *fout;
    fout = fopen(dst, "wb");

    if (fout == NULL)
    {
        fprintf(stderr,
                "Error: [%s:%d]: Failed to open output file [%s] to write\n",
                __FILE__, __LINE__, dst);
        exit(-1);
    }

    ctx_t ctx;
    init_context(&ctx);
    ctx.fileCount += 1;
    ctx.allFileSize += get_file_size(fin);
    run_compress(fin, &ctx, fout, bitsToMask);
    fclose(fin);
    fclose(fout);
    return 0;
}

int mrc_uncompress(const char *src, const char *dst)
{
    FILE *fin, *fout;
    ctx_t ctx;
    mrczip_header_t hd;
    fin = fopen(src, "rb");
    fout = fopen(dst, "wb");

    if (fin == NULL || fout == NULL)
    {
        fprintf(stderr, "fail open file\n");
        return -1;
    }

    init_context(&ctx);
    ctx.fileCount += 1;
    init_mrczip_header(&hd, 0);
    read_mrczip_header(fin, &hd);
    print_mrczip_header(&hd, "Header Info in Decompression");
    run_uncompress(fin, &ctx, &hd, fout);
    print_context_info(&ctx, "Contex Info after Decompression");
    fclose(fout);
    fclose(fin);
    return 0;
}

void usage(char **argv)
{
    printf("\nUsage:\n\n");
    printf(
        "\t%s -i <input file> -o <output file> [-t <zip | unzip> -b <bits to erase>]",
        argv[0]);
    printf("\nwhere:\n");
    printf("\t");
    printf("-i\tinput file that need to be compressed or decompressed\n\n");
    printf("\t");
    printf("-o\t output file that being compressed or decompressed \n\n");
    printf("\t");
    printf("-b\t bits to be erased, range[0..32], default is 0\n\n");
    printf("\t");
    printf(
        "-t\t operation type, e.g compress or decompressed file, value should be [zip | unzip], default is zip\n\n");
}

int main(int argc, char *argv[])
{
    char *opt_str = "hi:o:b:t:";
    int opt = 0;
    const char *inputFileName = NULL;
    const char *outputFileName = NULL;
    int bitsToErase = 0;
    const char *operType = "zip";

    if (argc < 2)
    {
        usage(argv);
        exit(-1);
    }

    while ((opt = getopt(argc, argv, opt_str)) != -1)
    {
        switch (opt)
        {
            case 'i':
                inputFileName = optarg;
                break;

            case 'o':
                outputFileName = optarg;
                break;

            case 'b':
                bitsToErase = atoi(optarg);
                break;

            case 't':
                operType = optarg;
                break;

            case 'h':
                usage(argv);
                return 0;

            default:
                printf("Invalid command line parameters: %s!\n", optarg);
                usage(argv);
                return -1;
        }
    }

    printf("ZLIB:%s\n", zlib_version);

    if (strcmp(operType, "zip") == 0)
    {
        mrc_compress(inputFileName, outputFileName, bitsToErase);
    }

    else if (strcmp(operType, "unzip") == 0)
    {
        mrc_uncompress(inputFileName, outputFileName);
    }

    return 0;
}

