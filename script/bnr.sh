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
./middle_full.sh $1

