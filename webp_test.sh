#!/bin/bash

./create_image.sh
make mount
cp ${1:-image.webp} mnt
make unmount
/mnt/c/Users/alex/College/CS4398/Assignment2/cmake-build-wsl-opensuse/Assignment2 -vd disk.img
display recovered_image.webp
