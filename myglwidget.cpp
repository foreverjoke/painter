#include "myglwidget.h"
#include <GL/glu.h>
#include <QKeyEvent>
#include <QTextStream>

MyGLWidget::MyGLWidget(QWidget *parent):
    QGLWidget(parent)
{
    fullscreen = false;
    rotating = false;
    xPos = 0.0f; yPos = 0.0f; zPos = -5.0f;
    xRotate = 0.0f; yRotate = 0.0f; zRotate = 0.0f;

    glClearColor(0.0f,0.0f,0.0f,0.0f);  //设置清屏颜色为黑色
    setGeometry(200,100,700,500);
    setWindowTitle("3D模型显示");
    //QString wd = QDir::currentPath();
    //QString openFileName = QFileDialog::getOpenFileName(this,tr("打开文件"),wd);
    //loadObject("piano-p865.off", &mPaint);                    //加载模型
    //updateGL();

    /*QTimer *timer = new QTimer(this);                   //创建一个定时器
    //将定时器的计时信号与updateGL()绑定
    connect(timer, SIGNAL(timeout()), this, SLOT(updateGL()));
    timer->start(100); */                                  //以10ms为一个计时周期

}

MyGLWidget::~MyGLWidget(){

}

void MyGLWidget::initializeGL(){
    //glClearColor(0.0,0.0,0.0,0.0);  //黑色背景
    glShadeModel(GL_SMOOTH);        //启用阴影平滑

    glClearDepth(1.0);          //设置深度缓存
    glEnable(GL_DEPTH_TEST);    //启用深度测试
    //glDepthFunc(GL_LEQUAL);     //所作深度测试的类型
    glDepthFunc(GL_LESS);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);   //告诉系统对透视进行修正

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

}

void MyGLWidget::resizeGL(int w, int h){
    glViewport(0,0,(GLint)w,(GLint)h);  //重置当前的视口
    glMatrixMode(GL_PROJECTION);        //选择投影矩阵
    glLoadIdentity();                   //重置投影矩阵

    //gluPerspective()用来设置透视投影矩阵，这里设置视角为45°，纵横比为窗口的纵横比
    //最近的位置为0.1，最远的位置为100，这两个值是场景中所能绘制的深度的临界值
    gluPerspective(45.0,(GLfloat)w/(GLfloat)h,0.1,100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void MyGLWidget::openFile(){
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //清除屏幕和深度缓存
    //glLoadIdentity();                                   //重置当前的模型观察矩阵

    QString wd = QDir::currentPath();
    QString openFileName = QFileDialog::getOpenFileName(this,tr("打开文件"),wd,"OFF files(*.off)");
    if(!openFileName.isEmpty())
        loadObject(openFileName,&mPaint);
}

void MyGLWidget::loadObject(QString filename,object3D* model)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);   //将要读入数据的文本打开
    QTextStream in(&file);
    in.readLine();                       //忽略第一行("OFF")
    QString line = in.readLine();
    (model->vNum) = line.split(" ").at(0).toInt();  //获取顶点数
    (model->fNum) = line.split(" ").at(1).toInt();  //获取面数

    //读取每个顶点的数据
    for(int i=0;i<model->vNum;i++){
        line = in.readLine();
        vertex getVertex;
        QTextStream oneLine(&line);
        oneLine>>getVertex.x>>getVertex.y>>getVertex.z;
        model->points.push_back(getVertex);
    }
    //读取每个面的数据
    for(int j=0;j<model->fNum;j++){
        line = in.readLine();
        face getFace;
        QTextStream oneLine(&line);
        oneLine>>getFace.numVertex;
        int tmp;
        for(int m=0;m<getFace.numVertex;m++){
            oneLine>>tmp;
            getFace.vIndexes.push_back(tmp);
        }
        model->faces.push_back(getFace);
    }

    file.close();
    updateGL();
}


void MyGLWidget::paintGL()                              //从这里开始进行所以的绘制
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //清除屏幕和深度缓存
    glLoadIdentity();                                   //重置当前的模型观察矩阵
    glTranslatef(xPos,yPos,zPos);                     //三维平移
    glRotatef(xRotate,1.0f,0.0f,0.0f);                //三维旋转
    glRotatef(yRotate,0.0f,1.0f,0.0f);
    glRotatef(zRotate,0.0f,0.0f,1.0f);
    glBegin(GL_TRIANGLES);                       //绘制由三角形组成的面
        for(int i=0;i<mPaint.fNum;i++){
            face getFace = mPaint.faces.at(i);
            vertex v1 = mPaint.points.at(getFace.vIndexes.at(0));
            vertex v2 = mPaint.points.at(getFace.vIndexes.at(1));
            vertex v3 = mPaint.points.at(getFace.vIndexes.at(2));

            glColor3f(1.0f, 1.0f, 1.0f);
            glVertex3d(v1.x,v1.y,v1.z);     //绘制顶点

            glColor3f(0.9f, 0.9f, 0.9f);    //改变颜色
            glVertex3d(v2.x,v2.y,v2.z);

            glColor3f(0.0f, 0.0f, 0.0f);    //改变颜色
            glVertex3d(v3.x,v3.y,v3.z);
        }
    glEnd();                                            //绘制结束

    /*xRotate += 1.0f;
    yRotate += 0.5f;
    zRotate += 0.3f;*/
}

//键盘事件处理
void MyGLWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_F1:                                    //F1为全屏和普通屏的切换键
        fullscreen = !fullscreen;
        if (fullscreen)
        {
            showFullScreen();
        }
        else
        {
            showNormal();
        }
        break;
    case Qt::Key_Escape:                                //ESC为退出键
        close();
        break;
    case Qt::Key_Left:
        yRotate += 1.0f;
        break;
    case Qt::Key_Right:
        yRotate -= 1.0f;
        break;

    }
    updateGL();
}
//滚轮事件处理
void MyGLWidget::wheelEvent(QWheelEvent *event){
    if(event->delta()>0)
        zPos += 0.1f;
    else
        zPos -= 0.1f;

    updateGL();
}

void MyGLWidget::mousePressEvent(QMouseEvent *event){
    rotating = true;
    pressPos = event->pos();
}

void MyGLWidget::mouseMoveEvent(QMouseEvent *event){
    if(rotating){
        currentPos = event->pos();
        zRotate -= (currentPos.x()-pressPos.x())*3/2;
        xRotate += (currentPos.y()-pressPos.y())*3/2;

    }
    pressPos = currentPos;
    updateGL();
}

void MyGLWidget::mouseReleaseEvent(QMouseEvent*){
    rotating = false;
}

/*void MyGLWidget::closeEvent(QCloseEvent*){
    glClearColor(0.0f,0.0f,0.0f,0.0f);
    glClear(GL_COLOR_BUFFER_BIT || GL_DEPTH_BUFFER_BIT);
}*/












