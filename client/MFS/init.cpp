//
// Created by chen on 23-3-28.
//
#include "common.h"
#include <vector>
#include <string>
#include <iostream>
using namespace std;
int64_t dev_size=429496729600;
int block_size=BLOCK_SIZE;
FILE* disk = fopen("./disk", "r+");
FILE* writeinfo = fopen("./writeinfo","w+");

FILE* slave1fp = fopen("./slave1","w+");
FILE* slave2fp = fopen("./slave2","w+");
FILE* nodefp = fopen("./node","w+");

/*int init_disk(){
    int bi_len = INFO_SIZE(dev_size,block_size);
    struct block_info *bi=(struct block_info *)malloc(bi_len);
    uint64_t dev_offset;
    dev_offset = INFO_OFFSET(dev_size,block_size);
    fseek(disk,dev_offset,SEEK_SET);
    fread(bi, bi_len, 1,disk);
    unsigned char *p_bitmap=read_bitmap(disk,dev_size,block_size);
    int bit_size=BLOCK_NUM(dev_size,block_size);
    int bm_list_len=0;
    int *bm_list = bitmap_usedblockid(p_bitmap,bit_size,0,bit_size,&bm_list_len);
    int bm_data_block_id;
    for (int i = 0; i < bit_size; ++i) {
        bi[i].b_block_id=-1;
    }
    fseek(disk,dev_offset,SEEK_SET);
    fwrite(bi,bi_len,1,disk);
}*/
/*int create_file(const char* path, int flag) {
    char id[20];
    int block_id;
    sscanf(path,"%*[^_]_%[^_]_", id);
    block_id=atoi(id);
    int bi_len = INFO_SIZE(dev_size, block_size);
    struct block_info *bi=(struct block_info *)malloc(bi_len);
    uint64_t dev_offset;
    dev_offset = INFO_OFFSET(dev_size,block_size);
    fseek(disk,dev_offset,SEEK_SET);
    fread(bi, bi_len, 1,disk);
    unsigned char *p_bitmap=read_bitmap(disk,dev_size,block_size);
    int bit_size=BLOCK_NUM(dev_size,block_size);
    int bm_list_len=0;
    bi[block_id].b_block_id=block_id;
    fseek(disk,dev_offset,SEEK_SET);
    fwrite(bi,1,bi_len,disk);
}*/

void getfileblk() {
    char buf[1024] = "\0";
    int bi_len = INFO_SIZE(dev_size, block_size);
    struct block_info *bi=(struct block_info *)malloc(bi_len);
    uint64_t dev_offset;
    dev_offset = INFO_OFFSET(dev_size,block_size);
    //fseek(disk,dev_offset,SEEK_SET);
    //fread(bi, bi_len, 1,disk);
    FILE* infofile = fopen("./filesblk", "r");
    /*if(infofile == NULL){
        printf("err open infofile\n");
    }*/
    printf("ok\n");
    off_t offe;
    char namebuf[20];
    vector<string> path_vec;
    vector<int> blk_id;
    vector<string> datanode;
    while (fgets(buf, 1024, infofile) != NULL) {
        if (buf[0] == 'f' && buf[1] == 'i' && buf[2] == 'l' && buf[3] == 'e') {
            /*for (int i = 0; i  <  path_vec.size(); ++i) {
                unsigned char *p_bitmap=read_bitmap(disk,dev_size,block_size);
                int bit_size=BITMAP_SIZE(dev_size,block_size);
                int bm_list_len=0;
                int a[40];
                int len=bitmap_availableblockid(path_vec.size(),bit_size,a,p_bitmap);
                for (int j = 0; j < len; ++j) {
                    blk_id.push_back(a[j]);
                    bitmap_set(a[j],p_bitmap,bit_size);
                }
                write_bitmap(disk,dev_size,block_size,bit_size,p_bitmap);
            }*/
            char outinfo[20];
            for (int i = 0; i  <  path_vec.size(); ++i){
                sprintf(outinfo,"%s %d\n",path_vec[i].c_str(),blk_id[i]);
                fwrite(outinfo,1,strlen(outinfo),writeinfo);
            }
            path_vec.clear();
            blk_id.clear();
            printf("%d\n", strlen(buf));
            printf("%s\n", buf);
            //namebuf = buf + 5;
            strcpy(namebuf, buf + 5);
            namebuf[strlen(namebuf) - 1] = '\0';
            //create_file(namebuf,0);
            //fwrite(buf,sizeof(char),strlen(buf) ,inforawhd);
            //fprintf(inforawhd,"\n");
            offe = 0;
        }

        if (buf[0] == 'b' && buf[1] == 'l' && buf[2] == 'o' && buf[3] == 'c' && buf[4] == 'k') {
            //fwrite(buf,sizeof(char),strlen(buf) ,inforawhd);
            printf("%s\n", buf);
            fgets(buf, 1024, infofile);
            buf[strlen(buf) - 1] = '\0';
            path_vec.push_back(buf);
            fgets(buf, 1024, infofile);
            buf[strlen(buf) - 1] = '\0';
            datanode.push_back(buf);
            
            //(buf, namebuf);
            //continue;
        }
    }
    //free(buf);
    /*for (int i = 0; i  <  path_vec.size(); ++i) {
        unsigned char *p_bitmap=read_bitmap(disk,dev_size,block_size);
        int bit_size=BITMAP_SIZE(dev_size,block_size);
        int bm_list_len=0;
        int a[2000];
        int len=bitmap_availableblockid(path_vec.size(),bit_size,a,p_bitmap);
        for (int j = 0; j < len; ++j) {
            blk_id.push_back(a[j]);
            //bitmap_set(a[j],p_bitmap,bit_size);
        }
        //write_bitmap(disk,dev_size,block_size,bit_size,p_bitmap);
    }*/
    
    char outinfo[2000];
     for (int i = 0; i  <  path_vec.size(); ++i){
        blk_id.push_back(i+1);
     }
    for (int i = 0; i  <  path_vec.size(); ++i){
        sprintf(outinfo,"%s\n%d\n",path_vec[i].c_str(),blk_id[i]);
        printf("%s %d\n",path_vec[i].c_str(),blk_id[i]);
        std::cout<<datanode[i]<<std::endl;
        if(datanode[i]=="slave1")
        fwrite(outinfo,1,strlen(outinfo),slave1fp);
        else if(datanode[i]=="slave2")
        fwrite(outinfo,1,strlen(outinfo),slave2fp);
        else
            fwrite(outinfo,1,strlen(outinfo),nodefp);
    }
    fclose(infofile);
}
long write2disk(string path_vec,int blk_id) {
    char buf[1024];
    int len;
    long end_off;
    FILE *fp= fopen(path_vec.c_str(),"r");
    while(len=fread(buf,sizeof(char),1024,fp)){
        fseek(fp,blk_id*BLOCK_SIZE,SEEK_SET);
        fwrite(buf,sizeof(char),len,fp);
    }
    end_off=ftell(fp);
    fclose(fp);
    return end_off%BLOCK_SIZE;
}
int getinfo(){
    char buf[1024];
    FILE *fp=fopen("writeinfo","r");
    while (fgets(buf, 1024, fp) != NULL){
        char tbuf[1024];
        fgets(tbuf, 1024, fp);
        write2disk(buf,atoi(tbuf));
    }
    
}
int main(){

   /* int bit_size=BITMAP_SIZE(dev_size,block_size);
    int blknum= BLOCK_NUM(dev_size,block_size);
    unsigned char buf[100];
    for (int i = 0; i < bit_size; ++i) {
        buf[i]=0X00;
    }
    int len=write_bitmap(disk, dev_size, block_size, sizeof(buf), buf);
    unsigned char *p_bitmap=read_bitmap(disk,dev_size,block_size);
    print_bitmap(p_bitmap,blknum);
    free(p_bitmap);*/
    
    getfileblk();
    
}
