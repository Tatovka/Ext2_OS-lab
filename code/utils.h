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

struct __attribute__((packed))
inode {
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

void alloc_error() {
    perror("failed to allocate memory");
    exit(1);
}

int write_full(int fd, char* buf, size_t size) {
    size_t written = 0;
    while(written < size) {
        int s = write(fd, buf + written, size - written);
        if (s < 0) return -1;
        written += s;
    }
    return size;
}

int pread_full(int fd, char* buf, size_t size, size_t offset) {
    size_t read = 0;
    while(read < size) {
        int s = pread(fd, buf + read, size - read, offset + read);
        if (s < 0) return -1;
        if (s == 0) return read;
        read += s;
    }
    return size;
}

struct ext2 get_fs_data(int fs) {
    static char sbData[SB_LEN];
    int cnt = pread_full(fs, sbData, SB_LEN, SB_LOC);

    if (cnt < SB_LEN) {
        perror("failed to read superblock");
        exit(1);
    }

    struct ext2 res;
    int* sbData_i = (int*) sbData;
    short* sbData_s = (short*) sbData;

    res.inodes_cnt = le32toh(sbData_i[0]); 
    res.block_cnt = le32toh(sbData_i[1]); 
    res.blocks_per_group = le32toh(sbData_i[8]);
    res.inodes_per_group = le32toh(sbData_i[10]);
    res.inode_size = le16toh(sbData_s[44]);
    res.block_size = 1024 << le32toh(sbData_i[6]);
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
    static char descData[DESC_SIZE];
    int cnt = pread_full(fs, descData, DESC_SIZE, desc_table_loc + group * DESC_SIZE);
    if (cnt < DESC_SIZE) {
        perror("failed to read super block descriptor");
        exit(1);
    }
    return le32toh(((int*) descData)[2]);
}

struct block_tree {
    int isDirect;
    int block;
    //if indirect
    struct block_tree** children; 
};

struct block_tree* build_tree(int level, int block, int fs, const struct ext2* fsData) {
    if (block == 0 && level) return NULL;
    //block == 0 && level == 0 --- sparse file null block

    struct block_tree* node = (struct block_tree*) calloc(1, sizeof(struct block_tree));
    if (node == NULL)
        alloc_error();

    node->block = block;
    if (level == 0) { //direct
        *node = (struct block_tree){1, block, NULL};
        return node;
    }

    node->isDirect = 0;
    node->children = (struct block_tree**) calloc(fsData->block_size, sizeof(struct block_tree*));
    if (node->children == NULL)
        alloc_error();

    ll blockStart = (ll)block * fsData->block_size;
    int* blockData = (int*) malloc(fsData->block_size);
    if (blockData == NULL)
        alloc_error();

    int cnt = pread_full(fs, (char*) blockData, fsData->block_size, blockStart);
    if (cnt < 0) {
        perror("failed to read block");
        exit(1);
    }

    for (int i = 0; i < cnt / sizeof(int); i++) {
        node->children[i] = build_tree(level - 1, le32toh(blockData[i]), fs, fsData);
    }

    free(blockData);
    return node;
}

struct block_tree get_block_tree(const struct inode* inode, int fs, struct ext2* fsData) {
    struct block_tree** firstLayer = (struct block_tree**) calloc(15, sizeof(struct block_tree*));
    if (firstLayer == NULL)
        alloc_error();

    for (int i = 0; i < 12; ++i) {
        firstLayer[i] = build_tree(0, inode->direct[i], fs, fsData);
    }
    firstLayer[12] = build_tree(1, inode->indirect1, fs, fsData);
    firstLayer[13] = build_tree(2, inode->indirect2, fs, fsData);
    firstLayer[14] = build_tree(3, inode->indirect3, fs, fsData);

    struct block_tree root = {0, 0, firstLayer};
    return root;
}

void free_tree(struct block_tree* root, int blockSz) {
    if (root == NULL) return;
    if (!root->isDirect) {
        for (int i = 0; i < blockSz; ++i) {
            free_tree(root->children[i], blockSz);
        }
        free(root->children);
    }
    free(root);
}

#if __BYTE_ORDER != __LITTLE_ENDIAN
void fix_byte_order(struct inode* node) {
    if (node == NULL) return;

    node->mode   = le16toh(node->mode);
    node->uid    = le16toh(node->uid);
    node->gid    = le16toh(node->gid);
    node->hlinks = le16toh(node->hlinks);

    node->size_low      = le32toh(node->size_low);
    node->access_time   = le32toh(node->access_time);
    node->creation_time = le32toh(node->creation_time);
    node->mod_time      = le32toh(node->mod_time);
    node->deletion_time = le32toh(node->deletion_time);
    node->sectors       = le32toh(node->sectors);
    node->flags         = le32toh(node->flags);
    node->osSpec        = le32toh(node->osSpec);

    for (int i = 0; i < DIRECT_PTRS; i++) {
        node->direct[i] = le32toh(node->direct[i]);
    }

    node->indirect1 = le32toh(node->indirect1);
    node->indirect2 = le32toh(node->indirect2);
    node->indirect3 = le32toh(node->indirect3);

    node->gen       = le32toh(node->gen);
    node->acl       = le32toh(node->acl);
    node->size_high = le32toh(node->size_high);
}
#endif

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
    if (inodeData == NULL)
        alloc_error();

    int cnt = pread_full(fs, inodeData, fsData.inode_size, inodeStart);
    if (cnt < fsData.inode_size) {
        perror("failed to read inode entry");
        exit(1);
    }

    struct inode res;
    memcpy(&res, inodeData, 112);
    free(inodeData);
    #if __BYTE_ORDER != __LITTLE_ENDIAN
        fix_byte_order(&res);
    #endif

    res.number = inode;
    res.blocks = res.sectors / (fsData.block_size >> 9);
    res.size = (ll)res.size_high << 32 | res.size_low;
    return res;
}
