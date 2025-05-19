#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INODES 8
#define MAX_BLOCKS 8
#define MAX_NAME_LEN 8
#define MAX_ENTRIES 4

typedef enum { FREE, FILE_TYPE, DIR_TYPE } FileType;

typedef struct {
    FileType type;
    int data_block;
    int ref_count;
} Inode;

typedef struct {
    char name[MAX_NAME_LEN];
    int inode_num;
} DirEntry;

typedef union {
    char file_data;
    DirEntry dir_entries[MAX_ENTRIES];
} DataBlock;

typedef struct {
    char inode_bitmap[MAX_INODES];
    Inode inodes[MAX_INODES];
    char block_bitmap[MAX_BLOCKS];
    DataBlock blocks[MAX_BLOCKS];
    int dir_count;
    int file_count;
} FileSystem;

void init_fs(FileSystem *fs) {
    memset(fs->inode_bitmap, 0, MAX_INODES);
    memset(fs->block_bitmap, 0, MAX_BLOCKS);

    for (int i = 0; i < MAX_INODES; i++) {
        fs->inodes[i].type = FREE;
        fs->inodes[i].data_block = -1;
        fs->inodes[i].ref_count = 0;
    }

    fs->inode_bitmap[0] = 1;
    fs->inodes[0].type = DIR_TYPE;
    fs->inodes[0].data_block = 0;
    fs->inodes[0].ref_count = 2;

    fs->block_bitmap[0] = 1;

    strcpy(fs->blocks[0].dir_entries[0].name, ".");
    fs->blocks[0].dir_entries[0].inode_num = 0;

    strcpy(fs->blocks[0].dir_entries[1].name, "..");
    fs->blocks[0].dir_entries[1].inode_num = 0;

    fs->dir_count = 1;
    fs->file_count = 0;
}

int find_free_inode(FileSystem *fs) {
    for (int i = 0; i < MAX_INODES; i++) {
        if (fs->inode_bitmap[i] == 0) {
            return i;
        }
    }
    return -1;
}

int find_free_block(FileSystem *fs) {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (fs->block_bitmap[0] == 0) {
            return i;
        }
    }
    return -1;
}

int find_in_dir(FileSystem *fs, int dir_inode, const char *name) {
    int dir_block = fs->inodes[dir_inode].data_block;

    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (strcmp(fs->blocks[dir_block].dir_entries[i].name, name) == 0) {
            return fs->blocks[dir_block].dir_entries[i].inode_num;
        }
    }

    return -1;
}

int add_to_dir(FileSystem *fs, int dir_inode, const char *name, int inode_num) {
    int dir_block = fs->inodes[dir_inode].data_block;

    for (int i = 2; i < MAX_ENTRIES; i++) {
        if (fs->blocks[dir_block].dir_entries[i].name[0] == '\0') {
            strcpy(fs->blocks[dir_block].dir_entries[i].name, name);
            fs->blocks[dir_block].dir_entries[i].inode_num = inode_num;
            return 0;
        }
    }
    return -1;
}

int create(FileSystem *fs, const char *name) {
    int dir_inode = 0;

    if (find_in_dir(fs, dir_inode, name) != -1) {
        return -1;
    }

    int new_inode = find_free_inode(fs);
    if (new_inode == -1) {
        return -1;
    }

    fs->inode_bitmap[new_inode] = 1;
    fs->inodes[new_inode].type = FILE_TYPE;
    fs->inodes[new_inode].data_block = -1;
    fs->inodes[new_inode].ref_count = 1;

    if (add_to_dir(fs, dir_inode, name, new_inode) == -1) {
        fs->inode_bitmap[new_inode] = 0;
        return -1;
    }

    fs->file_count++;
    return new_inode;
}

int mkdir(FileSystem *fs, const char *name) {
    int parent_inode = 0;

    if (find_in_dir(fs, parent_inode, name) != -1) {
        return -1;
    }

    int new_inode = find_free_inode(fs);
    if (new_inode == -1) {
        return -1;
    }

    int new_block = find_free_block(fs);
    if (new_block == -1) {
        return -1;
    }

    fs->inode_bitmap[new_inode] = 1;
    fs->inodes[new_inode].type = DIR_TYPE;
    fs->inodes[new_inode].data_block = new_block;
    fs->inodes[new_inode].ref_count = 2;

    fs->block_bitmap[new_block] = 1;

    strcpy(fs->blocks[new_block].dir_entries[0].name, ".");
    fs->blocks[new_block].dir_entries[0].inode_num = new_inode;

    strcpy(fs->blocks[new_block].dir_entries[1].name, "..");
    fs->blocks[new_block].dir_entries[1].inode_num = parent_inode;

    for (int i = 2; i < MAX_ENTRIES; i++) {
        fs->blocks[new_block].dir_entries[1].name[0] = '\0';
    }

    if (add_to_dir(fs, parent_inode, name, new_inode) == -1) {
        fs->inode_bitmap[new_inode] = 0;
        fs->block_bitmap[new_block] = 0;
        return -1;
    }

    fs->inodes[parent_inode].ref_count++;

    fs->dir_count++;
    return new_inode;
}

int write_to_file(FileSystem *fs, const char *name, char data) {
    int file_inodes = find_in_dir(fs, 0, name);

    if (file_inodes == -1) {
        return -1;
    }

    if (fs->inodes[file_inodes].type != FILE_TYPE) {
        return -1;
    }

    int data_block = fs->inodes[file_inodes].data_block;
    if (data_block == -1) {
        data_block = find_free_block(fs);
        if (data_block == -1) {
            return -1;
        }
        fs->block_bitmap[data_block] = 1;
        fs->inodes[file_inodes].data_block = data_block;
    }

    fs->blocks[data_block].file_data = data;
    return 0;
}