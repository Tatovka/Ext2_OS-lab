#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define ll long long

struct ext2 {
    int block_size;
    int inodes_cnt;
    int block_cnt;
    int blocks_per_group;
    int inodes_per_group;
    int inode_size;
};
#define DIRECT_PTRS 12
struct inode {
    short mode;
    short uid;
    int size_low;
    int access_time;
    int creation_time;
    int mod_time;
    int deletion_time;
    short gid;
    short hlinks;
    int sectors;
    int flags;
    int osSpec;

    int direct[DIRECT_PTRS];
    int indirect1;
    int indirect2;
    int indirect3;

    int gen;
    int acl;
    int size_high;
    
    int blocks;
    int number;
    ll size;
};

#define SB_LOC 1024
#define SB_LEN 1024

#define DESC_TABLE_LOC (SB_LOC + SB_LEN)
#define DESC_SIZE 32

struct ext2 get_fs_data(int fs) {
    char sbData[SB_LEN];
    int cnt = pread(fs, sbData, 1024, SB_LOC);

    if (cnt < 0) {
        perror("failed to read superblock");
        exit(1);
    }

    struct ext2 res;
    int* sbData_i = (int*) sbData;
    short* sbData_s = (short*) sbData;

    res.inodes_cnt = sbData_i[0]; 
    res.block_cnt = sbData_i[1]; 
    res.blocks_per_group = sbData_i[8];
    res.inodes_per_group = sbData_i[10];
    res.inode_size = sbData_s[44];
    res.block_size = 1024 << sbData_i[6];
    return res;
}

//block group that inode belongs to
int inode_block_group(int inode, struct ext2* fsData) {
    return (inode - 1) / fsData->inodes_per_group;
}

//inode index in block group inode table
int inode_index(int inode, struct ext2* fsData) {
    return (inode - 1) % fsData->inodes_per_group;
}

//starting block of inode table in the given block group
int group_inode_table(int group, int fs, struct ext2 fsData) {
    int desc_table_loc = (fsData.block_size == 1024) ? 2048 : fsData.block_size;
    char descData[SB_LEN];
    pread(fs, descData, 1024, desc_table_loc + group * DESC_SIZE);
    return ((int*) descData)[2];
}

struct block_tree {
    int isDirect;
    int block;
    //if indirect
    struct block_tree** children; 
};

struct block_tree* get_tree(int level, int block, int fs, const struct ext2* fsData) {
    struct block_tree* node = (struct block_tree*) calloc(1, sizeof(struct block_tree));
    node->block = block;
    if (level == 0) { //direct
        *node = (struct block_tree){1, block, NULL};
        return node;
    }
    if (block == 0) return NULL;

    node->isDirect = 0;
    node->children = (struct block_tree**) calloc(fsData->block_size, sizeof(struct block_tree*));

    ll blockStart = (ll)block * fsData->block_size;
    int* blockData = (int*) malloc(fsData->block_size);
    pread(fs, blockData, fsData->block_size, blockStart);

    for (int i = 0; i < fsData->block_size / sizeof(int); i++) {
        node->children[i] = get_tree(level - 1, blockData[i], fs, fsData);
    }

    return node;
}

struct block_tree get_block_tree(const struct inode* inode, int fs, struct ext2* fsData) {
    struct block_tree** firstLayer = (struct block_tree**) calloc(15, sizeof(struct block_tree*));
    for (int i = 0; i < 12; ++i) {
        firstLayer[i] = get_tree(0, inode->direct[i], fs, fsData);
    }
    firstLayer[12] = get_tree(1, inode->indirect1, fs, fsData);
    firstLayer[13] = get_tree(2, inode->indirect2, fs, fsData);
    firstLayer[14] = get_tree(3, inode->indirect3, fs, fsData);
    struct block_tree root = {0, 0, firstLayer};
    return root;
}

#define BLOCK_GROUP_CNT(d) (d.block_cnt / d.blocks_per_group)
struct inode get_inode(int inode, int fs, struct ext2 fsData) {
    int group = inode_block_group(inode, &fsData);
    if (group > BLOCK_GROUP_CNT(fsData)) {
        perror("invalid inode");
        exit(1);
    } 

    int tableBlock = group_inode_table(group, fs, fsData);
    int index = inode_index(inode, &fsData);
    ll inodeStart = (ll)index * fsData.inode_size + (ll)tableBlock * fsData.block_size;

    char* inodeData = (char*) malloc(fsData.inode_size);
    
    int cnt = pread(fs, inodeData, fsData.inode_size, inodeStart);
    if (cnt < 0) {
        perror("failed to read inode entry");
        exit(1);
    }
    struct inode res;
    memcpy(&res, inodeData, 112);
    free(inodeData);

    res.number = inode;
    res.blocks = res.sectors / (fsData.block_size >> 9);
    res.size = (ll)res.size_high << 32 | res.size_low;
    return res;
}