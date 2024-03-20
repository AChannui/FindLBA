#!/bin/bash

function run_test {
    local FILE=${1:-file-50k}
    local FSTYPE=${2:-ext3}

    ./create_image.sh ${FSTYPE}
    make mount
    cp ${FILE} mnt
    make unmount
    /mnt/c/Users/alex/College/CS4398/Assignment2/cmake-build-wsl-opensuse/Assignment2 -d disk.img
}

for file in file-{50k,5M,5G}; do 
    for fstype in ext{2,3}; do
	run_test $file $fstype | tee output-$file-$fstype.txt
    done
done

