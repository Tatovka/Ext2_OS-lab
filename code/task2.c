#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static ll read_bytes = 0;

void print_tree(struct inode* inode, struct block_tree* root, const struct ext2* fsData, int fs, char* buf) {
    if (root == NULL) {
        return;
    }

    if (!root->isDirect) {
        for (int i = 0; i < fsData->block_size / sizeof(int); i++) {
            print_tree(inode, root->children[i], fsData, fs, buf);
        }
        return;
    }

    if (root->block == 0) { //sparse file null block
        for (int i = 0; i < fsData->block_size; ++i) buf[i] = 0;
    } else {
        int cnt = pread_full(fs, buf, fsData->block_size, (ll)root->block * fsData->block_size);
        if (cnt < fsData->block_size) {
            perror("failed to read block ");
            fprintf(stderr, "%i", root->block);
            exit(1);
        }
    }

    int writeSz = fsData->block_size;
    if (read_bytes >= inode->size) return;
    if (read_bytes + fsData->block_size >= inode->size) {
        writeSz = inode->size - read_bytes;
    }

    int cnt = write_full(STDOUT_FILENO, buf, writeSz);
    if (cnt < writeSz) {
        fprintf(stderr, "failed to write block %i of size %d", root->block, writeSz);
        exit(1);
    }
    read_bytes += writeSz;
}

int main(int argc, char** argv) {
    char* fsPath = argv[1];
    int inode = atoi(argv[2]);

    int fs = open(fsPath, O_RDONLY);
    if (fs < 0) {
        perror("unable to open file system");
        return 1;
    }

    struct ext2 fsData = get_fs_data(fs);
    struct inode metadata = get_inode(inode, fs, fsData);
    struct block_tree root = get_block_tree(&metadata, fs, &fsData);
    
    char* buf = malloc(fsData.block_size);
    if (buf == NULL)
        alloc_error();
    
    for (int i = 0; i < 15; ++i) {
        print_tree(&metadata, root.children[i], &fsData, fs, buf);
    }

    if (read_bytes < metadata.size) {
       for (int i = 0; i < fsData.block_size; ++i) buf[i] = 0;
    }
    while (read_bytes < metadata.size) {
        int writeSz = fsData.block_size;
        if (read_bytes + fsData.block_size >= metadata.size) {
            writeSz = metadata.size - read_bytes;
        }
        int cnt = write(STDOUT_FILENO, buf, writeSz);
        if (cnt < 0) {
            fprintf(stderr, "failed to write block");\
            return 1;
        }
        read_bytes += cnt;
    }

    for (int i = 0; i < 15; ++i) {
        free_tree(root.children[i], fsData.block_size);
    }
    free(root.children);
    free(buf);
    close(fs);

    return 0;
}