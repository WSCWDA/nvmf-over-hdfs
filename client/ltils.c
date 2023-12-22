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
int fd;
uint64_t dev_size;
uint64_t block_size;
struct file_directory{
    char csm_name[MAX_FILENAME]; //filename(plus space for nul)
    char data_name[MAX_FILENAME];
    uint32_t block_id;
    uint64_t c_offset;
    uint64_t real_offset;
    uint32_t csize;
    uint32_t dsize;
    uint32_t file_fd;
    uint16_t flag;
    uint32_t cur_offset;
}* file;


/*getattr函数用来填充每个文件的mode和size
 * getattr 修改文件mode
 * 修改文件大小
 * */

static int lfs_getattrr(const char *path, struct stat *stbuf){
    char out_meta[20]={0};
    char out_block_id[20]={0};
    sscanf(path,"%*[^_]_%[^_]_", out_block_id);
    int b_block_id=atoi(out_block_id);
    sscanf(path,"%*[^.].%s", out_meta);
    /*  printf("block_id:%d\n",b_block_id);
  */
    memset(stbuf,0, sizeof(struct stat));
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
    //获取块id：b_block_id

    //读取block_info的数据
    int bi_len = INFO_SIZE(dev_size,block_size);
    printf("bi_len:%d\n",bi_len);
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
/*    printf("bm_list_len %d \n",bm_list_len);
    printf("循环bm列表\n");*/
    //file[bm_data_block_id].dsize = 2;

    for (int i = 0; i < bm_list_len; i++) {
        bm_data_block_id = bm_list[i];
        if (bi[bm_data_block_id].b_block_id == b_block_id) {
            //stbuf->st_mode = S_IFREG | 0644;
            //记录offset和id
            printf("file %d data_name:%s\n", bm_data_block_id, file[bm_data_block_id].data_name);
            file[bm_data_block_id].block_id = bi[bm_data_block_id].b_block_id;
            file[bm_data_block_id].c_offset = DATA_OFFSET(dev_size, block_size) + bm_data_block_id * block_size;
            printf("file-directory %d c_offset:%ld\n", bm_data_block_id, file[bm_data_block_id].c_offset);
            if (strcmp(out_meta,"meta") == 0) {
                //记录size
                file[bm_data_block_id].csize = bi[bm_data_block_id].b_csm_length;
                printf("file-directory %d csize:%d\n", bm_data_block_id, file[bm_data_block_id].csize);
                stbuf->st_size = file[bm_data_block_id].csize;
            } else {
                file[bm_data_block_id].dsize = bi[bm_data_block_id].b_data_length;
                printf("file-directory %d dsize:%d\n", bm_data_block_id, file[bm_data_block_id].dsize);
                stbuf->st_size = file[bm_data_block_id].dsize;
            }
            break;
        }
    }
/*
    printf("exit getter\n");
    printf("stbuf->st_size %ld\n",stbuf->st_size);
*/
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
            sprintf(file[bm_data_block_id].csm_name, "blk_%d_1001.meta", bi[bm_data_block_id].b_block_id);
            //printf("file name:%s\n",file[bm_data_block_id].csm_name);
            filler(buf,file[bm_data_block_id].csm_name, NULL, 0);
        }else {
            sprintf(file[bm_data_block_id].data_name, "blk_%d", bi[bm_data_block_id].b_block_id);
            //printf("file name:%s\n",file[bm_data_block_id].data_name);
            filler(buf, file[bm_data_block_id].data_name, NULL, 0);
        }
    }
    return 0;
}


/*open 的时候维护一个文件句柄的列表*/
static int lfs_open(const char* path, struct fuse_file_info *fi) {
    /* 1、先拿到文件名、循环遍历文件是否存在，因为每个文件在file结构体中的位置是固定的，所以就使用file结构中的编号作为文件句柄 */
    //文件名称解析到
    char out_meta[20] = {0};
    sscanf(path, "%*[^.].%s", out_meta); //匹配出目录中的meta
    char out_block_id[20] = {0};
    sscanf(path, "%*[^_]_%[^_]_", out_block_id);//匹配出目录中的 block_id
    //判断path是否存在，不存在创建
    int data_block_id = -1;
    for(int i=0;i <= BLOCK_NUM(dev_size,block_size); i++){
        if(file[i].block_id == atoi(out_block_id)){
            //先获取到初始的便宜后面可以直接用来写入数据
            fi->fh = i+3;
            file[i].file_fd = fi->fh;
            data_block_id = i;
            break;
        }
    }
    //如果不在磁盘中 要重新找可用块，并将fd、offset等信息加入file
    int bit_size = BLOCK_NUM(dev_size, block_size);
    unsigned char *p_bitmap = read_bitmap(fd, dev_size, block_size);
    if(data_block_id < 0) {
        bitmap_availableblockid(1, bit_size, &data_block_id, p_bitmap);
        file[data_block_id].c_offset = DATA_OFFSET(dev_size, block_size) + data_block_id * block_size;
        file[data_block_id].block_id = atoi(out_block_id);
        fi->fh = data_block_id+3;
        file[data_block_id].file_fd = fi->fh;
        //更改bitmap
        bitmap_set(data_block_id, p_bitmap, bit_size);
        write_bitmap(fd, dev_size, block_size, BITMAP_SIZE(dev_size, block_size), p_bitmap);
    }
    if(strcmp(out_meta,"meta") == 0){
        file[data_block_id].flag = 1; //1为meta数据

    }else {
        file[data_block_id].flag = 0; //0为blk数据
        file[data_block_id].real_offset = file[data_block_id].c_offset + 2*MB_SIZE;
    }
    return 0;

}
/*static int lfs_release(const char *path, struct fuse_file_info *fi)
{
    uint64_t data_block_id;
    uint64_t bi_offset;
    struct block_info bi;
    struct file_directory *fp;
    data_block_id = fi->fh - 3;
    fp = &file[data_block_id];
    bi.b_block_id = fp->block_id;
    bi.b_data_length = fp->dsize;
    bi.b_csm_length = fp->csize;
    bi_offset = INFO_OFFSET(dev_size, block_size) + data_block_id * sizeof(struct block_info);
    lseek(fd, bi_offset, SEEK_SET);
    write(fd, &bi, sizeof(struct block_info));
    fp->cur_offset = bi_offset + sizeof(struct block_info);
    return 0;
}*/

/* 将文件数据写入磁盘中
 * fuse将文件分成 4096 bytes 写入
 * 1、根据file中存储的fd获取文件id以及文件类型flag
 * 2、根据flag（1为meta，0为blk），计算数据块在磁盘中的偏移量
 * 3、将数据写入磁盘中
 * 4、并更新block_info的信息*/
static int lfs_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi){
    //printf("path:%s\n",path);
    uint64_t dev_offset;
    uint64_t data_block_id ;
    uint64_t data_begin_offset;
    struct file_directory *fp;

    data_block_id = fi->fh - 3;
    fp = &file[data_block_id];
    data_begin_offset = fp->c_offset;
    dev_offset = fp->real_offset+offset;
    //设置磁盘的偏移量,并修改file内容
    /*if(fp->flag == 1 ){
        if(fp->csize < 2 * MB_SIZE){
            dev_offset = data_begin_offset + offset;
        }else{
            printf("csm length too long\n");
            return 0;
        }
        //sprintf(fp->csm_name, "blk_%d_1001.meta", fp->block_id);
    }else if(fp->flag == 0 ){
        if(fp->dsize < 128 * MB_SIZE){
            dev_offset = data_begin_offset + 2 * MB_SIZE + offset;
        }else{
            printf("data length too long\n");
            return 0;
        }
        //sprintf(fp->data_name, "blk_%d", fp->block_id);
    }*/
    //printf("写入磁盘的位置：%ld\n",dev_offset);
    if(fp->cur_offset != dev_offset){
        lseek(fd, dev_offset, SEEK_SET);
        //printf("cur_offset:%d\n",fp->cur_offset);
        //printf("dev_offset:%ld\n",dev_offset);
    }

    //把数据写入对应位置
    size_t data_size;
    data_size = size;
    //printf("size : %ld\n",data_size);
    write(fd,buf,data_size);
    fp->cur_offset = dev_offset + data_size;
    //printf("file[data_block_id].data_name:%s\n",file[data_block_id].data_name);
    //更新block_info的信息
    struct block_info bii;
    bii.b_block_id = file[data_block_id].block_id;
    //printf("file[data_block_id].flag:%d \n",file[data_block_id].flag);
    if(file[data_block_id].flag == 1){
        bii.b_csm_length = file[data_block_id].csize + data_size;
        //bii.b_data_length = file[data_block_id].dsize;
        file[data_block_id].csize = bii.b_csm_length;
        //printf("b_block_id=%d\nb_csm_length=%ld\n",bii.b_block_id,bii.b_csm_length);
    }else if(file[data_block_id].flag == 0){
        bii.b_data_length = file[data_block_id].dsize + data_size;
        //bii.b_csm_length = file[data_block_id].csize;
        file[data_block_id].dsize = bii.b_data_length;
        //printf("file data size:%d\n",file[data_block_id].dsize);
        //printf("b_block_id=%d\n b_data_length=%ld\n",bii.b_block_id,bii.b_data_length);
    }
    offset = INFO_OFFSET(dev_size, block_size) + data_block_id * sizeof(struct block_info);
    lseek(fd, offset, SEEK_SET);
    write(fd, &bii, sizeof(struct block_info));
    return size;
}

static int lfs_write2(const char *path, const char *buf, size_t size,
                      off_t offset, struct fuse_file_info *fi){
    //printf("path:%s\n",path);
    uint64_t dev_offset;
    uint64_t data_block_id ;
    //uint64_t data_begin_offset;
    struct file_directory *fp;
    size_t data_size;
    int cur_fd;
    data_size = size;
    data_block_id = fi->fh - 3;
    fp = &file[data_block_id];
    //data_begin_offset = fp->c_offset;
    fp->real_offset + offset;
    dev_offset = fp->real_offset + offset;
    //设置磁盘的偏移量,并修改file内容
    fp->csize = fp->csize  + data_size;
    fp->dsize = fp->dsize + data_size;
/*    if(fp->flag == 1 ){
        if(fp->csize <2 * MB_SIZE){
            dev_offset = data_begin_offset + offset;
            fp->csize = fp->csize + data_size;
        }else{
            printf("csm length too long\n");
            return 0;
        }
        //sprintf(fp->csm_name, "blk_%d_1001.meta", fp->block_id);
    }else if(fp->flag == 0 ){
        if(fp->dsize < 128 * MB_SIZE){
            dev_offset = data_begin_offset + 2 * MB_SIZE + offset;
            // printf("data_begin_offset:%d\n",data_begin_offset);
            fp->dsize = fp->dsize + data_size;

        }else{
            printf("data length too long\n");
            return 0;
        }
        //sprintf(fp->data_name, "blk_%d", fp->block_id);
    }*/
    //printf("写入磁盘的位置：%ld\n",dev_offset);

    if(fp->cur_offset != dev_offset){
        lseek(fd, dev_offset, SEEK_SET);
        printf("cur_offset:%ld\n",lseek(fd,0,SEEK_CUR));
        printf("dev_offset:%ld\n",dev_offset);
    }

    //printf("size : %ld\n",data_size);
    write(fd,buf,data_size);
    fp->cur_offset = dev_offset + data_size;
    //printf("file[data_block_id].data_name:%s\n",file[data_block_id].data_name);
    return size;
}
static int lfs_mknod(const char *path, mode_t mode, dev_t rdev) {
    return 0;
}
/*
 * 读取的path要么存在 要不不存在
 * 不存在的时候直接返回输入错误
 * 存在的时候,获取file中存储的c_offset
 * 先分析是meta还是data，来设置dev_offset
 * */
static int lfs_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi){
    /* printf("offset:%ld\n",offset);
     printf("size:%ld\n",size);
     printf("\n");
     printf("fi-fh 文件句柄：%ld\n",fi->fh);*/

    uint64_t data_begin_offset = -1;
    //atoi(out_block_id);
    uint64_t data_block_id;


    data_block_id = fi->fh - 3;
    data_begin_offset = file[data_block_id].c_offset;

    if(data_begin_offset <0)
    {
        //printf("文件不存在\n");
        return 0;
    }
    size_t data_size;
    uint64_t dev_offset;

    if(file[data_block_id].csize > 0)
    {
        if(offset > file[data_block_id].csize && size > (file[data_block_id].csize - offset)){
            //printf("Illegal input!\n");
            return 0;
        }else{
            data_size = size;
            // printf("data_size : %ld\n",data_size);
            dev_offset = data_begin_offset + offset;
            lseek(fd,dev_offset,SEEK_SET);
        }
    }else if(file[data_block_id].dsize > 0)
    {
        if(offset > file[data_block_id].dsize && size >(file[data_block_id].dsize - offset)){
            printf("Illegal input!\n");
            return 0;
        }else{
            //test
            /*printf("offset:%ld\n",offset);
            printf("dsize:%d\n",file[data_block_id].dsize);
            printf("size:%ld\n",size);
            printf("dsize- offset:%ld\n",file[data_block_id].dsize - offset);
*/
            data_size = size;
            /* printf("data_size : %ld\n",data_size);
             printf("dsize:%d\n",file[data_block_id].dsize);*/
            dev_offset = data_begin_offset + 2*MB_SIZE + offset;
            lseek(fd,dev_offset,SEEK_SET);
        }
    }

    /*  printf("读取的位置磁盘中：%ld\n",dev_offset);*/
    read(fd,buf,data_size);
    return size;
}
/*
 * 读取bi获取磁盘信息，bm拿到列表遍历bi找到改block的信息
 * 根据size大小移动指针位置读取信息
 * */

static int lfs_flush(const char*path, struct fuse_file_info *fi) {
    (void)path;
    (void)fi;
    return 0;
}

/** Get extended attributes */
static int lfs_getxattr(const char *path, const char *name, char *value, size_t size) {
    (void)path;
    (void)name;
    (void)value;
    (void)size;
    printf("not need getxattr\n");
    return 0;

}
static int lfs_truncate(const char *path, off_t size){
    (void)path;
    (void)size;
    return 0;
}

/*
static int lfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int fd;

    fd = open(path, fi->flags, mode);
    if (fd == -1)
        return -errno;
    fi->fh = fd;
    return 0;
}
*/

//*****************************************************************//
static struct fuse_operations u_operation = {
        .getattr = lfs_getattrr,
        .readdir = lfs_readdir,
        //.create = lfs_create,
        .mknod = lfs_mknod,
        .write = lfs_write2,
        .flush = lfs_flush,
        .open = lfs_open,
        //.release = lfs_release,
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
    file = malloc(sizeof(struct file_directory)*BLOCK_NUM(dev_size,block_size));
    //printf("file 是不是越界了啊：%ld\n",sizeof(struct file_directory)*BLOCK_NUM(dev_size,block_size));
    //argc--;
    res =fuse_main(argc-1, argv, &u_operation, NULL);
    close(fd);
    printf("Close fd\n");
    return 0;
}
