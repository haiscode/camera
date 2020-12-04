#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    carme = new camera;
    //初始化摄像头
    carme->camera_init();

    //QTimer
    timer = new QTimer(this);

    connect(timer,SIGNAL(timeout()),this,SLOT(cappic()));
}

Widget::~Widget()
{
    carme->camera_close();
    delete ui;
}


void Widget::on_pushButton_clicked()
{
    //定时器
    timer->start(1);

    //线程


}

void Widget::cappic()
{
    carme->camera_cap();
}

void Widget::on_pushButton_2_clicked()
{
   timer->stop();

}
