CC = gcc
UTILS = util1 util2 util3

util%: code/task%.c
	$(CC) $< -o $@

run%: ARGS = $(FS) $(INODE)
run3: ARGS = $(FILE)

run%: util%
	./$< $(ARGS) $(if $(OUTPUT),> $(OUTPUT))

clear:
	clear:
	rm -f $(UTILS)