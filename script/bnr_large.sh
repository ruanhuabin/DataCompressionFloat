#!/bin/bash - 
#===============================================================================
#
#          FILE:  bnr.sh
# 
#         USAGE:  ./bnr.sh 
# 
#   DESCRIPTION:  
# 
#       OPTIONS:  ---
#  REQUIREMENTS:  ---
#          BUGS:  ---
#         NOTES:  ---
#        AUTHOR: YOUR NAME (), 
#       COMPANY: 
#       CREATED: 2016年08月11日 10时36分03秒 CST
#      REVISION:  ---
#===============================================================================

cd ..
make clean
sleep 1

make

if [ $? -ne 0 ]
then
    cd script/
    exit
fi

cd script/

N=8

if [ $# -eq 2 ]
then
    N=$1
fi

./run_large_full_test.sh $N 2>/dev/null


