/*******************************************************************
 *       Filename:  erroranalysis.c                                     
 *                                                                 
 *    Description:                                        
 *                                                                 
 *        Version:  1.0                                            
 *        Created:  2016年08月13日 09时30分21秒                                 
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
void usage(char **argv)
{
      printf("\nUsage:\n\n");
      printf("\t%s -a <original file> -b <decompress file> -o <output result file>", argv[0]);
      printf("\nwhere:\n");
      printf("\t");
      printf("-a\toriginal file without being compressed\n\n");
      printf("\t");
      printf("-b\tfile that being decompressed from a compessed file\n\n");
      printf("\t");
      printf("-o\tfile that used to save the analysis result\n\n");
}

void compare(float *buffer1, float *buffer2, int n)
{
    //float *diff = (float *)malloc(n * sizeof(float));

    float n1 = 0.0f;
    float n2 = 0.0f;
    float err = 0.0f;
    float relativeErr = 0.0f;
    for(int i = 0; i < n; i ++)
    {
        n1 = buffer1[i];
        n2 = buffer2[i];
        err = fabsf(n2 - n1);
        if(fabsf(n1) > 10E-4)
        {
            relativeErr = err / fabsf(n1);
        }
        else
        {
            relativeErr = 0.0f;
        }
        printf("%f %f %f %E %f %E\n", n1, n2, err, err, relativeErr, relativeErr);
    }
}

int main ( int argc, char *argv[] )
{ 
    char *opt_str = "ha:b:o:";
    int opt = 0;
    const char *originalFile = NULL;
    const char *decompressFile = NULL;
    const char *outputResultFile = NULL;


    if(argc < 2)
    {
        usage(argv);
        exit(-1);
    }

    while( (opt = getopt(argc, argv, opt_str)) != -1)
    {
        switch(opt)
        {
                case 'a':
                        originalFile = optarg;
                        break;
                case 'b':
                        decompressFile = optarg;
                        break;
                case 'o':
                        outputResultFile = optarg;
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


    printf("original File = %s, decompress file = %s, output file = %s\n", originalFile, decompressFile, outputResultFile);

    FILE *origInputFile = fopen(originalFile, "rb");
    FILE *decompressInputFile = fopen(decompressFile, "rb");
    FILE *outputFile = fopen(outputResultFile, "w");

    if( origInputFile == NULL || decompressInputFile == NULL || outputFile == NULL )
    {
        fprintf(stderr, "[%s:%d] open file [%s] failed\n", __FILE__, __LINE__, originalFile);
        exit(-1);
    }
 
    if( decompressInputFile == NULL )
    {
        fprintf(stderr, "[%s:%d] open file [%s] failed\n", __FILE__, __LINE__, decompressFile);
        exit(-1);
    }

    if( outputFile == NULL )
    {
        fprintf(stderr, "[%s:%d] open file [%s] failed\n", __FILE__, __LINE__, outputResultFile);
        exit(-1);
    }

    const int ITEMS_TO_READ = 1024 * 1024 * 8;
    float *buffer1 = (float *)malloc(ITEMS_TO_READ * sizeof(float));
    float *buffer2 = (float *)malloc(ITEMS_TO_READ * sizeof(float));

    int num1 = fread(buffer1, sizeof(float), ITEMS_TO_READ, origInputFile);
    int num2 = fread(buffer2, sizeof(float), ITEMS_TO_READ, decompressInputFile);

    printf("num1 = %d, num2 = %d\n", num1, num2);

    while(num1 > 0 && num2 > 0)
    {
        compare(buffer1, buffer2, num1 < num2 ? num1 : num2);
        num1 = fread(buffer1, sizeof(float), ITEMS_TO_READ, origInputFile);
        num2 = fread(buffer2, sizeof(float), ITEMS_TO_READ, decompressInputFile);
    }

    
    free(buffer1);
    free(buffer2);
    return EXIT_SUCCESS;
}

