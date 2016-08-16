#!/bin/bash - 
#===============================================================================
#
#          FILE:  genmaskdata.sh
# 
#         USAGE:  ./genmaskdata.sh 
# 
#   DESCRIPTION:  
# 
#       OPTIONS:  ---
#  REQUIREMENTS:  ---
#          BUGS:  ---
#         NOTES:  ---
#        AUTHOR: YOUR NAME (), 
#       COMPANY: 
#       CREATED: 2016年08月14日 22时36分17秒 CST
#      REVISION:  ---
#===============================================================================

for((i=0; i<=32; i++))
do
    ./erasebytes_c -i ../data/stack_small.mrc -o mask${i}bits.mrc -b $i
done

