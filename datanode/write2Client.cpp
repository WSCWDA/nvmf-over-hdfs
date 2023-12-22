#include<iostream>
#include<string.h>
#include <cstdio>
#include<stdlib.h>
using namespace std;
#define BLOCK_SIZE (long)(128*1024*1024)
FILE *disk= fopen("/dev/nvme0n1","r+");
long write2disk(string path_vec,int blk_id) {
    char buf[1024];
    int len;
    long end_off;
    char *path= const_cast<char *>(path_vec.c_str());
    path[strlen(path) - 6] = '\0'; 
    FILE *fp= fopen(path,"r");
    fseek(disk,blk_id*BLOCK_SIZE,SEEK_SET);
    while(len=fread(buf,sizeof(char),1024,fp)){
        //printf("%d\n",len);
        fwrite(buf,sizeof(char),len,disk);
    }
    end_off=ftell(disk);
    fclose(fp);
    return end_off%BLOCK_SIZE;
}
int getinfo(){
    char buf[1024];
    FILE *infofp=fopen("./slave1","r");
    while (fgets(buf, 1024, infofp) != NULL){
        char tbuf[1024];
        fgets(tbuf, 1024, infofp);
        printf("%s %s\n",buf,tbuf);

        //write2disk(buf,atoi(tbuf));
        
    }
}
int main(){
    getinfo();
}
