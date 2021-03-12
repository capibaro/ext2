#include <stdio.h>
#include <string.h>
#include "disk.h"
#include "ext2.h"
#include "util.h"
#include "sh.h"

sp_block sp;
file_node fn[FNODE_CNT];
dir_node dn;

char disk_buf[DEVICE_BLOCK_SIZE];
char buf[FNAME_SIZE];

void format()
{
    for (int i = 0; i < DATA_BLOCK_CNT; i++)
    {
        memset(disk_buf, 0, DEVICE_BLOCK_SIZE);
        store(disk_buf, i, DEVICE_BLOCK_SIZE);
    }

    read((char *)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
    sp.magic_num = MAGIC;
    sp.free_block_count = DATA_BLOCK_CNT - 3 - FNODE_SIZE * FNODE_CNT / DATA_BLOCK_SIZE;
    sp.free_inode_count = FNODE_CNT - 1;
    sp.dir_inode_count = 1;
    for (int i = 0; i < 3 + FNODE_SIZE * FNODE_CNT / DATA_BLOCK_SIZE; i++)
    {
        map_set(sp.block_map, i);
    }
    map_set(sp.inode_map, ROOT_NODE);
    store((char *)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);

    fn[ROOT_NODE].size = 0;
    fn[ROOT_NODE].file_type = DIR_T;
    fn[ROOT_NODE].link = 0;
    fn[ROOT_NODE].block_point[0] = ROOT_BLOCK;
    inode_set((char *)&fn[ROOT_NODE], ROOT_NODE);
    store((char *)fn, FILE_NODE_BEGIN, FNODE_SIZE * FNODE_CNT);
}

void boot()
{
    char c;
    printf("naive ext2 is booting...\n");
    read((char *)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
    if (sp.magic_num == MAGIC)
    {
        printf("existing file system found\n");
        while (1)
        {
            printf("format disk?(y/n)");
            c = getchar();
            if (c == 'y')
            {
                format();
                break;
            }
            else if (c == 'n')
            {
                break;
            }
            else
            {
                continue;
            }
        }
    }
    else
    {
        printf("no existing file system found\n");
        printf("a new file system will be built\n");
        format();
    }
}

int main(int argc, char const *argv[])
{
    open_disk();
    boot();
    shell();
    close_disk();
    return 0;
}