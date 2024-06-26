all: file-50k file-5M file-5G disk.img

file-50k:
	dd if=/dev/urandom of=$@ bs=1k count=50

file-5M:
	dd if=/dev/urandom of=$@ bs=1M count=5

file-5G:
	dd if=/dev/urandom of=$@ bs=1G count=5

disk.img:
	./create_image.sh

.PHONY: mount
mount: disk.img
	sudo losetup --partscan --find --show disk.img
	mkdir -p mnt
	sudo mount /dev/loop0p1 mnt
	sudo chmod a+rwx mnt
	sudo chmod a+rw /dev/loop0*

.PHONY: unmount
unmount: 
	sudo umount /dev/loop0p1
	sudo losetup -d /dev/loop0

copy: file-50k file-5M file-5G
	cp $^ mnt

debugfs: disk.img
	make mount
	/usr/sbin/debugfs /dev/loop0p1
	make unmount

image.webp:
	convert -size 800x800 canvas:white $@

black_image.webp:
	convert -size 800x800 canvas:black $@

gradient.webp:
	convert -size 800x800 gradient:"#089AD7-#FE87BD" -append $@

small.webp:
	convert -quality 100 -define webp:method=0 -define webp:alpha-compression=0 -define webp:lossless=true -size 2048x2048 gradient:"#089AD7-#FE87BD" -append $@

medium.webp:
	convert -quality 100 -define webp:method=0 -define webp:alpha-compression=0 -define webp:lossless=true -size 4096x4096 gradient:"#A89AD7-#FE87BD" -append $@

large.webp:
	convert -quality 100 -define webp:method=0 -define webp:alpha-compression=0 -define webp:lossless=true -size 16383x16383 gradient:"#3B0E47-#E8Da19" -append $@

first.webp:
	convert -quality 100 -define webp:method=0 -define webp:alpha-compression=0 -define webp:lossless=true -size 3072x3072 gradient:"#089AD7-#FE87BD" -append $@

clean: 
	rm -f file* disk*.img

