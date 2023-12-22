#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
/*初始化bitmap*/

#include "hdfslowfs.h"

unsigned char * bitmap_init(int size)
{
    int len=size/8 + 1;
	unsigned char *p_bitmap = (unsigned char*)malloc(len);
	if (p_bitmap == NULL)
		return NULL;

    //将bitmap指向的存储空间初始值置为0
	memset(p_bitmap, 0, len);

	return p_bitmap;
}

unsigned char * read_bitmap(int fd, int dev_size, int block_size)
{
    int len=BITMAP_SIZE(dev_size,block_size);
    unsigned char *p_bitmap = (unsigned char*)malloc(len);
    int offset= BITMAP_OFFSET;

    lseek(fd,offset,SEEK_SET);
    read(fd,p_bitmap,len);

    return p_bitmap;
}

int write_bitmap(int fd, int dev_size, int block_size, int len, char *buf)
{
    int offset= BITMAP_OFFSET;

    lseek(fd,offset,SEEK_SET);
    int ret= write(fd,buf,len);
    return ret;
}

unsigned char *find_bitmap_char(int index, unsigned char *p_bitmap, int size)
{
    if (index>=size)
        return NULL;

    int i = floor(index/8);


    unsigned char * p = p_bitmap+i;

 //   printf("i=%d,*p=0x%x",i,*p);

    return p;
}
//设置bitmap第index位的值为1
unsigned char bitmap_set(int index,unsigned char* p_bitmap,int size)
{
    if (index>=size)
        return 0;

    unsigned char *p=find_bitmap_char(index, p_bitmap, size);
    int r = index % 8;
    unsigned char mask = 0x01<<r;

    *p=*p|mask;

    return *p;
 }

 unsigned char bitmap_unset(int index,unsigned char* p_bitmap,int size)
{
    if (index>=size)
        return 0;

    unsigned char *p=find_bitmap_char(index, p_bitmap, size);
    int r = index % 8;
    unsigned char mask = 0x01<<r;
    mask=~mask;

    *p=*p&mask;

    return *p;
 }


//返回第index位对应的值
int bitmap_get(int index,unsigned char* p_bitmap,int size)
{
    if (index>=size)
        return 0;

    unsigned char *p=find_bitmap_char(index, p_bitmap, size);
    int r = index % 8;
    unsigned char mask = 0x01<<r;
//    printf("get %d bit=%d,%d\n",index, *p&mask,(*p&mask)>0);
    if ((*p&mask)>0)
        return 1;
    else
        return 0;
}

int * bitmap_usedblockid(unsigned char* p_bitmap,int size, int start, int end,int *list_len)
{
    int len=(end-start)*sizeof(int);
    int *p_blocklist=(int *)malloc(len);
    for(int i=0;i<(end-start);i++)
        p_blocklist[i]=-1;
    memset(p_blocklist,-1,len);
    int index=0;
    for(int i=start;i<end;i++)
    {
        if (bitmap_get(i,p_bitmap,size)>0)
        {
            p_blocklist[index]=i;
            index=index+1;
        }
    }
    *list_len = index;
    //printf("list_len is %d\n",*list_len);
    return p_blocklist;
}



int bitmap_availableblockid(int n,int size,int * availablelist,unsigned char* p_bitmap)
{
    int index = 0;
    for(int i = 0;i<size;i++)
    {
        if (bitmap_get(i,p_bitmap,size)==0)
            availablelist[index++]=i;
        if (index==n)
            break;
    }
    return index;
 }

 void print_bitmap(unsigned char *p_bitmap, int size)
 {
     int flag;
     for(int i=0;i<size;i++)
     {
         printf("%3d:%d\t",i,bitmap_get(i,p_bitmap,size));
         flag++;
         if (flag%10==0)
            printf("\n");
     }
     printf("\n");
 }



