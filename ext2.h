#ifndef EXT2_H
#define EXT2_H

#define MAGIC 0xdec0de
#define SUPER_BLOCK 0
#define SUPER_BLOCK_SIZE 656
#define ROOT_BLOCK 2
#define DATA_BLOCK_CNT 4096
#define DATA_BLOCK_SIZE 1024
#define ROOT_NODE 0
#define DNODE_SIZE 128
#define FILE_NODE_BEGIN 3
#define FNODE_CNT 1024
#define FNODE_SIZE 32
#define FNAME_SIZE 121
#define DIRECT_INDEX_SIZE 6

typedef int int32_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef struct super_block
{
    int32_t magic_num;
    int32_t free_block_count;
    int32_t free_inode_count;
    int32_t dir_inode_count;
    uint32_t block_map[128];
    uint32_t inode_map[32];
} sp_block;

typedef struct inode
{
    uint32_t size;
    uint16_t file_type;
    uint16_t link;
    uint32_t block_point[DIRECT_INDEX_SIZE];
} file_node;

typedef struct dir_item
{
    uint32_t inode_id;
    uint16_t valid;
    uint8_t type;
    char name[FNAME_SIZE];
} dir_node;

typedef enum file_type
{
    DIR_T,
    FILE_T
} FileType;

#endif