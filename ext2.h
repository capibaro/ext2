#ifndef EXT2_H
#define EXT2_H

#define MAGIC 0xdec0de
#define DATA_BLOCK_CNT 4096
#define DATA_BLOCK_SIZE 1024
#define SUPER_BLOCK_SIZE 656
#define FNODE_CNT 1024
#define FNODE_SIZE 32
#define DNODE_CNT 1024
#define DNODE_SIZE 128
#define ROOT_NODE 0
#define ROOT_BLOCK 2
#define SUPER_BLOCK 0
#define DISK_BLOCK_SIZE 512
#define DIRECT_INDEX_SIZE 6
#define FILE_NODE_BEGIN 3

typedef int int32_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

// 656B
typedef struct super_block {
    int32_t magic_num;                  // 幻数
    int32_t free_block_count;           // 空闲数据块数
    int32_t free_inode_count;           // 空闲inode数
    int32_t dir_inode_count;            // 目录inode数
    uint32_t block_map[128];            // 数据块占用位图
    uint32_t inode_map[32];             // inode占用位图
} sp_block;

// 32B
typedef struct inode {
    uint32_t size;              // 文件大小
    uint16_t file_type;         // 文件类型（文件/文件夹）
    uint16_t link;              // 连接数
    uint32_t block_point[6];    // 数据块指针
} file_node;

// 128B
typedef struct dir_item {               // 目录项一个更常见的叫法是 dirent(directory entry)
    uint32_t inode_id;          // 当前目录项表示的文件/目录的对应inode
    uint16_t valid;             // 当前目录项是否有效 
    uint8_t type;               // 当前目录项类型（文件/目录）
    char name[121];             // 目录项表示的文件/目录的文件名/目录名
} dir_node;

typedef enum file_type{
    DIR_T,
    FILE_T
}FileType;

#endif