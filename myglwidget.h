#ifndef MYGLWIDGET_H
#define MYGLWIDGET_H

#include <QWidget>
#include <QGLWidget>
#include <QEvent>
#include <QVector>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QPoint>
#include <QCloseEvent>
struct vertex{      //顶点结构体
    float x,y,z;
};
struct face{        //面结构体
    int numVertex;  //顶点个数
    QVector<int> vIndexes;  //顶点的索引
};

struct object3D{      //物体结构体
    int vNum,fNum;      //物体的顶点数和面数
    QVector<vertex> points;    //包含顶点数据的向量
    QVector<face> faces;        //包含面数据的向量
};

class MyGLWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit MyGLWidget(QWidget *parent = 0);
    ~MyGLWidget();
    void openFile();     //打开模型

protected:
    //对3个纯虚函数的重定义
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void keyPressEvent(QKeyEvent *event);   //处理键盘事件
    void wheelEvent(QWheelEvent *event);    //滚轮事件
    void mousePressEvent(QMouseEvent *event);   //鼠标事件
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent*);

    //void closeEvent(QCloseEvent *event);    //窗口关闭事件

private:
    void loadObject(QString filename,object3D* model);    //从文件加载一个模型
    bool fullscreen;        //是否全屏显示
    bool rotating;          //是否正在旋转
    QPoint pressPos;        //鼠标按下位置
    QPoint currentPos;      //鼠标当前位置

    object3D mPaint;          //要绘制的物体
    GLfloat xPos;          //x轴坐标
    GLfloat yPos;          //y轴坐标
    GLfloat zPos;          //z轴坐标
    GLfloat xRotate;        //3个方向的旋转角度
    GLfloat yRotate;
    GLfloat zRotate;

};

#endif // MYGLWIDGET_H
