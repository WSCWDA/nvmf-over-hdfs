//
// Created by chen on 23-3-27.
//

#ifndef NEWFS_COMMON_H
#define NEWFS_COMMON_H
//
// Created by Li on 2021/7/12.
//

/*
|--sb--|---bm----|---bi---|-path-|-csm0-+-data0-|......

|-512B-|-512*200B-|--13M--|------|--2M--|-126M-|......

|----------128M------------------|
*/

#ifndef HDFSLOWFS_H
#define HDFSLOWFS_H


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>              /* low-level i/o */
#include <sys/stat.h>
#include <stdbool.h>
#include <cmath>

#define MK_SUPER_MAGIC 0xabcdefa

#define MB_SIZE 1024*1024

#define ceil(n) ((int)n+1)//向上取整

//super_block的信息
struct super_block {
    uint32_t s_magic; //4字节0xabcdefa
    uint32_t s_block_size; //每个块大小
    uint32_t s_block_counts; //块的总数量
    uint32_t s_data_block_first; //第一个数据块的偏移量
    uint64_t s_dev_size; //磁盘的大小
    uint64_t s_offset; //信息块的偏移量
    uint32_t s_mkfs_time; //文件系统被创建的时间
    char reserve[512 - 32];
};
//数据块的信息
struct block_info {
    //uint32_t b_creat_time; //数据块被创建的时间
    //uint32_t b_update_time; //数据块更新的时间
    //uint64_t b_csm_length;//写入文件的大小
    uint64_t b_data_length;
    uint32_t b_block_id;//b_block_id
    int32_t data_block_num=0;
    short data_block[3200];
};

#define BLOCK_SIZE (long)(128*1024*1024)
#define BLOCK_INFO_SIZE sizeof(struct block_info) // bm+bi 大小

#define BLOCK_NUM(devsize,blocksize) (devsize/BLOCK_SIZE-1) //数据块的个数

#define BITMAP_SIZE(devsize,blocksize) (int)(ceil(BLOCK_NUM(devsize,blocksize)/8.0))//bitmap 大小
#define BITMAP_SPACE(devsize,blocksize) (int)(ceil(BITMAP_SIZE(devsize,blocksize)/512.0))//bitmap 占用的512块数
#define BITMAP_OFFSET sizeof(struct super_block)

#define INFO_SIZE(devsize,blocksize)  BLOCK_NUM(devsize,blocksize)*sizeof(struct block_info)//block info 大小
#define INFO_SPACE(devsize,blocksize) (int)(ceil(INFO_SIZE(devsize,blocksize)/512.0))//block info 占用的512块数
#define INFO_OFFSET(devsize,blocksize) BITMAP_OFFSET+BITMAP_SIZE(devsize,blocksize)//block info 起始偏移量

#define EXTRA_SIZE(devsize,blocksize) BLOCK_NUM(devsize,blocksize)*sizeof(struct block_extra_info)//block extra info 大小
#define EXTRA_SPACE(devsize,blocksize) (int)(ceil(EXTRA_SIZE(devsize,blocksize)/512.0))//block extra info 占用的512块数
#define EXTRA_OFFSET(devsize,blocksize) (INFO_OFFSET(devsize,blocksize)+INFO_SPACE(devsize,blocksize)*512)//block extra inf 起始偏移量

#define DATA_OFFSET(devsize,blocksize) (INFO_OFFSET(devsize,blocksize)+INFO_SIZE(devsize,blocksize))//数据块的起始偏移量

int bitmap_availableblockid(int n, int size, int* availablelist, unsigned char* p_bitmap);
int* bitmap_usedblockid(unsigned char* p_bitmap, int size, int start, int end, int* list_len);
unsigned char bitmap_unset(int index, unsigned char* p_bitmap, int size);
unsigned char bitmap_set(int index, unsigned char* p_bitmap, int size);
int bitmap_get(int index, unsigned char* p_bitmap, int size);
int write_bitmap(FILE* fd, uint64_t dev_size, uint64_t block_size, int len,unsigned char* buf);
unsigned char* read_bitmap(FILE* fd, uint64_t dev_size, uint64_t block_size);
void print_bitmap(unsigned char* p_bitmap, int size);

#endif //HDFSLOWFS_H

#endif //NEWFS_COMMON_H
