CC = gcc
UTILS = util1 util2 util3

util%: code/task%.c
	$(CC) $< -o $@

run%: ARGS = $(FS) $(INODE)
run3: ARGS = $(FILE)

run%: util%
	./$< $(ARGS) $(if $(OUTPUT),> $(OUTPUT))

FSNAME = ext2-2.img
MNT = mnt
DIR2 = $(MNT)/dir2/dir2-2/empty_folder/emptier_folder/voobshe_empty_folder_zhestb/empty_folder
HS = hs

test:
	touch $(FSNAME)
	truncate --size 16G $(FSNAME)
	mkfs.ext2 $(FSNAME) -b 2K
	mkdir $(MNT)
	mkdir hs
	sudo mount -t ext2 $(FSNAME) $(MNT)
	sudo chmod 777 $(MNT)
	mkdir $(MNT)/dir1
	mkdir -p $(DIR2)
	cp test_files/bigFile $(MNT)/dir1/bigFile
	truncate --size 5G $(MNT)/dir1/bigFile
	cp test_files/song.mp3 $(DIR2)/song.mp3
	cp test_files/video.mp4 $(DIR2)/video.mp4
	sha512sum $(DIR2)/song.mp3 > $(HS)/song
	sha512sum $(DIR2)/video.mp4 > $(HS)/video
	export IBF=`ls -lai mnt/dir1 | grep bigFile | sed 's/ .*//'` &&\
	export ISONG=`ls -lai $(DIR2) | grep song.mp3 | sed 's/ .*//'` &&\
	export IVIDEO=`ls -lai $(DIR2) | grep video.mp4 | sed 's/ .*//'` &&\
	sudo umount $(MNT) &&\
	make run1 FS=$(FSNAME) INODE=$$IBF > testout11 &&\
	make run1 FS=$(FSNAME) INODE=$$ISONG > testout12 &&\
	make run1 FS=$(FSNAME) INODE=$$IVIDEO > testout13 &&\
	make run2 FS=$(FSNAME) INODE=$$ISONG | sha512sum > $(HS)/song2 &&\
	make run2 FS=$(FSNAME) INODE=$$IVIDEO | sha512sum > $(HS)/video2
	sha512sum $(HS)/song2 $(HS)/song
	sha512sum $(HS)/video2 $(HS)/video

clear:
	rm -f $(UTILS)
	rm -f $(FSNAME)
	rm -rf $(MNT)
	rm -rf $(HS)
	rm testout*