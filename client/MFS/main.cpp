#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stddef.h>
//#include <fuse3/fuse.h>
#include "common.h"
#include <sys/stat.h>

/*int64_t dev_size=429496729600;
int block_size=BLOCK_SIZE;
FILE *fd= fopen("/dev/1","r+");
static int lfs_getattrr(const char *path, struct stat *stbuf);
static int lfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi);
static int lfs_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi);
static int lfs_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi);

static int lfs_getattrr(const char *path, struct stat *stbuf){
    char id[20];
    int block_id;
    sscanf(path,"%*[^_]_%[^_]_", id);
    block_id=atoi(id);
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
    } else if(strcmp(path,"meta")==0){
        stbuf->st_mode = S_IFREG | 0644;
        //stbuf->st_size =  file[bm_data_block_id].csize;
        stbuf->st_size = 0;
    }else{
        stbuf->st_mode = S_IFREG | 0644;
        //stbuf->st_size =file[bm_data_block_id].dsize;
        stbuf->st_size = 0;
    }
    int bi_len = INFO_SIZE(dev_size,block_size);
    struct block_info *bi=(struct block_info *)malloc(bi_len);
    uint64_t dev_offset;
    dev_offset = INFO_OFFSET(dev_size,block_size);
    fseek(fd,dev_offset,SEEK_SET);
    fread(bi, bi_len, 1,fd);
    unsigned char *p_bitmap=read_bitmap(fd,dev_size,block_size);
    int bit_size=BLOCK_NUM(dev_size,block_size);
    int bm_list_len=0;
    int *bm_list = bitmap_usedblockid(p_bitmap,bit_size,0,bit_size,&bm_list_len);
    int bm_data_block_id;
    printf("file-data %d size:%ld\n", block_id,  bi[block_id].b_data_length);
    stbuf->st_size = bi[block_id].b_data_length;
    return 0;
}
int lfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    char out_block_id[20]={0};
    sscanf(path,"%*[^_]_%[^_]_", out_block_id);
    int b_block_id = atoi(out_block_id);
    if (strcmp(path, "/") != 0)
        return -ENOENT;
    filler(buf, ".", NULL, 0);
    //在/目录下增加. 这个目录项
    filler(buf, "..", NULL, 0);
    //读取block_info的数据
    int bi_len = INFO_SIZE(dev_size,block_size);
    struct block_info *bi=(struct block_info *)malloc(bi_len);
    uint64_t dev_offset;
    dev_offset = INFO_OFFSET(dev_size,block_size);
    fseek(fd,dev_offset,SEEK_SET);
    fread(bi, bi_len, 1,fd);
    //读取bitmap
    unsigned char *p_bitmap=read_bitmap(fd,dev_size,block_size);
    int bit_size=BLOCK_NUM(dev_size,block_size);
    int bm_list_len=0;
    int *bm_list = bitmap_usedblockid(p_bitmap,bit_size,0,bit_size,&bm_list_len);
    //循环写入file_directory
    //struct file_directory *file = malloc(sizeof (struct file_directory));
    int bm_data_block_id;

    for(int i=0;i<bit_size;i++){
       if(bi[i].b_block_id!=-1){
           char string[20];
           sprintf(string, "%d", bi[i].b_block_id);
           filler(buf,string , NULL, 0);
    }}
    return 0;
}

int lfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    return 0;
}

int lfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    return 0;
}
int main() {
    struct block_info *pData = malloc(sizeof(struct file_data)*BLOCK_NUM(dev_size,block_size));
    return 0;
}*/