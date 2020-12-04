#include "camera.h"

camera::camera()
{

}

int camera::camera_init()
{
    //打开摄像头驱动
        camerafd = open("/dev/video7",O_RDWR);
                if(camerafd == -1 )
                {
                    perror("open");
                    return -1;

                }
        lcdfd = open("/dev/fb0",O_RDWR);
                if(lcdfd == -1 )
                {
                    perror("open");
                    return -1;

                }
        //映射lcd的首地址
        lcdbuf = mmap(NULL, 800*480*4, PROT_READ|PROT_WRITE,MAP_SHARED,
                      lcdfd,0);
                      if(lcdbuf == MAP_FAILED)
                    {
                        printf("LCD %s",strerror(errno));
                        return -1;
                    }




        //指定格式
        struct v4l2_format myfix;
        bzero(&myfix,sizeof(myfix));
        myfix.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        myfix.fmt.pix.width = W;
        myfix.fmt.pix.height = H;
        myfix.fmt.pix.pixelformat =V4L2_PIX_FMT_YUYV;
        //发送命令
        ret = ioctl(camerafd,VIDIOC_S_FMT,&myfix);
            if(ret == -1)
            {
                perror("设置采集格式失败!\n");
                return -1;
            }

        //跟驱动申请缓存区
        struct v4l2_requestbuffers mybuf;
        bzero(&mybuf,sizeof(mybuf));
        mybuf.count = 4; //申请4快缓存区
        mybuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mybuf.memory = V4L2_MEMORY_MMAP;
        //发送命令
        ret = ioctl(camerafd,VIDIOC_REQBUFS,&mybuf);
            if(ret == -1)
            {
                perror("申请缓冲块失败!\n");
                return -1;
            }

        //分配刚才申请的4个缓存区 ----》0，1，2，4 队列
        //顺便映射得到每个缓存区的首地址


        for (int i = 0; i < 4; i++)
        {
            bzero(&otherbuf,sizeof(otherbuf));

            otherbuf.index = i;
            otherbuf.memory = V4L2_MEMORY_MMAP;
            otherbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            ret = ioctl(camerafd,VIDIOC_QUERYBUF,&otherbuf);
            if(ret == -1)
            {
                perror("分配缓冲块失败!\n");
                return -1;
            }
            //将地址映射到虚拟内存中
            array[i].start = mmap(NULL,otherbuf.length, PROT_READ|PROT_WRITE,MAP_SHARED,
                      camerafd,otherbuf.m.offset);//otherbuf.m.offset获取内存地址偏移量
                    if(array[i].start == MAP_FAILED)
                    {
                        perror("映射缓冲块失败!\n");
                        return -1;
                    }



            array[i].somelength = otherbuf.length;
            //画面入队
            ret = ioctl(camerafd,VIDIOC_QBUF,&otherbuf);
            if(ret == -1)
            {
                perror("入队失败!\n");
                return -1;
            }
        }



        //启动摄像头进行画面采集

        enum v4l2_buf_type mytype = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ret = ioctl(camerafd,VIDIOC_STREAMON,&mytype);
            if(ret == -1)
            {
                perror("启动摄像头采集失败!\n");
                return -1;
            }
        return  0;

}

int camera::camera_cap()
{
    int  argbbuf[H*W];
    for (int i = 0; i < 4; i++)
    {

        bzero(&otherbuf,sizeof(otherbuf));
        otherbuf.index = i;
        otherbuf.memory = V4L2_MEMORY_MMAP;
        otherbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        //画面出队
        ret = ioctl(camerafd,VIDIOC_DQBUF,&otherbuf);
        if(ret == -1)
        {
            perror("出队失败");
            return -1;
        }

        //画面入队
        ret = ioctl(camerafd,VIDIOC_QBUF,&otherbuf);
        if(ret == -1)
        {
            perror("入队失败");
            return -1;
        }

        //把出队画面从lcd上显示出来
        //将array[i].start YUV格式的内容转换成ARGB格式在lcd屏幕上去显示
        allyuvtoRGB((char*)array[i].start,argbbuf);

        //获取rgb数据
        //allyuvtoRGB2(array[i].start,rgbbuf);
        //将获取到的rgb数据压缩并写入jpg文件中
        //rgbtojeg(rgbbuf,"2.jpg");
        //将lcdbuf写入lcd映射中
        for (int j = 0; j < H; j++)
            memcpy((int *)lcdbuf+80+800*j,&argbbuf[W*j],W*4);

    }
    return  0 ;
}

//封装 yuv---->argb
int camera::toRGB(int y,int u, int v)
{
    int r,g,b;
    int pix;

    //通过公式进行转换
    r= y + ((360 * (v - 128))>>8) ;
    g= y - ( ( ( 88 * (u - 128)  + 184 * (v - 128)) )>>8) ;
    b= y + ((455 * (u - 128))>>8) ;
    //修正结果
    if(r>255)
        r=255;
    if(g>255)
        g=255;
    if(b>255)
        b=255;

    if(r<0)
        r=0;
    if(g<0)
        g=0;
    if(b<0)
        b=0;
    pix = 0x00<<24 | r<<16 | g<<8 | b;

    return pix;

}

/*YUYV-->ARGB*/
int camera::allyuvtoRGB(char *yubdata,int *argbbuf)
{
    for (int i = 0 ,j = 0; j < H*W; i+=4,j+=2)
    {
        //第一组数组
        argbbuf[j] = toRGB(yubdata[i],yubdata[i+1],yubdata[i+3]);


        //第二组数组
        argbbuf[j+1] = toRGB(yubdata[i+2],yubdata[i+1],yubdata[i+3]);


    }
    return 0;

}
int  camera::camera_close()
{
    //关闭摄像头
    enum v4l2_buf_type mytype = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(camerafd,VIDIOC_STREAMOFF,&mytype);
        if(ret == -1)
        {
            perror("关闭摄像头采集失败!\n");
            return -1;
        }
    close(lcdfd);
    close(camerafd);
    munmap(lcdbuf,800*480*4);
    return  0;
}
