#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>

#include <sys/ioctl.h>  
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <jpeglib.h>




#define H 480
#define W 640


//定义结构体来储存映射的首地址的映射内存块的大小
struct camerebuf
{
    void *start; //每个缓冲内存的首地址
    int somelength;//保存的内存块的大小
};
//像素结构体
struct  rgb
{
    int pix; //rgb
    int Apix; //argb
};


//封装 yuv---->argb
struct  rgb toRGB(int y,int u, int v)
{
    int r,g,b;
    struct  rgb newrgb;

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
    newrgb.Apix = 0x00<<24 | r<<16 | g<<8 | b;
    newrgb.pix = r<<16 | g<<8 | b;
    return newrgb;

}
/*RGB create jpeg  in location*/
int rgbtojeg(int *rgbdata,char *filename)
{
    //定义压缩结构体
    struct jpeg_compress_struct myconpress;
    jpeg_create_compress(&myconpress);

    //定义错误结构体
    struct jpeg_error_mgr myerror;
    myconpress.err = jpeg_std_error(&myerror);


    //设置压缩参数
    myconpress.image_width =W;
    myconpress.image_height = H;
    myconpress.input_components = 3;
    myconpress.in_color_space = JCS_RGB;
    jpeg_set_defaults(&myconpress);

    //设置压缩比例
    jpeg_set_quality(&myconpress,80,TRUE);

    //建立一个空白jpg文件
    FILE *myjpg = fopen(filename,"w+");
        if(myjpg == NULL)
        {
            perror("fopen");
            return -1;
        }

    //绑定输出
    jpeg_stdio_dest(&myconpress,myjpg);

    //开始压缩
    jpeg_start_compress(&myconpress,TRUE);
    JSAMPARRAY array[1];
    int i,j;
    //把压缩后的数据写入空白的jpeg
    for(i = 0 ; i<H;i++)
    {
        array[0] = rgbdata+W*3*i;
        jpeg_write_scanlines(&myconpress,array,1);
    }


    //收尾工作
    jpeg_finish_compress(&myconpress);
    jpeg_destroy_compress(&myconpress);
    fclose(myjpg);
    return 0;



}
/*YUYV-->RGB/ARGB*/
int allyuvtoRGB(char *yubdata,int *argbbuf,int *rgbbuf)
{
    for (int i = 0 ,j = 0; j < H*W; i+=4,j+=2)
    {
        //第一组数组
        argbbuf[j] = toRGB(yubdata[i],yubdata[i+1],yubdata[i+3]).Apix;
        rgbbuf[j] = toRGB(yubdata[i],yubdata[i+1],yubdata[i+3]).pix;

        //第二组数组
        argbbuf[j+1] = toRGB(yubdata[i+2],yubdata[i+1],yubdata[i+3]).Apix;
        rgbbuf[j+1] = toRGB(yubdata[i+2],yubdata[i+1],yubdata[i+3]).pix;

    }
    return 0;

}




int main()
{
    //打开摄像头驱动
    int camerafd = open("/dev/video7",O_RDWR);
            if(camerafd == -1 )
            {
                perror("open");
                return -1;

            }
    int lcdfd = open("/dev/fb0",O_RDWR);
            if(lcdfd == -1 )
            {
                perror("open");
                return -1;

            }
    //映射lcd的首地址
    int *lcdbuf = mmap(NULL, 800*480*4, PROT_READ|PROT_WRITE,MAP_SHARED,
                  lcdfd,0);
                  if(lcdbuf == MAP_FAILED)
                {
                    printf("LCD %s",strerror(errno));
                    return -1;
                }
    


    int ret;
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
    //定义结构体数据存放四个缓存区的信息
    struct camerebuf array[4];
    struct v4l2_buffer otherbuf;
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



    //循环出对入队显示画面
    int  argbbuf[H*W]; 
    int  rgbbuf[H*W];  
    while (1)
    {
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
            allyuvtoRGB(array[i].start,argbbuf,rgbbuf);


            //将lcdbuf写入lcd映射中
            for (int j = 0; j < H; j++)
                memcpy(lcdbuf+80+800*j,&argbbuf[W*j],W*4);

            //将获取到的RGB输入到本地图片
            rgbtojeg(rgbbuf,"2.jpg");
            
        }

        
    }

}