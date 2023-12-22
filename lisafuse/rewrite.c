//
// Created by Li on 2021/9/4.
//

//gcc -Wall lfs.c `pkg-config fuse --cflags --libs` -o  lfs
#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS  64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <stddef.h>
#include <fuse.h>
#include "hdfslowfs.h"
#include <sys/stat.h>
#include <io.h>
#define MAX_FILENAME 20
#define MAX_EXTENSION 10
int block_id_use = 0;
unsigned char *p_bitmap;
int fd;
uint64_t cur_offset;
uint64_t dev_size;
uint64_t block_size;
struct file_data{
    uint64_t block_id;
    uint64_t size;
    char name[MAX_FILENAME];
} *pData;
struct file_meta{
    uint64_t block_id;
    uint64_t size;
    char name[MAX_FILENAME];
} *pMeta;
struct file_fd{
    uint64_t block_id;
    uint64_t data_block_id;
    uint64_t size;
    uint64_t dev_offset;
    uint64_t fd;
    uint8_t flag;//0为meta 1为data
} *pFd;

/*getattr函数用来填充每个文件的mode和size
 * getattr 修改文件mode
 * 修改文件大小
 */
static int lfs_getattrr(const char *path, struct stat *stbuf){
    //解析path获取block_id
    char out_meta[20]={0};
    char out_block_id[20]={0};
    sscanf(path,"%*[^_]_%[^_]_", out_block_id);
    int b_block_id = atoi(out_block_id);
    sscanf(path,"%*[^.].%s", out_meta);
    //先初始化mode
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
    } else if(strcmp(out_meta,"meta")==0){
        stbuf->st_mode = S_IFREG | 0644;
        //stbuf->st_size =  file[bm_data_block_id].csize;
        stbuf->st_size = 0;
    }else{
        stbuf->st_mode = S_IFREG | 0644;
        //stbuf->st_size =file[bm_data_block_id].dsize;
        stbuf->st_size = 0;
    }
    //读取block_info的数据
    int bi_len = INFO_SIZE(dev_size,block_size);
    //printf("bi_len:%d\n",bi_len);
    struct block_info *bi=malloc(bi_len);
    uint64_t dev_offset;
    dev_offset = INFO_OFFSET(dev_size,block_size);
    lseek(fd,dev_offset,SEEK_SET);
    read(fd, bi, bi_len);
    //读取bitmap
    unsigned char *p_bitmap=read_bitmap(fd,dev_size,block_size);
    int bit_size=BLOCK_NUM(dev_size,block_size);
    int bm_list_len=0;
    int *bm_list = bitmap_usedblockid(p_bitmap,bit_size,0,bit_size,&bm_list_len);
    int bm_data_block_id;

    for (int i = 0; i < bm_list_len; i++) {
        bm_data_block_id = bm_list[i];
        if (bi[bm_data_block_id].b_block_id == b_block_id) {
            if (strcmp(out_meta,"meta") == 0) {
                //记录size
                pMeta[bm_data_block_id].size = bi[bm_data_block_id].b_csm_length;
                //printf("file-meta %d csize:%ld\n", bm_data_block_id, pMeta[bm_data_block_id].size);
                pMeta[bm_data_block_id].block_id = b_block_id;
                stbuf->st_size = pMeta[bm_data_block_id].size;
            } else {
                pData[bm_data_block_id].block_id = b_block_id;
                pData[bm_data_block_id].size = bi[bm_data_block_id].b_data_length;
                //printf("file-data %d size:%ld\n", bm_data_block_id, pData[bm_data_block_id].size);
                stbuf->st_size = pData[bm_data_block_id].size;
            }
            break;
        }
    }
    return 0;
}

//遍历目录将文件name添加到buf中
/*
 * readdir
 * 遍历整个file_directory的name
 * 在重新挂载的时候，open磁盘获取到name填充file_directory结构体
 * */
static int lfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi){
    (void) fi;
    (void) offset;
    char out_block_id[20]={0};
    sscanf(path,"%*[^_]_%[^_]_", out_block_id);
    int b_block_id = atoi(out_block_id);
    char out_meta[20]={0};
    sscanf(path,"%*[^.].%s", out_meta);
    if (strcmp(path, "/") != 0)
        return -ENOENT;
    filler(buf, ".", NULL, 0);
    //在/目录下增加. 这个目录项
    filler(buf, "..", NULL, 0);
    //读取block_info的数据
    int bi_len = INFO_SIZE(dev_size,block_size);
    struct block_info *bi=malloc(bi_len);
    uint64_t dev_offset;
    dev_offset = INFO_OFFSET(dev_size,block_size);
    lseek(fd,dev_offset,SEEK_SET);
    read(fd, bi, bi_len);
    //读取bitmap
    unsigned char *p_bitmap=read_bitmap(fd,dev_size,block_size);
    int bit_size=BLOCK_NUM(dev_size,block_size);
    int bm_list_len=0;
    int *bm_list = bitmap_usedblockid(p_bitmap,bit_size,0,bit_size,&bm_list_len);
    //循环写入file_directory
    //struct file_directory *file = malloc(sizeof (struct file_directory));
    int bm_data_block_id;

    for(int i=0;i<bm_list_len;i++){
        bm_data_block_id = bm_list[i];
        if(strcmp(out_meta,"meta") == 0){
            sprintf(pMeta[bm_data_block_id].name, "blk_%d_1001.meta", bi[bm_data_block_id].b_block_id);
            //printf("file name:%s\n",file[bm_data_block_id].csm_name);
            filler(buf,pMeta[bm_data_block_id].name, NULL, 0);
        }else {
            sprintf(pData[bm_data_block_id].name, "blk_%d", bi[bm_data_block_id].b_block_id);
            //printf("file name:%s\n",file[bm_data_block_id].data_name);
            filler(buf, pData[bm_data_block_id].name, NULL, 0);
        }
    }
    return 0;
}


/*open 的时候维护一个文件句柄的列表*/
static int lfs_open(const char* path, struct fuse_file_info *fi) {
    //解析path
    char out_meta[20] = {0};
    sscanf(path, "%*[^.].%s", out_meta); //匹配出目录中的meta
    char out_block_id[20] = {0};
    sscanf(path, "%*[^_]_%[^_]_", out_block_id);//匹配出目录中的 block_id
    int b_block_id = atoi(out_block_id);
    int data_block_id = -1;
    //根据文件类型来决定修改meta或者data结构体
    if(strcmp(out_meta,"meta") == 0){
       // pFd[data_block_id].flag = 0;
        for(int i=0;i <= BLOCK_NUM(dev_size,block_size); i++){
            if( pMeta[i].block_id == b_block_id){
                //先获取到初始的便宜后面可以直接用来写入数据
                fi->fh = i+3;
                pFd[i+3].fd = fi->fh;
                pFd[i+3].data_block_id = data_block_id;
                data_block_id = i;
                break;
            }
        }
        if(data_block_id < 0) {
            //printf("fils not exist\n");
            int bit_size = BLOCK_NUM(dev_size, block_size);
            unsigned char *p_bitmap = read_bitmap(fd, dev_size, block_size);
            bitmap_availableblockid(1, bit_size, &data_block_id, p_bitmap);
            fi->fh = data_block_id+3;
            pFd[data_block_id+3].fd = fi->fh;
            pFd[data_block_id+3].dev_offset = DATA_OFFSET(dev_size, block_size) + data_block_id * block_size;
            pFd[data_block_id+3].data_block_id = data_block_id;
            pMeta[data_block_id].block_id = b_block_id;
            pFd[data_block_id+3].block_id = b_block_id;
            bitmap_set(data_block_id, p_bitmap, bit_size);
            write_bitmap(fd, dev_size, block_size, BITMAP_SIZE(dev_size, block_size), p_bitmap);
            pFd[data_block_id +3].flag = 0;
        }

    }else{
        //pFd[data_block_id].flag = 1;
        //printf("b_block_id:%d\n",b_block_id);
        if(block_id_use != 0){
            for(int i=0;i <= BLOCK_NUM(dev_size,block_size); i++){
                if( pData[i].block_id == b_block_id){
                    //先获取到初始的便宜后面可以直接用来写入数据
                    fi->fh = i+BLOCK_NUM(dev_size,block_size)+2;
                    //printf("fd_id:%ld\n",fi->fh);
                    pFd[i+BLOCK_NUM(dev_size,block_size)+2].fd = fi->fh;
                    data_block_id = i;
                    pFd[i+BLOCK_NUM(dev_size,block_size)+2].data_block_id = data_block_id;
                    //printf("pData-%d-block_id:%ld\n",i,pData[i].block_id);
                    break; 
                }

            }
        }

        if(data_block_id < 0) {
            //printf("fils not exist\n");
            int bit_size = BLOCK_NUM(dev_size, block_size);
            //unsigned char *p_bitmap = read_bitmap(fd, dev_size, block_size);
            bitmap_availableblockid(1, bit_size, &data_block_id, p_bitmap);
            block_id_use++;
            //data_block_id = 0;
            fi->fh = data_block_id+BLOCK_NUM(dev_size,block_size)+2;
            //printf("fd_id:%ld\n",fi->fh);
            pFd[data_block_id+BLOCK_NUM(dev_size,block_size)+2].fd = fi->fh;
            pFd[data_block_id+BLOCK_NUM(dev_size,block_size)+2].dev_offset = DATA_OFFSET(dev_size, block_size) + data_block_id * block_size + 2*MB_SIZE;
            pFd[data_block_id+BLOCK_NUM(dev_size,block_size)+2].data_block_id = data_block_id;
            pFd[data_block_id+BLOCK_NUM(dev_size,block_size)+2].block_id = b_block_id;
            pFd[data_block_id+BLOCK_NUM(dev_size,block_size)+2].flag = 1;
            pData[data_block_id].block_id = b_block_id;
            //printf("pData-%d-block_id:%ld\n",data_block_id,pData[data_block_id].block_id);
            bitmap_set(data_block_id, p_bitmap, bit_size);
            write_bitmap(fd, dev_size, block_size, BITMAP_SIZE(dev_size, block_size), p_bitmap);
        }
    }

    return 0;

}
static int lfs_release(const char *path, struct fuse_file_info *fi)
{
    uint64_t bi_offset;
    uint64_t fd_id ;
    uint64_t data_block_id;
    fd_id = fi->fh;
    struct block_info bi;
    bi.b_block_id = pFd[fd_id].block_id;
    data_block_id = pFd[fd_id].data_block_id;
    if(pFd[fd_id].flag == 0){
        bi.b_csm_length = pFd[fd_id].size;
    }else{
        bi.b_data_length = pFd[fd_id].size;
    }
    bi_offset = INFO_OFFSET(dev_size, block_size) +  data_block_id * sizeof(struct block_info);
    lseek(fd, bi_offset, SEEK_SET);
    write(fd, &bi, sizeof(struct block_info));
    cur_offset = bi_offset + sizeof(struct block_info);
    return 0;
}

static int lfs_write(const char *path, const char *buf, size_t size,
                      off_t offset, struct fuse_file_info *fi){
    uint64_t fd_id ;
    uint64_t data_offset;
    data_offset = offset;
    uint64_t dev_offset;
    fd_id = fi->fh;
    pFd[fd_id].size = pFd[fd_id].size + size;
    dev_offset = pFd[fd_id].dev_offset + data_offset;
    if(cur_offset != dev_offset){
        lseek(fd,dev_offset, SEEK_SET);
        //printf("cur_offset:%ld\n",lseek(fd,0,SEEK_CUR));
        //printf("dev_offset:%ld\n",dev_offset);
    }
    write(fd,buf,size);
    cur_offset = dev_offset +size;
    return size;
}


static int lfs_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi){
    uint64_t data_offset;
    data_offset = offset;
    uint64_t fd_id;
    fd_id = fi->fh;
   // printf("fd_id:%ld\n",fd_id);
    uint64_t dev_offset;
    dev_offset = pFd[fd_id].dev_offset + data_offset;
    if(cur_offset != dev_offset){
        lseek(fd, dev_offset, SEEK_SET);
        //printf("cur_offset:%ld\n",lseek(fd,0,SEEK_CUR));
        //printf("dev_offset:%ld\n",dev_offset);
    }
    read(fd,buf,size);
    cur_offset = dev_offset + size;
    return size;
}


static int lfs_flush(const char*path, struct fuse_file_info *fi) {
    (void)path;
    (void)fi;
    return 0;
}
static int lfs_mknod(const char *path, mode_t mode, dev_t rdev) {
    return 0;
}
/** Get extended attributes */
static int lfs_getxattr(const char *path, const char *name, char *value, size_t size) {
    (void)path;
    (void)name;
    (void)value;
    (void)size;
    //printf("not need getxattr\n");
    return 0;

}
static int lfs_truncate(const char *path, off_t size){
    (void)path;
    (void)size;
    return 0;
}



//*****************************************************************//
static struct fuse_operations u_operation = {
        .getattr = lfs_getattrr,
        .readdir = lfs_readdir,
        //.create = lfs_create,
        .mknod = lfs_mknod,
        .write = lfs_write,
        .flush = lfs_flush,
        .open = lfs_open,
        .release = lfs_release,
        .read = lfs_read,
        .getxattr = lfs_getxattr,
        .truncate = lfs_truncate,
};
//./lfs ./test -d /dev/sdc4
int main(int argc, char *argv[]) {
    umask(0);
    int res = 0;
    char *dev_name = argv[argc-1];
    fd = open(dev_name,O_RDWR | O_EXCL);
    if(fd <0 ){
        printf("Open fd Error\n");
        return 0;
    }

    //读superblock
    struct super_block sb;
    //lseek(fd,0,SEEK_SET);
    if(read(fd,&sb,sizeof(struct super_block)) == -1){
        printf("read fd Error\n");
        return 0;
    }

    if(sb.s_magic != MK_SUPER_MAGIC)
    {
        printf("Read Dev Error.\n");
        return 0;
    }
    dev_size = sb.s_dev_size;
    block_size =sb.s_block_size;
    printf("dev_size:%ld\n",dev_size);
    printf("sb.s_dev_size:%I64lu\n",sb.s_dev_size);
    printf("block_size:%ld\n",block_size);
    printf("BLOCK_NUM(dev_size,block_size) :%ld\n",BLOCK_NUM(dev_size,block_size));
    pMeta = malloc(sizeof(struct file_meta)*BLOCK_NUM(dev_size,block_size));
    pData = malloc(sizeof(struct file_data)*BLOCK_NUM(dev_size,block_size));
    pFd = malloc(sizeof(struct file_fd)*(2*BLOCK_NUM(dev_size,block_size))+3);
    //printf("file 是不是越界了啊：%ld\n",sizeof(struct file_directory)*BLOCK_NUM(dev_size,block_size));
    //argc--;
    p_bitmap =  read_bitmap(fd, dev_size, block_size);
    res =fuse_main(argc-1, argv, &u_operation, NULL);
    close(fd);
    printf("Close fd\n");
    return 0;
}
