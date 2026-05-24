CC = gcc
UTILS = util1 util2 util3

util%: code/task%.c code/utils.h
	$(CC) $< -o $@

run%: ARGS = $(FS) $(INODE)
run3: ARGS = $(FILE)

run%: util%
	valgrind ./$< $(ARGS) $(if $(OUTPUT),> $(OUTPUT))

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
	echo $$(sha256sum $(DIR2)/song.mp3 | grep -oE '^\w*') testout21.mp3 > $(HS)/song
	echo $$(sha256sum $(DIR2)/video.mp4 | grep -oE '^\w*') testout22.mp4 > $(HS)/video
	cat $(MNT)/dir1/bigFile | sha256sum > $(HS)/bf
	export IBF=`ls -lai mnt/dir1 | grep bigFile | $(INODE_GREP)` &&\
	export ISONG=`ls -lai $(DIR2) | grep song.mp3 | $(INODE_GREP)` &&\
	export IVIDEO=`ls -lai $(DIR2) | grep video.mp4 | $(INODE_GREP)` &&\
	export IDIR1=`ls -lai $(MNT) | grep dir1 | $(INODE_GREP)` &&\
	export IDIR2=`ls -lai $(MNT) | grep dir2 | $(INODE_GREP)` &&\
	export IDIR3=`ls -lai $(MNT)/dir2/dir2-2/empty_folder/emptier_folder/voobshe_empty_folder_zhestb | grep empty_folder | $(INODE_GREP)` &&\
	sudo umount $(MNT) &&\
	valgrind ./util1 $(FSNAME) $$IBF > testout11 &&\
	valgrind ./util1 $(FSNAME) $$ISONG > testout12 &&\
	valgrind ./util1 $(FSNAME) $$IVIDEO > testout13 &&\
	valgrind ./util2 $(FSNAME) $$ISONG > testout21.mp3 &&\
	valgrind ./util2 $(FSNAME) $$IVIDEO > testout22.mp4 &&\
	valgrind ./util2 $(FSNAME) $$IDIR1 > testout31 &&\
	valgrind ./util2 $(FSNAME) $$IDIR2 > testout32 &&\
	valgrind ./util2 $(FSNAME) $$IDIR3 > testout33 &&\
	valgrind ./util2 $(FSNAME) $$IBF | sha256sum > $(HS)/bf2&&\
	valgrind ./util3 testout31 > testout34 && valgrind ./util3 testout32 > testout35 && valgrind ./util3 testout33 > testout36
	sha256sum -c $(HS)/song
	sha256sum -c $(HS)/video
	echo $$(sha256sum $(HS)/bf | grep -oE '^\w*') $(HS)/bf2 > $(HS)/bf
	sha256sum -c $(HS)/bf
	
clear:
	rm -f $(UTILS)
	rm -f $(FSNAME)
	rm -rf $(MNT)
	rm -rf $(HS)
	rm testout*