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
#define PARENT(i) (i/2)
#define RIGHT(i) (i*2 + 1)
#define LEFT(i) (i*2)
#define EXCHANGE(a,b,t) do{t=a;a=b;b=t;}while(0)
typedef struct _stat_info_t
{
    float n1;
    float n2;
    float err;
    float relativeErr;

}stat_info_t;

unsigned int count = 0;
const unsigned MAX_ITEM_NUM = 4000 * 4000 * 40;

float enlargeTimes = 1.0f;
void usage(char **argv)
{
      printf("\nUsage:\n\n");
      printf("\t%s -a <original file> -b <decompress file> -o <output result file> -k <int num>", argv[0]);
      printf("\nwhere:\n");
      printf("\t");
      printf("-a\toriginal file without being compressed\n\n");
      printf("\t");
      printf("-b\tfile that being decompressed from a compessed file\n\n");
      printf("\t");
      printf("-k\t top K maximum absolutely error point that will be printed to console\n\n");
}

const float ZERO = 1E-7;
void topK(stat_info_t *input, int K, int n)
{
    stat_info_t tmp;
    for(int k = 0; k < K; k ++)
    {
        //printf("k = %d\n", k);
        for(int i = n - 1; i > k; i --)
        {
            if(input[i].err > input[i - 1].err)
            {
                tmp = input[i - 1];
                input[i - 1] = input[i];
                input[i] = tmp;
            }
            else if(fabsf(input[i].err - input[i - 1].err) <= ZERO)
            {
                if(input[i].relativeErr > input[i - 1].relativeErr)
                {
                    tmp = input[i - 1];
                    input[i - 1] = input[i];
                    input[i] = tmp;
                }
            }
        }
    }
}

void get_topn_quick(int32_t arr[],int32_t low,int32_t high,const int32_t topn)
{
    if(low >= high || topn > high)return;
    int32_t i = low,j = high,tmp = arr[i];
    while(i<j)
    {
        while(i<j && arr[j] < tmp)j--;
        if(i<j)arr[i++] = arr[j];
        while(i<j && arr[i] >= tmp)i++;
        if(i<j)arr[j--] = arr[i];
    }
    arr[i] = tmp;
    int32_t n = i - low + 1;
    if (n == topn)return;
    else if (n > topn)
        get_topn_quick(arr, low, i-1, topn);
    else if (n < topn)
        get_topn_quick(arr, i+1, high, topn - n);
}

int comparator(const void *a, const void *b)
{
    stat_info_t *sa = (stat_info_t *)a;
    stat_info_t *sb = (stat_info_t *)b;

    float erra = sa->err;
    float errb = sb->err;

    float relativeErra = sa->relativeErr;
    float relativeErrb = sb->relativeErr;


    /**
     *  Sort in reverse order
     */
    if( erra > errb)
    {
        //fprintf(stderr, "%f:%f:%d\n", erra, errb, -1);
        return -1;
    }
    else if(erra < errb)
    {
        //fprintf(stderr, "%f:%f:%d\n", erra, errb, 1);
        return 1;
    }
    else if(fabsf(erra - errb) <= 10E-5) /* When two err equal, then compare coresponding relative err */
    {
       // fprintf(stderr, "%f:%f:%d\n", erra, errb, 0);
        if(relativeErra > relativeErrb)
        {
            return -1;
        }
        else if(relativeErra < relativeErrb)
        {
            return 1;
        }
        else if(fabsf(relativeErra - relativeErrb) <= 10E-5)
        {
            return 0;
        }
    }

    fprintf(stderr,
			"comparator should not run here, but run here, needs to check\n");
    return 0;

}


void calculateDiff(float *buffer1, float *buffer2, int n, stat_info_t *stat_points)
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

        //printf("i = %d, n = %d, count = %d\n", i, n, count);

        stat_points[count].n1 = n1;
        stat_points[count].n2 = n2;
        stat_points[count].err = err * enlargeTimes;
        stat_points[count].relativeErr = relativeErr * enlargeTimes;
        count ++;
    //    printf("%f %f %f %E %f %E\n", n1, n2, err, err, relativeErr, relativeErr);
    }
}
void max_heapify(int32_t arr[],const uint32_t size,uint32_t i)
{
    uint32_t left = LEFT(i),right = RIGHT(i),largest = 0,tmp = 0;
    if(left<size && arr[left] > arr[i])largest = left;
    else largest = i;
    if(right<size && arr[right] > arr[largest])largest = right;
    if(largest != i)
    {
        EXCHANGE(arr[i],arr[largest],tmp);
        max_heapify(arr,size,largest);
    }
}

void min_heapify(int32_t arr[],const uint32_t size,uint32_t i)
{
    uint32_t left = LEFT(i),right = RIGHT(i),largest = 0,tmp = 0;
    if(left<size && arr[left] < arr[i])largest = left;
    else largest = i;
    if(right<size && arr[right] < arr[largest])largest = right;
    if(largest != i)
    {
        EXCHANGE(arr[i],arr[largest],tmp);
        min_heapify(arr,size,largest);
    }
}

void get_topn_heap(int32_t arr[], const int32_t arr_size, const int32_t topn)
{
    int32_t i = topn / 2, tmp = 0;
    // 在[0--topn)范围内构建最小堆,即优先级队列
    while (i >= 0)min_heapify(arr, topn, i--);
    for (i = topn; i < arr_size; ++i)
    {
        if (arr[i] <= arr[0])continue;    //小于最小值,没有判断的必要
        EXCHANGE(arr[0], arr[i], tmp);
        min_heapify(arr, topn, 0);
    }
}

void get_topN(stat_info_t arr[],int32_t low,int32_t high,const int32_t topn)
{
    if(low >= high || topn > high)
    {
        return;
    }
    int32_t i = low,j = high;
    stat_info_t tmp = arr[i];
    while(i<j)
    {
        while(i<j && arr[j].err < tmp.err)
        {
            j--;
        }
        if(i<j)
        {
            arr[i++] = arr[j];
        }
        //while(i<j && (arr[i].err > tmp.err || fabsf(arr[i].err - tmp.err) <= ZERO ))
        while(i<j && (arr[i].err >= tmp.err ))
        {
            i++;
        }
        if(i<j)
        {
            arr[j--] = arr[i];
        }
    }
    arr[i] = tmp;
    int32_t n = i - low + 1;
    if (n == topn)
    {
        return;
    }
    else if (n > topn)
    {
        get_topN(arr, low, i-1, topn);
    }
    else if (n < topn)
    {
        get_topN(arr, i+1, high, topn - n);
    }
}
int main ( int argc, char *argv[] )
{ 

    char *opt_str = "ha:b:k:";
    int opt = 0;
    int rank = 5;
    const char *originalFile = NULL;
    const char *decompressFile = NULL;
    /*
     *const char *outputResultFile = NULL;
     */


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
                /*
                 *case 'o':
                 *        outputResultFile = optarg;
                 *        break;
                 */
                case 'k':
                        rank = atoi(optarg);
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


    fprintf(stderr, "original File = %s, decompress file = %s, topN = %d\n", originalFile, decompressFile, rank);

    FILE *origInputFile = fopen(originalFile, "rb");
    FILE *decompressInputFile = fopen(decompressFile, "rb");
    //FILE *outputFile = fopen(outputResultFile, "w");

    if( origInputFile == NULL )
    {
        fprintf(stderr, "[%s:%d] open file [%s] failed\n", __FILE__, __LINE__, originalFile);
        exit(-1);
    }
 
    if( decompressInputFile == NULL )
    {
        fprintf(stderr, "[%s:%d] open file [%s] failed\n", __FILE__, __LINE__, decompressFile);
        exit(-1);
    }

    /*
     *if( outputFile == NULL )
     *{
     *    fprintf(stderr, "[%s:%d] open file [%s] failed\n", __FILE__, __LINE__, outputResultFile);
     *    exit(-1);
     *}
     */

    const int ITEMS_TO_READ = 1024 * 1024 * 8;
    /*
     *const int ITEMS_TO_READ = 1024 * 6;
     */
    float *buffer1 = (float *)malloc(ITEMS_TO_READ * sizeof(float));
    float *buffer2 = (float *)malloc(ITEMS_TO_READ * sizeof(float));
    stat_info_t *stat_points = (stat_info_t *)malloc(MAX_ITEM_NUM * sizeof(stat_info_t));
    unsigned int num1 = fread(buffer1, sizeof(float), ITEMS_TO_READ, origInputFile);
    unsigned int num2 = fread(buffer2, sizeof(float), ITEMS_TO_READ, decompressInputFile);


    if(buffer1 == NULL)
    {
        fprintf(stderr, "[%s:%d]: Memory alloc failed, require mem size is: %lu\n", __FILE__, __LINE__, ITEMS_TO_READ * sizeof(float));
        return -1;
    }
    if(buffer2 == NULL)
    {
        fprintf(stderr, "[%s:%d]: Memory alloc failed, require mem size is: %lu\n", __FILE__, __LINE__, ITEMS_TO_READ * sizeof(float));
        return -1;
    }
    if(stat_points == NULL)
    {
        fprintf(stderr, "[%s:%d]: Memory alloc failed, require mem size is: %lu\n", __FILE__, __LINE__, MAX_ITEM_NUM * sizeof(float));
        return -1;
    }
    fprintf(stderr, "num1 = %d, num2 = %d\n", num1, num2);



    unsigned int chunkIndex = 0;
    unsigned int num = 0;
    unsigned int totalPointsRead = 0;
    while(num1 > 0 && num2 > 0)
    {
        num = num1 < num2 ? num1 : num2;
        totalPointsRead += num;
        fprintf(stderr, "chunkIndex = %u, num1 = %u, num2 = %u, totalPointsRead = %u, maxCouldLoad = %u\n", chunkIndex, num1, num2, totalPointsRead, MAX_ITEM_NUM);
        chunkIndex ++;
        calculateDiff(buffer1, buffer2, num, stat_points);
        num1 = fread(buffer1, sizeof(float), ITEMS_TO_READ, origInputFile);
        num2 = fread(buffer2, sizeof(float), ITEMS_TO_READ, decompressInputFile);
    }

    
    fprintf(stderr, "Total Points = %u\n", count);
    //qsort(stat_points, count, sizeof(stat_info_t), comparator);

    topK(stat_points, rank, count); 
    
    /*
     *get_topN(stat_points, 0, count - 1, rank);
     */

    
    /**
     *  Only sort top N elements
     */
    /*
     *qsort(stat_points, rank, sizeof(stat_info_t), comparator);
     */


    float n1 = 0.0f;
    float n2 = 0.0f;
    float err = 0.0f;
    float relativeErr = 0.0f;
    for(int i = 0; i < rank; i ++)
    {
        n1 = stat_points[i].n1;
        n2 = stat_points[i].n2;
        err = stat_points[i].err;
        relativeErr = stat_points[i].relativeErr;
        printf("%f %f %f %E %f %E\n", n1, n2, err, err, relativeErr, relativeErr);
    }
    free(buffer1);
    free(buffer2);
    free(stat_points);
    return EXIT_SUCCESS;
}

