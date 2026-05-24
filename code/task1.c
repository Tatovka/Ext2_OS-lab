#include "utils.h"
#include <time.h>

void print_inode(const struct inode *node) {
    printf("========== INODE INFO (Number: %d) ==========\n", node->number);

    printf("File Mode:     0o%06o (", node->mode);
    if ((node->mode & 0xF000) == 0x8000) printf("Regular file");
    else if ((node->mode & 0xF000) == 0x4000) printf("Directory");
    else if ((node->mode & 0xF000) == 0xA000) printf("Symbolic link");
    else printf("Other type");
    printf(")\n");

    printf("UID / GID:     %d / %d\n", node->uid, node->gid);
    printf("Size:          %lld bytes\n", node->size);
    printf("Hard Links:    %d\n", node->hlinks);
    printf("Sectors (512B): %d\n", node->sectors);
    printf("Blocks count:  %d\n", node->blocks);
    
    printf("---------------------------------------------\n");

    time_t raw_time;

    raw_time = (time_t)node->access_time;
    printf("Access Time:   %s", raw_time ? ctime(&raw_time) : "Not available\n");

    raw_time = (time_t)node->creation_time;
    printf("Creation Time: %s", raw_time ? ctime(&raw_time) : "Not available\n");

    raw_time = (time_t)node->mod_time;
    printf("Modify Time:   %s", raw_time ? ctime(&raw_time) : "Not available\n");

    raw_time = (time_t)node->deletion_time;

    if (node->deletion_time == 0) {
        printf("Deletion Time: Active (Not deleted)\n");
    } else {
        printf("Deletion Time: %s", ctime(&raw_time));
    }
    printf("=============================================\n");
}

static ll printed_bytes = 0;
void print_tree(struct inode* inode, struct block_tree* root, int level, const struct ext2* fsData) {
    
    if (root == NULL) {
        return;
    }

    static const char* padding[] = {"├──", "│   ├", "│   │   ├", "│   │   │   ├"};
    if (printed_bytes < inode->size) {
        printf("%s%u\n", padding[level], (uint)root->block);
    }

    if (root->isDirect) {
        printed_bytes += fsData->block_size;
        return;
    }

    for (int i = 0; i < fsData->block_size / sizeof(int); i++) {
        print_tree(inode, root->children[i], level + 1, fsData);
    }
}

signed main(int argc, char** argv) {
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
    close(fs);
    print_inode(&metadata);

    printf("INODE BLOCK TREE\n");
    for (int i = 0; i < 15; ++i) {
        print_tree(&metadata, root.children[i], 0, &fsData);
        free_tree(root.children[i], fsData.block_size);
    }
    free(root.children);
    
    return 0;
}