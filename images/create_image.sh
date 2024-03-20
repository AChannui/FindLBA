#!/bin/bash

rm disk.img
truncate -s 8G disk.img
echo -e "label: dos\n\n2048,+,L" | /usr/sbin/sfdisk disk.img
#echo -e "label: dos\n\n2048,+,L" 
#echo -e "o\nn\n\n\n\n\nw\n" | /usr/sbin/fdisk disk.img
/usr/sbin/mkfs.${1:-ext3} -E offset=$((2048*512)) disk.img 8191M
