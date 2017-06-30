/*******************************************************************
 *       Filename:  workers.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2016年08月07日 11时47分36秒
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
#include <math.h>
#include "common.h"
#include "workers.h"
#include "constant.h"
#include "mrczip.h"

static int bitsMaskTable[] =
{
    0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFC, 0xFFFFFFF8, 0xFFFFFFF0, 0xFFFFFFE0,
    0xFFFFFFC0, 0xFFFFFF80, 0xFFFFFF00, 0xFFFFFE00, 0xFFFFFC00, 0xFFFFF800,
    0xFFFFF000, 0xFFFFE000, 0xFFFFC000, 0xFFFF8000, 0xFFFF0000, 0xFFFE0000,
    0xFFFC0000, 0xFFF80000, 0xFFF00000, 0xFFE00000, 0xFFC00000, 0xFF800000,
    0xFF000000, 0xFE000000, 0xFC000000, 0xF8000000, 0xF0000000, 0xE0000000,
    0xC0000000, 0x80000000, 0x00000000
};

int isTestThroughput = 0;


void term_zips(mzip_t zips[])
{
    int i;

    for (i = 0; i < COMPRESSION_PATH_NUM; i++)
    {
        mzip_term(&zips[i]);
    }
}

void uncompress_byte_stream(FILE *fin, mzip_t zips[], char *outs[], uint32_t chk)
{
    int i;
    char *p;
    char bhd[HDR_SIZE * COMPRESSION_PATH_NUM] =
    { 0 };
    btype_t btypes[COMPRESSION_PATH_NUM];
    double start;
    /*Read header in every block*/
    fread(bhd, 1, HDR_SIZE * COMPRESSION_PATH_NUM, fin);

    for (i = 0, p = bhd; i < COMPRESSION_PATH_NUM; i++)
    {
        /*In every block, the sum of each byte stream length and HDR_SIZE will be saved in zips[j].inlen*/
        unpack_header(p, &(btypes[i]), &(zips[i].inlen));
        p += HDR_SIZE;
        fread(zips[i].in, 1, zips[i].inlen, fin);
    }

    for (i = 0; i < COMPRESSION_PATH_NUM; i++)
    {
        start = now_sec();
        /*uncompress each byte stream, the uncompress data will be saved in outs[i]*/
        zips[i].unzipfun(&(zips[i]), btypes[i], chk, &(outs[i]));
        zips[i].time2 += (now_sec() - start);
    }

    return;
}

void apply_mask(float *buffer, int startIndex, int num, const int bitsToErase,
                int isFirstChk)
{
    int *p = (int *) buffer;

    /**
     *  We should ignore first 1024 bytes in first chunk since it is the header info
     */
    if (isFirstChk)
    {
        startIndex = startIndex + 256;
        p = p + 256;
    }

    for (int i = startIndex; i < num; i++)
    {
        *p = (*p) & bitsMaskTable[bitsToErase];
        p = p + 1;
    }
}

void apply_mask_to_int(int *buffer, int startIndex, int num, const int bitsToErase,
                       int isFirstChk)
{
    int *p = buffer;

    /**
     *  We should ignore first 1024 bytes in first chunk since it is the header info
     */
    if (isFirstChk)
    {
        startIndex = startIndex + 256;
        p = p + 256;
    }

    for (int i = startIndex; i < num; i++)
    {
        *p = (*p) & bitsMaskTable[bitsToErase];
        p = p + 1;
    }
}


void convert_float_to_int(float *buf, int *data_to_compress, int num, int isFirstChk)
{

    int startIndex = 0;
    if(isFirstChk)
    {
        memcpy(data_to_compress, buf, 1024);
        startIndex = 256;
    }

    char *tmp_buf = (char *)malloc(num);

    for(int i = startIndex; i < num; i ++)
    {
        tmp_buf[i] = (char)round(buf[i]);
    }

    for(int i = startIndex; i < num; i ++)
    {
        char *p1 = (char *)&data_to_compress[i];
        *p1 = tmp_buf[i];
    } 

    free(tmp_buf);
}
void split_convert_float_to_one_byte_stream(float *buf, int num, char *zins[], int bitsToMask,  int isFirstChk, int *data_to_compress)
{
    char *p0, *p1, *p2, *p3;

    convert_float_to_int(buf, data_to_compress, num, isFirstChk);
    char *p = (char *) data_to_compress;
    p0 = zins[0];
    p1 = zins[1];
    p2 = zins[2];
    p3 = zins[3];
    /**
     *  1024 Bytes in the first chunk is the header info in MRC file, we should not apply lossy compression to it
     */
    //apply_mask(buf, 0, num, bitsToMask, isFirstChk);

    /**
     * Make zips[i] to corresponding byte stream
     */
    for (int i = 0; i < num; i++)
    {
        *p0++ = *p++;
        *p1++ = *p++;
        *p2++ = *p++;
        *p3++ = *p++;
    }
}




void split_float_to_byte_stream(float *buf, int num, char *zins[], int bitsToMask,  int isFirstChk)
{
    char *p0, *p1, *p2, *p3;
    char *p = (char *) buf;
    p0 = zins[0];
    p1 = zins[1];
    p2 = zins[2];
    p3 = zins[3];
    /**
     *  1024 Bytes in the first chunk is the header info in MRC file, we should not apply lossy compression to it
     */
    apply_mask(buf, 0, num, bitsToMask, isFirstChk);

    /**
     * Make zips[i] to corresponding byte stream
     */
    for (int i = 0; i < num; i++)
    {
        *p0++ = *p++;
        *p1++ = *p++;
        *p2++ = *p++;
        *p3++ = *p++;
    }
}

int comparator(const void *a, const void *b)
{
    const float ZERO = 1.0E-6;
    float *fa = (float *)a;
    float *fb = (float *)b;

    if (*fa > *fb)
    {
        return 1;
    }

    else if (*fa < *fb)
    {
        return -1;
    }

    else if (fabsf(*fa - *fb) < ZERO)
    {
        return 0;
    }

    return 0;
}

float getMiddle(float *buf, int num, int isFirstChk)
{
    float *tmpBuffer = (float *)malloc(num * sizeof(float));

    if (tmpBuffer == NULL)
    {
        fprintf(stderr, "[%s:%d] Failed to alloc memory\n", __FILE__, __LINE__);
        exit(-1);
    }

    if (isFirstChk)
    {
        num = num - 256;
        memcpy(tmpBuffer, buf + 256, num  * sizeof(float));
    }

    else
    {
        memcpy(tmpBuffer, buf, num * sizeof(float));
    }

    qsort(tmpBuffer, num, sizeof(float), comparator);
    int middleIndex = num / 2;
    float middleValue = tmpBuffer[middleIndex];
    free(tmpBuffer);
    return middleValue;
}

void XOR(float *buffer, float middleValue,  int num, int isFirstChk)
{
    int startIndex = 0;

    if (isFirstChk)
    {
        startIndex = 256;
    }

    char *pm = (char *)&middleValue;
    char *pc = NULL;

    for (int i = startIndex; i < num; i ++)
    {
        pc = (char *)& buffer[i];
        pc[0] = pc[0] ^ pm[0];
        pc[1] = pc[1] ^ pm[1];
        pc[2] = pc[2] ^ pm[2];
    }
}

void split_float_to_byte_stream_v2(float *buf, int num, char *zins[], int bitsToMask, int isFirstChk)
{
    char *p0, *p1, *p2, *p3;
    char *p = (char *) buf;
    p0 = zins[0];
    p1 = zins[1];
    p2 = zins[2];
    p3 = zins[3];
    /**
     *  1024 Bytes in the first chunk is the header info in MRC file, we should not apply lossy compression to it
     */
    apply_mask(buf, 0, num, bitsToMask, isFirstChk);

    //  float middleValue = getMiddle(buf, num, isFirstChk);
    //  //float middleValue = isFirstChk ? buf[(num - 256) / 2] : buf[num / 2];
    //  XOR(buf, middleValue, num, isFirstChk);

    /**
     * Make zips[i] to corresponding byte stream
     */
    for (int i = 0; i < num; i++)
    {
        *p0++ = *p++;
        *p1++ = *p++;
        *p2++ = *p++;
        *p3++ = *p++;
    }
}

/*This function can be used to replace split_float_to_byte_stream in future*/
void split_float_to_byte_stream_ex(float *buf, int num, char *zins[], int bitsToMask, int isFirstChk)
{
    char *p[COMPRESSION_PATH_NUM];
    char *pf = (char *) buf;

    for (int i = 0; i < COMPRESSION_PATH_NUM; i ++)
    {
        p[i] = zins[i];
    }

    /**
       *  1024 Bytes in the first chunk is the header info in MRC file, we should not apply lossy compression to it
       */
    apply_mask(buf, 0, num, bitsToMask, isFirstChk);

    for (int i = 0; i < num; i ++)
    {
        for (int j = 0; j < COMPRESSION_PATH_NUM; j ++)
        {
            *(p[j]) = *pf;
            p[j] ++;
            pf ++;
        }
    }
}

void split_float_to_8byte_stream(float *buf, int num, char *zins[], int bitsToMask, int isFirstChk)
{
    char *p[COMPRESSION_PATH_NUM];
    char *pf = (char *) buf;

    for (int i = 0; i < COMPRESSION_PATH_NUM; i ++)
    {
        p[i] = zins[i];
    }

    /**
       *  1024 Bytes in the first chunk is the header info in MRC file, we should not apply lossy compression to it
       */
    apply_mask(buf, 0, num, bitsToMask, isFirstChk);

    for (int i = 0; i < num; i ++)
    {
        for (int j = 0; j < COMPRESSION_PATH_NUM; j = j + 2)
        {
            *(p[j]) = *pf;
            *(p[j]) = (*(p[j])) & 0x0F;
            *(p[j + 1]) = *pf;
            *(p[j + 1]) = (*(p[j + 1])) & 0xF0;
            //p[j] = p[j] + 2;
            p[j] ++;
            p[j + 1] ++;
            pf ++;
        }
    }

    //  for(int i = 0; i < num; i ++)
    //  {
    //    for(int j = 0; j < COMPRESSION_PATH_NUM; j ++)
    //    {
    //      *(p[j]) = *pf;
    //      p[j] ++;
    //      pf ++;
    //    }
    //  }
}

void split_float_to_16byte_stream(float *buf, int num, char *zins[], int bitsToMask,  int isFirstChk)
{
    char *p[COMPRESSION_PATH_NUM];
    char *pf = (char *) buf;

    for (int i = 0; i < COMPRESSION_PATH_NUM; i ++)
    {
        p[i] = zins[i];
    }

    /**
       *  1024 Bytes in the first chunk is the header info in MRC file, we should not apply lossy compression to it
       */
    apply_mask(buf, 0, num, bitsToMask, isFirstChk);

    for (int i = 0; i < num; i ++)
    {
        for (int j = 0; j < COMPRESSION_PATH_NUM; j = j + 4)
        {
            *(p[j]) = *pf;
            *(p[j]) = (*(p[j])) & 0x03;
            *(p[j + 1]) = *pf;
            *(p[j + 1]) = ( (*(p[j + 1])) & 0x0C) >> 2;
            *(p[j + 2]) = *pf;
            *(p[j + 2]) = ((*(p[j + 2])) & 0x30) >> 4;
            *(p[j + 3]) = *pf;
            *(p[j + 3]) = ((*(p[j + 3])) & 0xC0) >> 6;
            p[j] ++;
            p[j + 1] ++;
            p[j + 2] ++;
            p[j + 3] ++;
            pf ++;
        }
    }

    //  for(int i = 0; i < num; i ++)
    //  {
    //    for(int j = 0; j < COMPRESSION_PATH_NUM; j ++)
    //    {
    //      *(p[j]) = *pf;
    //      p[j] ++;
    //      pf ++;
    //    }
    //  }
}



void merge_byte_to_float_stream(float *buf, int num, char *zouts[])
{
    int i;
    char *p0, *p1, *p2, *p3;
    char *p = (char *) buf;
    p0 = zouts[0];
    p1 = zouts[1];
    p2 = zouts[2];
    p3 = zouts[3];

    for (i = 0; i < num; i++)
    {
        *p++ = *p0++;
        *p++ = *p1++;
        *p++ = *p2++;
        *p++ = *p3++;
    }

    return;
}

void merge_one_byte_to_float_stream(float *buf, int num, char *zouts[], int isFirstChunk)
{
    char *p0, *p1, *p2, *p3;
    char *p = (char *) buf;
    p0 = zouts[0];
    p1 = zouts[1];
    p2 = zouts[2];
    p3 = zouts[3];

    //static int index = 0;
    if(isFirstChunk)
    {
        /*
         *for(int i = 0; i < 256; i ++)
         *{
         *    *p++ = *p0++;
         *    *p++ = *p1++;
         *    *p++ = *p2++;
         *    *p++ = *p3++;
         *}
         */

        for(int i = 0; i < 256; i ++)
        {
            p[i * 4 + 0] = p0[i];
            p[i * 4 + 1] = p1[i];
            p[i * 4 + 2] = p2[i];
            p[i * 4 + 3] = p3[i];
        }
        
        //printf("=========================<%d>======================\n", index ++);
        for(int i = 256; i < num; i ++)
        {
            buf[i] = (float)p0[i];
        //    printf("%d:%d:%f\n", i, p0[i], buf[i]);    
        }
       // printf("===================================================\n");

    }
    else
    {
    
        for(int i = 0; i < num; i ++)
        {
            buf[i] = (float)p0[i];
        }
    }


    return;
    /*static int index = 0;*/
    /*printf("=========================<%d>=============================\n", index ++);*/
    /*for (i = 0; i < num; i++)*/
    /*{*/

    /*    buf[i] = (float)p0[i];*/
    /*    printf("%d:%d:%f\n", i, p0[i], buf[i]);*/
        /*
         **p++ = *p0++;
         **p++ = *p1++;
         **p++ = *p2++;
         **p++ = *p3++;
         */
    /*}*/
    /*printf("======================================================\n");*/

    /*return;*/
}
void split_int_to_byte_stream(float *buf, int num, char *zins[], int bitsToMask,  int isFirstChk)
{
    fprintf(stderr, "Start to covert float stream to int stream\n");
    int *intBuffer = (int *)malloc(num * sizeof(int));

    for (int i = 0; i < num; i ++)
    {
        intBuffer[i] = (int)(buf[i] * 100.0);
    }

    char *p0, *p1, *p2, *p3;
    char *p = (char *) intBuffer;
    p0 = zins[0];
    p1 = zins[1];
    p2 = zins[2];
    p3 = zins[3];
    /**
     *  1024 Bytes in the first chunk is the header info in MRC file, we should not apply lossy compression to it
     */
    apply_mask_to_int(intBuffer, 0, num, bitsToMask, isFirstChk);

    /**
     * Make zips[i] to corresponding byte stream
     */
    for (int i = 0; i < num; i++)
    {
        *p0++ = *p++;
        *p1++ = *p++;
        *p2++ = *p++;
        *p3++ = *p++;
    }
}

void merge_byte_to_int_stream(float *buf, int num, char *zouts[])
{
    int i;
    char *p0, *p1, *p2, *p3;
    char *p = (char *) buf;
    p0 = zouts[0];
    p1 = zouts[1];
    p2 = zouts[2];
    p3 = zouts[3];

    for (i = 0; i < num; i++)
    {
        *p++ = *p0++;
        *p++ = *p1++;
        *p++ = *p2++;
        *p++ = *p3++;
    }

    return;
}



int run_uncompress(FILE *fin, ctx_t *ctx, mrczip_header_t *hd, FILE *fout, const char *dataConvertedType)
{
    uint32_t i, j, num;
    uint32_t chk_size;
    uint64_t fsz;
    float *buf;
    mzip_t zips[COMPRESSION_PATH_NUM]; /* points to 4 bytes stream in float stream */
    char *outs[COMPRESSION_PATH_NUM];
    double start;
    fsz = hd->fsz / COMPRESSION_PATH_NUM;
    chk_size = hd->chk;
    start = now_sec();
    buf = (float *) malloc(sizeof(float) * chk_size);

    for (j = 0; j < COMPRESSION_PATH_NUM; j++)
    {
        init_mrc_zip_stream(&(zips[j]), chk_size, hd->ztypes[j] + 1, 0);
    }

    ctx->unzipTime += (now_sec() - start);
    /*Calculate how many chunks we have*/
    num = fsz / chk_size;

    int isFirstChunk = 0;
    for (i = 0; i < num; i++)
    {

        if( i == 0 )
        {
            isFirstChunk = 1;
        }
        else
        {
            isFirstChunk = 0;
        }
        uncompress_byte_stream(fin, zips, outs, chk_size);
        if(strcmp(dataConvertedType, "int") == 0)
        {
        
            printf("Decompress data as int type, isFirstChunk = %d\n", isFirstChunk);
            merge_one_byte_to_float_stream(buf, chk_size, outs, isFirstChunk);
        }
        else
        {
            printf("Decompress data as float type\n");
            merge_byte_to_float_stream(buf, chk_size, outs);
        }

        //    for(int i = 0; i < chk_size; i ++ )
        //    {
        //      buf[i] = buf[i] / 100.0f;
        //    }

        //#ifdef _OUTPUT_UNZIP_

        //if(_OUTPUT_UNZIP_ == 1)
        if (isTestThroughput  != 1)
        {
            //printf("======> start to write file1\n");
            fwrite(buf, sizeof(float), chk_size, fout);
        }

        //#endif
    }

    chk_size = fsz % chk_size;

    int total_chunks = num;

    //if the file size is smaller than chunk size, then we have only one chunk, then the first chunk flag should be true
    if(total_chunks == 0)
    {
        isFirstChunk = 1;
    }
    else
    {
        isFirstChunk = 0;
    }

    if (chk_size > 0)
    {
        uncompress_byte_stream(fin, zips, outs, chk_size);
        start = now_sec();
        if(strcmp(dataConvertedType, "int") == 0)
        {
            printf("The rest part is treated as int\n");
            merge_one_byte_to_float_stream(buf, chk_size, outs, isFirstChunk);
        }
        else
        {
            printf("The rest part is treated as float\n");
            merge_byte_to_float_stream(buf, chk_size, outs);
        }
        ctx->unzipTime += (now_sec() - start);

        //#ifdef _OUTPUT_UNZIP_
        //if(_OUTPUT_UNZIP_ == 1)
        if (isTestThroughput != 1)
        {
            //printf("======> start to write file1\n");
            fwrite(buf, sizeof(float), chk_size, fout);
        }

        //#endif
    }

    free(buf);
#ifdef _PRINT_ZIPS_
    print_result(zips, COMPRESSION_PATH_NUM, "Decompress Result Info");
#endif

    for (j = 0; j < COMPRESSION_PATH_NUM; j++)
    {
        ctx->allFileSize += zips[j].fsz;
        ctx->allZipFileSize += zips[j].zfsz;
        ctx->unzipTime += zips[j].time2;
        mzip_term(&(zips[j]));
    }

    return 0;
}

int run_compress(FILE *fin, ctx_t *ctx, FILE *fout, const int bitsToMask, const char *dataConvertedType)
{
    int j;
    int num;
    mrczip_header_t hd;
#ifdef _OUTPUT_ZIP_
    char bhd[COMPRESSION_PATH_NUM * HDR_SIZE] =   { 0 };
#endif
    float *buf;
    mzip_t zips[COMPRESSION_PATH_NUM];
    char *zins[COMPRESSION_PATH_NUM], *zouts[COMPRESSION_PATH_NUM];
    int32_t zlens[COMPRESSION_PATH_NUM];
    double begin, zbegin; //for timing
    begin = now_sec();
    buf = (float *) malloc(sizeof(float) * CHUNK_SIZE);

    int *data_to_compress = (int *)malloc(sizeof(int) * CHUNK_SIZE);

    if (buf == NULL)
    {
        fprintf(stderr, "[%s:%d] ERROR: fail to alloc mem\n", __FILE__, __LINE__);
        exit(-1);
    }

    /*
     * Firstly, we init 4 compressor for each byte of mrc float stream
     * */
    for (int i = 0; i < COMPRESSION_PATH_NUM; i ++)
    {
        init_mrc_zip_stream(&zips[i], CHUNK_SIZE, ZLIB_DEF, ZIP_FAST);
        //    init_mrc_zip_stream(&zips[1], CHUNK_SIZE, ZLIB_DEF, ZIP_FAST);
        //    init_mrc_zip_stream(&zips[2], CHUNK_SIZE, ZLIB_DEF, ZIP_FAST);
        //    init_mrc_zip_stream(&zips[3], CHUNK_SIZE, ZLIB_DEF, ZIP_FAST);
    }

    /*
     * Make zins[i] point to compressed data buffer of 4 zip streams
     * */
    for (j = 0; j < COMPRESSION_PATH_NUM; j++)
    {
        zins[j] = zips[j].zin;
    }

    /* begin to write zip file header */
    init_mrczip_header(&hd, 0);
    hd.chk = CHUNK_SIZE;

    for (j = 0; j < COMPRESSION_PATH_NUM; j++)
    {
        hd.ztypes[j] = zips[j].ztype;
    }

    ctx->zipTime += (now_sec() - begin);
    hd.fsz = get_file_size(fin);
    num = fread(buf, sizeof(uint32_t), CHUNK_SIZE, fin);

    /**
     *  if we have  data need to be compressed, we will firstly write header into zip file, the zip file format is defined as following:
     *  8 bytes: original file size
     *  4 bytes: pre-defined chunk size
     *  1 byte: compressed type,like compress_0,compress_1, compress_2, compress_3
     *  5 bytes: compress method, like ZLIB_DEF, LZ4_DEF, LZ4HC_DEF，  for each byte stream
     *  N bytes compress data organized as compressed chunk list:
     *      each compressed chunk is organized as following:
     *          16 bytes chunk header: every 4 bytes indicate the length of each compressed byte stream
     *          4 compressed byte stream data: first compressed stream data is the first byte data in the incomming float number sequence, second compressed stream data is the second byte data in the incomming float number sequence, ...
     */
    if (num > 0)
    {
        //#ifdef _OUTPUT_ZIP_
        //if(_OUTPUT_ZIP_ == 1)
        if (isTestThroughput != 1) // we will not write file if we want to test throughput, 1 means we are going to test throughput
        {
            write_mrczip_header(fout, &hd);
        }

        //#endif
    }

    //  FILE *fpSplit[4];
    //  for(int i = 0; i < 4; i ++)
    //  {
    //    char fileName[128];
    //    sprintf(fileName, "../tmp/path%d.bin", i);
    //    fpSplit[i] = fopen(fileName, "wb");
    //
    //  }
    int isFirstChk = 1;

    while (num > 0)
    {
        begin = now_sec();
        if(strcmp(dataConvertedType, "int") == 0)
        {
            printf("Treate data as int number\n");
            memset(data_to_compress, 0, CHUNK_SIZE * sizeof(int));
            split_convert_float_to_one_byte_stream(buf, num, zins, 24, isFirstChk, data_to_compress);
        }
        else
        {
            printf("Treate data as float number....\n");
            split_float_to_byte_stream(buf, num, zins, bitsToMask, isFirstChk);
        }
        //split_float_to_byte_stream_v2(buf, num, zins, bitsToMask, isFirstChk);
        //split_float_to_8byte_stream(buf, num, zins, bitsToMask, isFirstChk);
        //split_float_to_16byte_stream(buf, num, zins, bitsToMask, isFirstChk);
        //split_int_to_byte_stream(buf, num, zins, bitsToMask, isFirstChk);
        //    for(int i = 0; i < 4; i ++)
        //    {
        //      fwrite(zins[i], 1, num, fpSplit[i]);
        //    }
        /**
         *  We have processed the first chunk, so from next loop, this flag should be false
         */
        isFirstChk = 0;
        ctx->zipTime += (now_sec() - begin);
        /**
         *  TODO:we can select the compress method for every byte stream here, for example:
         *  set the corresponding method value  in hd.ztypes[j]
         */
        begin = now_sec();

        for (j = 0; j < COMPRESSION_PATH_NUM; j++)
        {
            zbegin = now_sec();
            zips[j].inlen = num;
            /**
             *  Here will invoke the mzlib_def() to compress the data
             *  for each chunk;
             *    4 input  byte streams are pointed by zips[0..3];
             *    4 output compressed byte streams are pointed by zouts[0..3];
             *    the length of each compressed byte stream is stored in zlens[0..3]
             */
            zips[j].zipfun(&(zips[j]), &zouts[j], &zlens[j]);
            //mzlib_def(&(zips[j]), &zouts[j], &zlens[j]);
            zips[j].compressTime += (now_sec() - zbegin);
        }

        ctx->zipTime += (now_sec() - begin);

        //#ifdef _OUTPUT_ZIP_
        //if(_OUTPUT_ZIP_ == 1)
        if (isTestThroughput != 1)
        {
            /**
             * The first 4 bytes data in zouts[j][0..3] is the length of each compressed byte stream, we extracted from zouts[0..3] respectly, and write the the header of each compress blocked
             */
            for (j = 0; j < COMPRESSION_PATH_NUM; j++)
            {
                memcpy(bhd + j * HDR_SIZE, zouts[j], HDR_SIZE);
            }

            fwrite(bhd, 1, COMPRESSION_PATH_NUM * HDR_SIZE, fout);

            /**
             *  Write the compressed data of each chunk to the zip file, the start position of each compressed byte stream is from zouts[j] + HDR_SIZE, where HDR_SIZe = 4
             */
            for (j = 0; j < COMPRESSION_PATH_NUM; j++)
            {
                fwrite(zouts[j] + HDR_SIZE, 1, zlens[j] - HDR_SIZE, fout);
            }
        }

        //#endif
        num = fread(buf, sizeof(float), CHUNK_SIZE, fin);
    }

    free(buf);
    free(data_to_compress);
    //  for(int i = 0; i < 4; i ++)
    //  {
    //    fclose(fpSplit[i]);
    //  }
#ifdef _PRINT_ZIPS_
    print_result(zips, COMPRESSION_PATH_NUM, "Compression Summary Result");
#endif

    /**
     *  ctx->zfsz stored  the sum of the length of each compressed byte stream
     */
    for (j = 0; j < COMPRESSION_PATH_NUM; j++)
    {
        ctx->allZipFileSize += zips[j].zfsz;
    }

    for (j = 0; j < COMPRESSION_PATH_NUM; j++)
    {
        mzip_term(&zips[j]);
    }

    return 0;
}

