#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>

#include <jpeglib.h>

int main(int argc, char const *argv[])
{
    //定义解压缩体和处理错误结构体并初始化
    struct jpeg_decompress_struct mydecom;
    jpeg_create_decompress(&mydecom);

    struct jpeg_error_mgr myerror;
    mydecom.err = jpeg_std_error(&myerror);

    //打开需要显示的jpg图片
    FILE * myjpg = fopen("1.jpg","r+");
        if(myjpg == NULL)
        {
            perror("myjpg");
            return -1;
        }
    
    //指定解压数据源
    jpeg_stdio_src(&mydecom,myjpg);

    //读取jpg的头信息
    jpeg_read_header(&mydecom,TRUE);

    //开始解压缩
    jpeg_start_decompress(&mydecom);


    //打印图片的宽高
    printf("W:%d\n",mydecom.image_width);
    printf("H:%d\n",mydecom.image_height);





    return 0;
}
