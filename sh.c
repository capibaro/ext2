#include <stdio.h>
#include <string.h>
#include "ext2.h"
#include "disk.h"
#include "util.h"
#include "sh.h"

extern sp_block sp;
extern file_node fn[FNODE_CNT];
extern dir_node dn;

extern char disk_buf[DEVICE_BLOCK_SIZE];
extern char buf[FNAME_SIZE];

char *ls_cmd = "ls";
char *mkdir_cmd = "mkdir";
char *touch_cmd = "touch";
char *cp_cmd = "cp";
char *shutdown_cmd = "shutdown";

int ls()
{
    int dir, q, r;
    uint32_t *blocks;
    dir = ROOT_NODE;
    // inode_get((char *)&fn[dir], dir);
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
    }
    inode_get((char *)&fn[dir], dir);
    blocks = fn[dir].block_point;
    q = fn[dir].size / (DEVICE_BLOCK_SIZE / DNODE_SIZE);
    r = fn[dir].size % (DEVICE_BLOCK_SIZE / DNODE_SIZE);
    for (int i = 0; i <= q; i++)
    {
        read(disk_buf, blocks[i], DEVICE_BLOCK_SIZE);
        for (int j = 0; j < DEVICE_BLOCK_SIZE / DNODE_SIZE; j++)
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
            sp.free_block_count--;
            sp.free_inode_count--;
            store((char *)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);

            dn.inode_id = fn_num;
            dn.valid = 1;
            dn.type = DIR_T;
            memset(dn.name, 0, FNAME_SIZE);
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

            read((char *)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
            sp.dir_inode_count++;
            map_set(sp.inode_map, fn_num);
            map_set(sp.block_map, db_num);
            store((char *)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
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
            memset(dn.name, 0, FNAME_SIZE);
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

                memset((char *)&dn, 0, sizeof(dn));
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