/*******************************************************************
 *       Filename:  mydiff.c                                     
 *                                                                 
 *    Description:                                        
 *                                                                 
 *        Version:  1.0                                            
 *        Created:  2016年08月14日 15时49分30秒                                 
 *       Revision:  none                                           
 *       Compiler:  gcc                                           
 *                                                                 
 *         Author:  Ruan Huabin                                      
 *          Email:  ruanhuabin@gmail.com                                        
 *        Company:  HPC tsinghua                                      
 *                                                                 
 *******************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
int main ( int argc, char *argv[] )
{ 
    FILE *file1 = fopen(argv[1], "rb");
    FILE *file2 = fopen(argv[2], "rb");



    int ITEMS_TO_READ = 1024 * 1024 * 16;
    float *buffer1 = (float *)malloc(ITEMS_TO_READ * sizeof(float));
    float *buffer2 = (float *)malloc(ITEMS_TO_READ * sizeof(float));

    size_t num1 = fread(buffer1, sizeof(float), ITEMS_TO_READ, file1);
    size_t num2 = fread(buffer2, sizeof(float), ITEMS_TO_READ, file2);

    int num;
    unsigned int count = 0;
    while(num1 > 0 && num2 > 0)
    {
        num = num1 < num2 ? num1: num2;
        for(int i = 0; i < num; i ++ )
        {
            if(fabsf(buffer1[i] - buffer2[i]) > 10E-6)
            {
                char *c1 = (char *)&buffer1[i];
                char *c2 = (char *)&buffer2[i];
                printf("count = 0x%08X, n1 = %f, n2 = %f\n", count, buffer1[i], buffer2[i]);
                for(int j = 0; j < 4; j ++)
                {
                    printf("%02x ", c1[j] & 0xFF);
                }
                printf("\n");
                for(int j = 0; j < 4; j ++)
                {
                    printf("%02x ", c2[j] & 0xFF);
                }

                printf("\n");
            }
            count = count + 4;
        }
    }


    return EXIT_SUCCESS;
}

