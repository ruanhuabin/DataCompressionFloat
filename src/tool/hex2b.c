/*******************************************************************
 *       Filename:  hex2b.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2016年08月08日 14时35分55秒
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
#include <ctype.h>
#include <math.h>

const char *hex2bTable[16] = { "0000", "0001", "0010", "0011", "0100", "0101",
                               "0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110",
                               "1111"

                             };

int getIndex(char c)
{
    c = toupper(c);
    int index = 0;

    if (c >= '0' && c <= '9')
    {
        index = c - '0';
    }

    else if (c >= 'A' && c <= 'F')
    {
        index = c - 'A' + 10;
    }

    return index;
}

static int pow2i(int i)
{
    int result = 1 << i;
    return result;
}
int b2dec(const char *bin)
{
    int len = strlen(bin);
    int sum = 0;
    int m = 0;
    int intValue = 0;

    for (int i = len - 1; i >= 0; i--)
    {
        intValue = bin[i] - '0';
        m = intValue * pow2i(7 - i);
        sum = sum + m;
        /*
         *printf("i = %d, power2i(7-i) = %d, intValue = %d, m = %d, sum = %d \n", i, pow2i(7-i), intValue, m, sum);
         */
    }

    return sum;
}

float b2f(const char *bin)
{
    int len = strlen(bin);
    float sum = 1.0;

    for (int i = 0; i < len; i++)
    {
        float bValue = (float) (bin[i] - '0');
        float m = bValue * powf(2.0, (i + 1) * (-1.0));
        /*
         *printf("i = %d, powf() = %f, bValue = %f, m = %f, sum = %f \n", i, powf(2.0, (i + 1) * (-1)), bValue, m, sum);
         */
        sum = sum + m;
    }

    return sum;
}
void printNumDetail(const char *binResult)
{
    char *sign = binResult[0] == '0' ? "+" : "-";
    char exponent[9];
    char mantisa[24];
    memset(exponent, '\0', sizeof(exponent));
    memset(mantisa, '\0', sizeof(mantisa));
    strncpy(exponent, binResult + 1, 8);
    strncpy(mantisa, binResult + 9, 23);
    int expResult = b2dec(exponent);
    float mantisaResult = b2f(mantisa);
    printf(
        "sign: %s\nexponent: %s --> %d [%d - 127 = %d]\nmantisa: %s --> %f\n",
        sign, exponent, (expResult - 127), expResult, expResult - 127,
        mantisa, mantisaResult);
    float floatNum = pow2i(expResult - 127) * mantisaResult;
    floatNum = strcmp(sign, "+") == 0 ? floatNum : floatNum * (-1.0);
    printf("Float Format: %s 2^%d x %f = %d * %f = %f\n", sign, expResult - 127,
           mantisaResult, pow2i(expResult - 127), mantisaResult, floatNum);
}
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf(
            "Usage: %s <float number1> [float number2] [float number3] ... [float numberN]\n",
            argv[0]);
        printf("E.G: %s <hexnumber>\n", argv[0]);
        exit(-1);
    }

    char hexNum[11] = "0x00000000";
    int len = strlen(hexNum);
    char *inputHexNum = argv[1];
    int j = len - 1;

    for (int i = strlen(inputHexNum) - 1; i >= 0; i--)
    {
        hexNum[j] = inputHexNum[i];
        j--;
    }

    printf("standard hex num = %s\n", hexNum);
    char binResult[33];
    memset(binResult, '\0', sizeof(binResult));

    for (int i = 2; i < len; i++)
    {
        int index = getIndex(hexNum[i]);
        strcat(binResult, hex2bTable[index]);
    }

    printf("%s:", hexNum);

    for (int i = 0; i < strlen(binResult); i++)
    {
        printf("%c", binResult[i]);

        if ((i + 1) % 8 == 0)
        {
            printf(" ");
        }
    }

    printf("\n");
    printNumDetail(binResult);
    return EXIT_SUCCESS;
}

