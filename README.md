# nvmf-over-hdfs
测试可分为两部分：
1.HDFS导入导出测试
2.文件系统测试

1.HDFS导入导出测试：

1）元数据生成
desc:根据hdfs fsck命令读取根目录/的元数据信息，并输出到fileblk文件中
cd hdfstest
make clean
make
生成readhdfs可执行程序
运行./readhdfs 生成fileblk文件

读取元数据并输出到fileblk文件中
source /etc/profile
source run.sh
make
./readhdfs

2）文件系统分配空间
init.cpp-desc：通过自建的文件系统MFS，根据元数据信息（fileblk信息）分配磁盘空间,读取filesblk 输出到slave1、slave2、node文件中
cd newfs
1、删除之前的cmake cmake .
2、cd cmake-build-debug cmake ..
3、make 

3）DataNode写入文件
writetest-desc：将生成的slave1、slave2文件复制到对应的DataNode节点中
在test文件夹下执行./writetest.cpp

问题：编译过程中 若cmake版本提示过低，可以修改CMakeLists.txt中的版本号为当前版本号


2.文件系统测试：
两个版本：
v1 LiFS-from黎梦钰 实现了读取的功能

note：
需要安装配置libfuse
https://github.com/libfuse/libfuse
rewrite.c是没有注释版的最终代码

直接make会报错
[root@slave1 lisafuse]# make
cc    -c -o ltils.o ltils.c
ltils.c:17:18: fatal error: fuse.h: No such file or directory
 #include <fuse.h>
                  ^
compilation terminated.
make: *** [ltils.o] Error 1

编译LisaFS命令
gcc -Wall bitmap.c rewrite.c hdfslowfs.h `pkg-config fuse3 --cflags --libs` -o rewrite

赋权
chmod 777 /dev/nvme0n1

运行LiFS
mkdir fusetest
./rewrite ./fusetest/ -d /dev/nvme0n1

v2 MFS-from 陈泳豪 扩展了LiFS，但有bug
代码中需要修改参数
init_disk.cpp
FILE* disk = fopen("./disk", "r+");
改为：FILE* disk = fopen("./nvme0n1", "r+");
char *disk_path="/dev/nvme0n1";


mkdir fuse

./MFS -d fuse
重新开启一个新的终端来执行文件系统操作

这个文件系统和性能无关，主要是为了在裸盘上实现读写