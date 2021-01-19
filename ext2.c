#include <stdio.h>
#include <string.h>
#include "disk.h"
#include "ext2.h"

char *ls_cmd = "ls";
char *mkdir_cmd = "mkdir";
char *touch_cmd = "touch";
char *cp_cmd = "cp";
char *shutdown_cmd = "shutdown";

sp_block sp;
file_node fn[FNODE_CNT];
dir_node dn;

char disk_buf[512];
char buf[121];

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
    if (len <= DISK_BLOCK_SIZE)
    {
        disk_write_block(num, buffer);
    }
    else
    {
        for (int i = 0; i <= len / DISK_BLOCK_SIZE; i++)
        {
            disk_write_block(num + i, buffer + i * DISK_BLOCK_SIZE);
        }
    }
}

void read(char *buffer, uint32_t num, uint32_t len)
{
    if (len <= DISK_BLOCK_SIZE)
    {
        disk_read_block(num, disk_buf);
        memcpy(buffer, disk_buf, len);
    }
    else
    {
        for (int i = 0; i <= len / DISK_BLOCK_SIZE; i++)
        {
            disk_read_block(num + i, disk_buf);
            if (i == len / DISK_BLOCK_SIZE)
                memcpy(buffer + i * DISK_BLOCK_SIZE, disk_buf, len - i * DISK_BLOCK_SIZE);
            else
                memcpy(buffer + i * DISK_BLOCK_SIZE, disk_buf, DISK_BLOCK_SIZE);
        }
    }
}

void inode_get(char *buffer, uint32_t num)
{
    uint32_t q, r;
    q = num / (DATA_BLOCK_SIZE / FNODE_SIZE) + 3;
    r = num % (DATA_BLOCK_SIZE / FNODE_SIZE);
    read(disk_buf, q, DISK_BLOCK_SIZE);
    memcpy(buffer, disk_buf + r * FNODE_SIZE, FNODE_SIZE);
}

void inode_set(char *buffer, uint32_t num)
{
    uint32_t q, r;
    q = num / (DATA_BLOCK_SIZE / FNODE_SIZE) + 3;
    r = num % (DATA_BLOCK_SIZE / FNODE_SIZE);
    read(disk_buf, q, DISK_BLOCK_SIZE);
    memcpy(disk_buf + r * FNODE_SIZE, buffer, FNODE_SIZE);
    store(disk_buf, q, DISK_BLOCK_SIZE);
}

int lookup(char *buffer, int type, uint32_t *blocks)
{
    for (int i = 0; i < DIRECT_INDEX_SIZE; i++)
    {
        if (blocks[i] == 0)
            break;
        read(disk_buf, blocks[i], DISK_BLOCK_SIZE);
        for (int j = 0; j < DISK_BLOCK_SIZE / DNODE_SIZE; j++)
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
        }
        read(disk_buf, blocks[i], DISK_BLOCK_SIZE);
        for (int j = 0; j < DISK_BLOCK_SIZE / DNODE_SIZE; j++)
        {
            memcpy((char *)&dnode, disk_buf + j * DNODE_SIZE, DNODE_SIZE);
            if (dnode.valid == 0)
            {
                memcpy(disk_buf + j * DNODE_SIZE, buffer, DNODE_SIZE);
                store(disk_buf, blocks[i], DISK_BLOCK_SIZE);
                return 1;
            }
        }
        if (i == DIRECT_INDEX_SIZE - 1)
            printf("no enough space\n");
    }
}

int ls()
{
    int dir, q, r;
    uint32_t *blocks;
    dir = ROOT_NODE;
    inode_get((char *)&fn[dir], dir);
    blocks = fn[dir].block_point;
    if (getchar() != '\n')
    {
        scanf("%s", buf);
        char *head = buf;
        char *tail = buf;
        while (strchr(head, '/') != NULL)
        {
            while (*head != '/')
                head++;
            *head++ = 0;
            inode_get((char *)&fn[dir], dir);
            dir = lookup(tail, DIR_T, fn[dir].block_point);
            if (dir == -1)
            {
                printf("no dir named %s\n", tail);
                return -1;
            }
            tail = head;
        }
        dir = lookup(head, DIR_T, fn[dir].block_point);
        if (dir == -1)
        {
            printf("no dir named %s\n", tail);
            return -1;
        }
        blocks = fn[dir].block_point;
    }
    q = fn[dir].size / (DISK_BLOCK_SIZE / DNODE_SIZE);
    r = fn[dir].size % (DISK_BLOCK_SIZE / DNODE_SIZE);
    for (int i = 0; i <= q; i++)
    {
        read(disk_buf, blocks[i], DISK_BLOCK_SIZE);
        for (int j = 0; j < DISK_BLOCK_SIZE / DNODE_SIZE; j++)
        {
            memcpy((char *)&dn, disk_buf + j * DNODE_SIZE, DNODE_SIZE);
            if (i == q && j == r)
            {
                if (i != 0 || j != 0)
                {
                    printf("\n");
                }
                break;
            }
            printf("%s ", dn.name);
        }
    }
}

int mkdir()
{
    int fn_num, db_num, dir;
    char *ptr = buf;
    if (getchar() != '\n')
    {
        do
        {
            scanf("%s", buf);
            char *head = buf;
            char *tail = buf;
            dir = ROOT_NODE;
            while (strchr(head, '/') != NULL)
            {
                while (*head != '/')
                    head++;
                *head++ = 0;
                inode_get((char *)&fn[dir], dir);
                dir = lookup(tail, DIR_T, fn[dir].block_point);
                if (dir == -1)
                {
                    printf("no dir named %s\n", tail);
                    return -1;
                }
                tail = head;
            }
            read((char *)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
            db_num = DATA_BLOCK_CNT - sp.free_block_count;
            fn_num = FNODE_CNT - sp.free_inode_count;

            dn.inode_id = fn_num;
            dn.valid = 1;
            dn.type = DIR_T;
            memcpy(dn.name, head, strlen(head));

            inode_get((char *)&fn[dir], dir);
            fn[dir].size++;
            dir_set((char *)&dn, fn[dir].block_point);
            inode_set((char *)&fn[dir], dir);

            fn[fn_num].size = 0;
            fn[fn_num].file_type = DIR_T;
            fn[fn_num].link = 0;
            fn[fn_num].block_point[0] = db_num;
            inode_set((char *)&fn[fn_num], fn_num);

            sp.free_inode_count--;
            sp.free_block_count--;
            sp.dir_inode_count++;
            map_set(sp.block_map, db_num);
            map_set(sp.inode_map, fn_num);
            store((char *)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
            memset((char*)&dn, 0, DNODE_SIZE);
        } while (getchar() != '\n');
    }
    else
    {
        printf("Usage: mkdir <dir>\n");
    }
}

int touch()
{
    int fn_num, db_num, dir;
    char *ptr = buf;
    if (getchar() != '\n')
    {
        do
        {
            scanf("%s", buf);
            char *head = buf;
            char *tail = buf;
            dir = ROOT_NODE;
            while (strchr(head, '/') != NULL)
            {
                while (*head != '/')
                    head++;
                *head++ = 0;
                inode_get((char *)&fn[dir], dir);
                dir = lookup(tail, DIR_T, fn[dir].block_point);
                if (dir == -1)
                {
                    printf("no dir named %s\n", tail);
                    return -1;
                }
                tail = head;
            }
            read((char *)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
            db_num = DATA_BLOCK_CNT - sp.free_block_count;
            fn_num = FNODE_CNT - sp.free_inode_count;

            dn.inode_id = fn_num;
            dn.valid = 1;
            dn.type = FILE_T;
            memcpy(dn.name, head, strlen(head));

            inode_get((char *)&fn[dir], dir);
            fn[dir].size++;
            dir_set((char *)&dn, fn[dir].block_point);
            inode_set((char *)&fn[dir], dir);

            fn[fn_num].size = 0;
            fn[fn_num].file_type = FILE_T;
            fn[fn_num].link = 0;
            fn[fn_num].block_point[0] = db_num;
            inode_set((char *)&fn[fn_num], fn_num);

            sp.free_block_count--;
            sp.free_inode_count--;
            sp.dir_inode_count++;
            map_set(sp.inode_map, fn_num);
            map_set(sp.block_map, db_num);
            store((char *)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
        } while (getchar() != '\n');
    }
    else
    {
        printf("Usage: touch <file>\n");
    }
}

int cp()
{
    int src, dst, dir;
    int fn_num, db_num;
    char *ptr = buf;
    if (getchar() != '\n')
    {
        scanf("%s", buf);
        char *head = buf;
        char *tail = buf;
        dir = ROOT_NODE;
        while (strchr(head, '/') != NULL)
        {
            while (*head != '/')
                head++;
            *head++ = 0;
            inode_get((char *)&fn[dir], dir);
            dir = lookup(tail, DIR_T, fn[dir].block_point);
            if (dir == -1)
            {
                printf("no dir named %s\n", tail);
                return -1;
            }
            tail = head;
        }
        inode_get((char *)&fn[dir], dir);
        src = lookup(head, FILE_T, fn[dir].block_point);
        if (src == -1)
        {
            printf("no file named %s\n", head);
            return -1;
        }
        else
        {
            if (getchar() != '\n')
            {
                scanf("%s", buf);
                head = buf;
                tail = buf;
                dir = ROOT_NODE;
                while (strchr(head, '/') != NULL)
                {
                    while (*head != '/')
                        head++;
                    *head++ = 0;
                    inode_get((char *)&fn[dir], dir);
                    dir = lookup(tail, DIR_T, fn[dir].block_point);
                    if (dir == -1)
                    {
                        printf("no dir named %s\n", tail);
                        return -1;
                    }
                    tail = head;
                }
                read((char *)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
                fn_num = FNODE_CNT - sp.free_inode_count;

                memset((char*)&dn, 0, sizeof(dn));
                dn.inode_id = fn_num;
                dn.valid = 1;
                dn.type = FILE_T;
                memcpy(dn.name, head, strlen(head));

                inode_get((char *)&fn[dir], dir);
                fn[dir].size++;
                dir_set((char *)&dn, fn[dir].block_point);
                inode_set((char *)&fn[dir], dir);

                fn[fn_num].size = 0;
                fn[fn_num].file_type = FILE_T;
                fn[fn_num].link = 0;
                memcpy(fn[fn_num].block_point, fn[src].block_point, DIRECT_INDEX_SIZE);
                inode_set((char *)&fn[fn_num], fn_num);

                sp.free_inode_count--;
                sp.dir_inode_count++;
                map_set(sp.inode_map, fn_num);
                store((char *)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
            }
            else
            {
                printf("Usage: cp <src> <dst>\n");
            }
        }
    }
    else
    {
        printf("Usage: cp <src> <dst>\n");
    }
}

void format()
{
    for (int i = 0; i < DATA_BLOCK_CNT; i++)
    {
        memset(disk_buf, 0, DISK_BLOCK_SIZE);
        store(disk_buf, i, DISK_BLOCK_SIZE);
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

void shell()
{
    while (1)
    {
        memset(buf, 0, sizeof(buf));
        fflush(stdin);
        printf(">>>");
        if (scanf("%s", buf) != EOF)
        {
            if (strcmp(buf, ls_cmd) == 0)
            {
                ls();
            }
            else if (strcmp(buf, mkdir_cmd) == 0)
            {
                mkdir();
            }
            else if (strcmp(buf, touch_cmd) == 0)
            {
                touch();
            }
            else if (strcmp(buf, cp_cmd) == 0)
            {
                cp();
            }
            else if (strcmp(buf, shutdown_cmd) == 0)
            {
                break;
            }
            else
            {
                printf("unknown command %s\n", buf);
            }
        }
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