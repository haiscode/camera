#ifndef CAMERA_H
#define CAMERA_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev2.h>
//#include <jpeglib.h>

#define H 480
#define W 640

//定义结构体来储存映射的首地址的映射内存块的大小
struct camerebuf
{
    void *start; //每个缓冲内存的首地址
    int somelength;//保存的内存块的大小
};

class camera
{
public:
    camera();
    //摄像头初始化
    int camera_init();
    //捕捉出对入队画面
    int camera_cap();
    //关闭摄像头
    int camera_close();

    int toRGB(int y,int u, int v);

    int allyuvtoRGB(char *yubdata,int *argbbuf);

private:
    int camerafd;
    int lcdfd;
    void *lcdbuf;
    //定义结构体数据存放四个缓存区的信息
    struct camerebuf array[4];
    struct v4l2_buffer otherbuf;
    int ret;

};

#endif // CAMERA_H
