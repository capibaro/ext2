#include <stdio.h>
#include <string.h>
#include "ext2.h"
#include "disk.h"
#include "util.h"

extern sp_block sp;
extern dir_node dn;

extern char disk_buf[DEVICE_BLOCK_SIZE];

void map_set(uint32_t *map, uint32_t num)
{
    int q, r;
    q = num / 32;
    r = num % 32;
    map[q] |= 1 << (31 - r);
}

void map_reset(uint32_t *map, uint32_t num)
{
    int q, r;
    q = num / 32;
    r = num % 32;
    map[q] &= ~(1 << (31 - r));
}

int map_test(uint32_t *map, uint32_t num)
{
    int q, r;
    q = num / 32;
    r = num % 32;
    return map[q] >> (31 - r) & 1;
}

void store(char *buffer, uint32_t num, uint32_t len)
{
    if (len <= DEVICE_BLOCK_SIZE)
    {
        disk_write_block(num, buffer);
    }
    else
    {
        for (int i = 0; i <= len / DEVICE_BLOCK_SIZE; i++)
        {
            disk_write_block(num + i, buffer + i * DEVICE_BLOCK_SIZE);
        }
    }
}

void read(char *buffer, uint32_t num, uint32_t len)
{
    if (len <= DEVICE_BLOCK_SIZE)
    {
        disk_read_block(num, disk_buf);
        memcpy(buffer, disk_buf, len);
    }
    else
    {
        for (int i = 0; i <= len / DEVICE_BLOCK_SIZE; i++)
        {
            disk_read_block(num + i, disk_buf);
            if (i == len / DEVICE_BLOCK_SIZE)
                memcpy(buffer + i * DEVICE_BLOCK_SIZE, disk_buf, len - i * DEVICE_BLOCK_SIZE);
            else
                memcpy(buffer + i * DEVICE_BLOCK_SIZE, disk_buf, DEVICE_BLOCK_SIZE);
        }
    }
}

void inode_get(char *buffer, uint32_t num)
{
    uint32_t q, r;
    q = num / (DATA_BLOCK_SIZE / FNODE_SIZE) + 3;
    r = num % (DATA_BLOCK_SIZE / FNODE_SIZE);
    read(disk_buf, q, DEVICE_BLOCK_SIZE);
    memcpy(buffer, disk_buf + r * FNODE_SIZE, FNODE_SIZE);
}

void inode_set(char *buffer, uint32_t num)
{
    uint32_t q, r;
    q = num / (DATA_BLOCK_SIZE / FNODE_SIZE) + 3;
    r = num % (DATA_BLOCK_SIZE / FNODE_SIZE);
    read(disk_buf, q, DEVICE_BLOCK_SIZE);
    memcpy(disk_buf + r * FNODE_SIZE, buffer, FNODE_SIZE);
    store(disk_buf, q, DEVICE_BLOCK_SIZE);
}

int lookup(char *buffer, int type, uint32_t *blocks)
{
    for (int i = 0; i < DIRECT_INDEX_SIZE; i++)
    {
        if (blocks[i] == 0)
        {
            return -1;
        }
        read(disk_buf, blocks[i], DEVICE_BLOCK_SIZE);
        for (int j = 0; j < DEVICE_BLOCK_SIZE / DNODE_SIZE; j++)
        {
            memcpy((char *)&dn, disk_buf + j * DNODE_SIZE, DNODE_SIZE);
            if (memcmp(dn.name, buffer, strlen(buffer)) == 0 && dn.type == type && dn.valid == 1)
                return dn.inode_id;
        }
        if (i == DIRECT_INDEX_SIZE - 1)
            return -1;
    }
}

int dir_set(char *buffer, uint32_t *blocks)
{
    dir_node dnode;
    for (int i = 0; i < DIRECT_INDEX_SIZE; i++)
    {
        if (blocks[i] == 0)
        {
            read((char *)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
            blocks[i] = DATA_BLOCK_CNT - sp.free_block_count;
            sp.free_block_count--;
            map_set(sp.block_map, blocks[i]);
            store((char *)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
        }
        read(disk_buf, blocks[i], DEVICE_BLOCK_SIZE);
        for (int j = 0; j < DEVICE_BLOCK_SIZE / DNODE_SIZE; j++)
        {
            memcpy((char *)&dnode, disk_buf + j * DNODE_SIZE, DNODE_SIZE);
            if (dnode.valid == 0)
            {
                memcpy(disk_buf + j * DNODE_SIZE, buffer, DNODE_SIZE);
                store(disk_buf, blocks[i], DEVICE_BLOCK_SIZE);
                return 1;
            }
        }
        if (i == DIRECT_INDEX_SIZE - 1)
            printf("no enough space\n");
    }
}