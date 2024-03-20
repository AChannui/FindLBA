#!/bin/bash

FILE=${1:-file-50k}
FSTYPE=${2:-ext3}

./create_image.sh ${FSTYPE}
make mount
cp ${FILE} mnt
make unmount
/tmp/tmp.VD00t0lx7J/cmake-build-virtualbox-opensuse/Assignment2 -d disk.img
