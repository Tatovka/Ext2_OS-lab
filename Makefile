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
INODE_GREP = grep -oE '^ *[0-9]+'

test:
	make util1
	make util2
	make util3
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
	export IBF=`ls -lai mnt/dir1 | grep bigFile | $(INODE_GREP)` &&\
	export ISONG=`ls -lai $(DIR2) | grep song.mp3 | $(INODE_GREP)` &&\
	export IVIDEO=`ls -lai $(DIR2) | grep video.mp4 | $(INODE_GREP)` &&\
	export IDIR1=`ls -lai $(MNT) | grep dir1 | $(INODE_GREP)` &&\
	export IDIR2=`ls -lai $(MNT) | grep dir2 | $(INODE_GREP)` &&\
	export IDIR3=`ls -lai $(MNT)/dir2/dir2-2/empty_folder/emptier_folder/voobshe_empty_folder_zhestb | grep empty_folder | $(INODE_GREP)` &&\
	sudo umount $(MNT) &&\
	./util1 $(FSNAME) $$IBF > testout11 &&\
	./util1 $(FSNAME) $$ISONG > testout12 &&\
	./util1 $(FSNAME) $$IVIDEO > testout13 &&\
	./util2 $(FSNAME) $$ISONG > testout21.mp3 &&\
	./util2 $(FSNAME) $$IVIDEO > testout22.mp4 &&\
	./util2 $(FSNAME) $$IDIR1 > testout31 &&\
	./util2 $(FSNAME) $$IDIR2 > testout32 &&\
	./util2 $(FSNAME) $$IDIR3 > testout33 &&\
	./util2 $(FSNAME) $$ISONG | sha512sum > $(HS)/song2&&\
	./util2 $(FSNAME) $$IVIDEO | sha512sum > $(HS)/video2
	./util3 testout31 > testout34 && ./util3 testout32 > testout35 && ./util3 testout33 > testout36

clear:
	rm -f $(UTILS)
	rm -f $(FSNAME)
	rm -rf $(MNT)
	rm -rf $(HS)
	rm testout*