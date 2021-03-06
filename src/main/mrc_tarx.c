/*******************************************************************
 *       Filename:  mrc_tarx.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2016年08月15日 11时45分12秒
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
#include <pthread.h>
#include <sys/time.h>
#include "zlib.h"
#include "common.h"
#include "workers.h"
#include "adapt.h"

typedef struct _my_args_t
{
    int idx;
    char *prefix;
    file_container_t *fnames;
    ctx_t nums;
    int bitsToLoss;
} margs_t;

/*Each thread will called this function*/
void *worker_compress(void *arg)
{
    int ret, jdx;
    uint32_t count = 0;
    uint32_t point = 0;
    char header[128] =
    { 0 };
    ctx_t ctx1;
    margs_t *args = (margs_t *) arg;
    ctx_t *ctx = &(args->nums);
    file_container_t *fnames = args->fnames;
    int idx = args->idx;
    int bitsToLoss = args->bitsToLoss;
    sprintf(header, "thread %d", idx);
    init_context(&ctx1);
    point = fnames->size / 10 + 1;

    if (point < 10)
    {
        point = 10;
    }

    while (get_next_file(fnames, &jdx) > -1)
    {
        reset_context(&ctx1);
        ret = zip_compress(&ctx1, fnames->srcs[jdx], fnames->dsts[jdx], bitsToLoss);

        if (ret != 0)
        {
            continue;
        }

        print_context_info(&ctx1, "Context Info in Worker Compress");
        update_context(ctx, &ctx1);
        count += 1;

        if (count % point == 0)
        {
            //print the speeds for all the files
            //ctx_print_more(ctx, header);
            print_context_info(ctx, header);
        }
    }

    return 0;
}

void *worker_uncompress(void *arg)
{
    int ret, file_idx;
    uint32_t count = 0;
    uint32_t point = 0;
    char header[128] =
    { 0 };
    ctx_t ctx2;
    margs_t *args = (margs_t *) arg;
    ctx_t *ctx = &(args->nums);
    file_container_t *fnames = args->fnames;
    int idx = args->idx;
    sprintf(header, "thread %d", idx);
    init_context(&ctx2);
    point = fnames->size / 10 + 1;

    if (point < 10)
    {
        point = 10;
    }

    while (get_next_file(fnames, &file_idx) > -1)
    {
        reset_context(&ctx2);
        ret = zip_uncompress(&ctx2, fnames->srcs[file_idx], fnames->dsts[file_idx]);

        if (ret != 0)
        {
            continue;
        }

        count += 1;
        //print the speeds for current file
        print_context_info(&ctx2, "Context Info in Worker Uncompress");
        update_context(ctx, &ctx2);

        if (count % point == 0)
        {
            print_context_info(ctx, header);
        }
    }

    return 0;
}

/* -------------------------------------------- */
int64_t handle_them(file_container_t *fnames, int threadNum, int type, int bitsToLoss)
{
    int i;
    ctx_t total;
    margs_t *args;
    pthread_t *threads;
    args = malloc(sizeof(margs_t) * threadNum);
    memset(args, 0, sizeof(margs_t) * threadNum);
    threads = malloc(sizeof(pthread_t) * threadNum);
    memset(threads, 0, sizeof(pthread_t) * threadNum);

    for (i = 0; i < threadNum; i++)
    {
        args[i].idx = i;
        args[i].fnames = fnames;
        init_context(&(args[i].nums));
        args[i].bitsToLoss = bitsToLoss;

        if (type == 0)
        {
            pthread_create(&(threads[i]), NULL, worker_compress, &args[i]);
        }

        else if (type == 1)
        {
            pthread_create(&(threads[i]), NULL, worker_uncompress, &args[i]);
        }
    }

    init_context(&total);

    for (i = 0; i < threadNum; i++)
    {
        pthread_join(threads[i], NULL);
        update_context(&total, &(args[i].nums));
    }

    //ctx_print_more(&total, "Overall");
    print_context_info(&total, "[Overall] Context Info In handle_them()");
    free(threads);
    free(args);
    return total.allFileSize;
}

int start_job(int argc, char *argv[], int jtype)
{
    file_container_t fnames;
    double start, end, diff, rate, num;
    struct timeval tm;
    int threads = 1;

    if (argc > 4)
    {
        threads = atoi(argv[4]);
    }

    if (jtype == 1)
    {
        init_file_container(&fnames, argv[2], argv[3], "uz");
    }

    else
    {
        init_file_container(&fnames, argv[2], argv[3], "nz");
    }

    print_file_container_info(&fnames);
    gettimeofday(&tm, NULL);
    start = tm.tv_sec + (tm.tv_usec / 1000000.0);
    num = handle_them(&fnames, threads, jtype, 0);
    gettimeofday(&tm, NULL);
    end = tm.tv_sec + (tm.tv_usec / 1000000.0);
    diff = end - start;
    rate = ((double) num) / (diff * 1024 * 1024);
    num = num / (1024.0 * 1024.0 * 1024);
    printf("num:%0.4f GBytes, time:%0.2f seconds, %0.2fMB/s\n", num, diff,
           rate);
    free_file_container(&fnames);
    return 0;
}


int start_mt_compress(const char *ifcFile,  const char *outputDir, int threadNum, int bitsToLoss)
{
    file_container_t fnames;
    double start, end, diff, rate, num;
    struct timeval tm;
    init_file_container_ex(&fnames, ifcFile, outputDir, "zip");
    print_file_container_info(&fnames);
    gettimeofday(&tm, NULL);
    start = tm.tv_sec + (tm.tv_usec / 1000000.0);
    num = handle_them(&fnames, threadNum, 0, bitsToLoss);
    gettimeofday(&tm, NULL);
    end = tm.tv_sec + (tm.tv_usec / 1000000.0);
    diff = end - start;
    rate = ((double) num) / (diff * 1024 * 1024);
    num = num / (1024.0 * 1024.0 * 1024);
    printf("num:%0.4f GBytes, time:%0.2f seconds, %0.2fMB/s\n", num, diff, rate);
    free_file_container(&fnames);
    return 0;
}


int start_mt_uncompress(const char *ifcFile,  const char *outputDir, int threadNum)
{
    file_container_t fnames;
    double start, end, diff, rate, num;
    struct timeval tm;
    init_file_container_ex(&fnames, ifcFile, outputDir, "unzip");
    print_file_container_info(&fnames);
    gettimeofday(&tm, NULL);
    start = tm.tv_sec + (tm.tv_usec / 1000000.0);
    num = handle_them(&fnames, threadNum, 1, 0);
    gettimeofday(&tm, NULL);
    end = tm.tv_sec + (tm.tv_usec / 1000000.0);
    diff = end - start;
    rate = ((double) num) / (diff * 1024 * 1024);
    num = num / (1024.0 * 1024.0 * 1024);
    printf("num:%0.4f GBytes, time:%0.2f seconds, %0.2fMB/s\n", num, diff,
           rate);
    free_file_container(&fnames);
    return 0;
}




void print_usage(const char *cmd)
{
    printf("Usage:\n(1)compress only: \n");
    printf("\t%s -oz <fname> <key> [threads]\n", cmd);
    printf("\te.g:%s -oz ./files.txt atmos 2\n", cmd);
    printf("(2)decompress only:\n");
    printf("\t%s -ou <fname> <key> [threads]\n", cmd);
    printf("\t%s -ou ./files.txt atmos 2\n", cmd);
    printf("(3)compress and decompress:\n");
    printf("\t%s -ob <fname> <key> [threads]\n", cmd);
    printf("\t%s -ob ./files.txt atmos 2\n", cmd);
}

int get_opt(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        exit(-1);
    }

    if (strcmp(argv[1], "-oz") == 0)
    {
        if (argc < 4)
        {
            print_usage(argv[0]);
            exit(-1);
        }

        printf("[only compress files]\n");
        start_job(argc, argv, 0);
    }

    else if (strcmp(argv[1], "-ou") == 0)
    {
        if (argc < 4)
        {
            print_usage(argv[0]);
            exit(-1);
        }

        printf("[only uncompress files]\n");
        start_job(argc, argv, 1);
    }

    else
    {
        if (argc < 4)
        {
            print_usage(argv[0]);
            exit(-1);
        }

        printf("[compress & decompress files]\n");
        start_job(argc, argv, 2);
    }

    return 0;
}


void usage(char **argv)
{
    printf("\nUsage:\n\n");
    printf(
        "\t%s -i <file list descriptor>  -t <zip | unzip> [-o <root dir of output file> -b <bits to erase> -n <thread numbers> -d <0 | 1>]",
        argv[0]);
    printf("\nwhere:\n");
    printf("\t");
    printf("-i\t a text file that contains the path of files that need to becompressed or decompressed\n\n");
    printf("\t");
    printf("-o\t a directory that used the output the compressed/uncompressed file, default is /tmp/ \n\n");
    printf("\t");
    printf("-b\t bits to be erased, range[0..32], default is 0\n\n");
    printf("\t");
    printf("-d\t whether to test the throughput, range[0 | 1 ], default is 0 means not to test throughput, 1 means to test the throughput\n\n");
    printf("\t");
    printf(
        "-t\t operation type, e.g compress or decompressed file, value should be [zip | unzip]\n\n");
    printf("\t");
    printf(
        "-n\t thread number to be invoked, default is 2\n\n");
}

int main(int argc, char *argv[])
{
    //printf("[Compress data with hpos]\n");
    //get_opt(argc, argv);
    char *opt_str = "hi:o:b:t:n:d:s:";
    int opt = 0;
    const char *ifcFile = NULL;
    const char *outputDir = "/tmp/";
    int bitsToLoss = 0;
    int threadNum = 2;
    extern int isTestThroughput ;
    const char *operType = "zip";
    const char *dataConvertedType = "float";

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
                ifcFile = optarg;
                break;

            case 'o':
                outputDir = optarg;
                break;

            case 'b':
                bitsToLoss = atoi(optarg);
                break;

            case 'n':
                threadNum = atoi(optarg);
                break;

            case 'd':
                isTestThroughput = atoi(optarg);
                break;

            case 't':
                operType = optarg;
                break;

            case 's':
                dataConvertedType = optarg;
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
        start_mt_compress(ifcFile, outputDir, threadNum, bitsToLoss);
    }

    else if (strcmp(operType, "unzip") == 0)
    {
        start_mt_uncompress(ifcFile, outputDir, threadNum);
    }

    return 0;
}
