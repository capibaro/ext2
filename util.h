#ifndef UTIL_H
#define UTIL_H

void map_set(uint32_t *map, uint32_t num);

void map_reset(uint32_t *map, uint32_t num);

int map_test(uint32_t *map, uint32_t num);

void store(char *buffer, uint32_t num, uint32_t len);

void read(char *buffer, uint32_t num, uint32_t len);

void inode_get(char *buffer, uint32_t num);

void inode_set(char *buffer, uint32_t num);

int lookup(char *buffer, int type, uint32_t *blocks);

int dir_set(char *buffer, uint32_t *blocks);

#endif