#!/bin/bash 
#===============================================================================
#
#          FILE:  test_mt_tarx.sh
# 
#         USAGE:  ./test_mt_tarx.sh 
# 
#   DESCRIPTION:  
# 
#       OPTIONS:  ---
#  REQUIREMENTS:  ---
#          BUGS:  ---
#         NOTES:  ---
#        AUTHOR: Ruan Huabin <ruanhuabin@gmail.com>
#       COMPANY: Tsinghua University
#       CREATED: 2017年07月14日 16时03分33秒 CST
#      REVISION:  ---
#===============================================================================
set -x 
set -e
set -o nounset                              # Treat unset variables as an error

cpuNum=`cat /proc/cpuinfo |grep "physical id"|sort |uniq|wc -l`
echo "cpuNum = $cpuNum"

totalCPUCores=`cat /proc/cpuinfo |grep "processor"|wc -l`
echo "totalCPUCores=$totalCPUCores"

#threadNum=(($cpuNum*$totalCPUCores))
#threadNum=`expr $cpuNum\*$totalCPUCores`
let "threadNum = cpuNum * totalCPUCores"
echo "threadNum = $threadNum"
#((threadNum2=${cpuNum}*${totalCPUCores}))
#echo "threadNum2 = $threadNum2"

#./mrc_tarx_c -i zipfilelist.txt -t zip -o /home/ruanhuabin/tmp/ -b 0 -d 0 -n 2 -t zip

fileNames=`ls ../data/*.mrc`
OLD_IFS="$IFS"
IFS=" "
fileList=($fileNames)
IFS=$OLD_IFS

rm filelist.txt
for n in ${fileList[@]}
do
    echo "$n" >>filelist.txt
done
