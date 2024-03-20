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

clean: 
	rm -f file* disk*.img
