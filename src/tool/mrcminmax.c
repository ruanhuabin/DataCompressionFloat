/*******************************************************************
 *       Filename:  erasebytes.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2016年08月13日 18时35分52秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ruan Huabin
 *          Email:  ruanhuabin@gmail.com
 *        Company:  HPC tsinghua
 *
 *******************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

int bitsMaskTable[] = { 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFC, 0xFFFFFFF8, 0xFFFFFFF0,
                        0xFFFFFFE0, 0xFFFFFFC0, 0xFFFFFF80, 0xFFFFFF00, 0xFFFFFE00, 0xFFFFFC00,
                        0xFFFFF800, 0xFFFFF000, 0xFFFFE000, 0xFFFFC000, 0xFFFF8000, 0xFFFF0000,
                        0xFFFE0000, 0xFFFC0000, 0xFFF80000, 0xFFF00000, 0xFFE00000, 0xFFC00000,
                        0xFF800000, 0xFF000000, 0xFE000000, 0xFC000000, 0xF8000000, 0xF0000000,
                        0xE0000000, 0xC0000000, 0x80000000, 0x00000000
                      };
void usage(char **argv)
{
    printf("\nUsage:\n\n");
    printf("\t%s -i <input file> -o <output file> [-b <bits to erase>]",
           argv[0]);
    printf("\nwhere:\n");
    printf("\t");
    printf("-i\tinput file that need to erase lowest byte\n\n");
}

int main(int argc, char *argv[])
{
    char *opt_str = "hi:";
    int opt = 0;
    const char *inputFileName = NULL;
    const char *outputFileName = NULL;
    int bitsToErase = 8;

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
            case 'h':
                usage(argv);
                return 0;

            default:
                printf("Invalid command line parameters: %s!\n", optarg);
                usage(argv);
                return -1;
        }
    }

    printf("Input File = %s, Output File = %s, bitsToErase = %d\n",
           inputFileName, outputFileName, bitsToErase);
    FILE *inputFile = fopen(inputFileName, "rb");

    if (inputFile == NULL)
    {
        fprintf(stderr, "[%s:%d] open file [%s] failed\n", __FILE__, __LINE__,
                inputFileName);
        exit(-1);
    }


    const int ITEMS_TO_READ = 1024 * 1024 * 8;
    float *buffer1 = (float *) malloc(ITEMS_TO_READ * sizeof(float));
    int num1 = fread(buffer1, 1, 1024, inputFile);
    printf("num1 = %d\n", num1);
    num1 = fread(buffer1, sizeof(float), ITEMS_TO_READ, inputFile);
    printf("num1 = %d\n", num1);


    float max = buffer1[0];
    float min = buffer1[0];

    int maxIndex = 0;
    int minIndex = 0;
    int chunkIndex = 0;
    int startIndex = 0;
    while (num1 > 0)
    {

        if(chunkIndex == 0)
        {
            startIndex = 256;
        }
        else
        {
            startIndex = 0;
        }
        for (int i = startIndex; i < num1; i++)
        {
            if(buffer1[i] > max)
            {
                max = buffer1[i];
                maxIndex = chunkIndex * ITEMS_TO_READ + i;
            }

            if(buffer1[i] < min)
            {
                min = buffer1[i];
                minIndex = chunkIndex * ITEMS_TO_READ + i;
            }
        }


        num1 = fread(buffer1, sizeof(float), ITEMS_TO_READ, inputFile);
        chunkIndex ++;
    }

    printf("min = %f, max = %f, minIndex = %d, maxIndex = %d\n", min, max, minIndex, maxIndex);


    free(buffer1);
    fclose(inputFile);
    return EXIT_SUCCESS;
}

