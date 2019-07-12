#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QAction>
#include <QList>
#include <QFileDialog>      //文件系统
#include <QColorDialog>     //颜色系统
#include <QMessageBox>      //标准对话框
#include <QInputDialog>     //接收输入的对话框
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QVector>
#include "paintboard.h"
#include "myglwidget.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    //实现QWidget的dragEnterEvent()和dropEvent()函数
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
private slots:
    void open();
    void save();
    void open3D();

private:
    Ui::MainWindow *ui;
    bool maybeSave();
    bool saveFile(const QByteArray &fileFormat);
    PaintBoard *mypaintingboard;
    static int countGLWidgets;
    MyGLWidget* glWidgets[100];
    //MyGLWidget* myglWidget;     //三维图形显示窗口

    //文件操作
    QAction* openfile;
    QList<QAction *>savefileas;
    QAction* exit;

    //不同的操作类型
    QAction* drawpen;
    QAction* drawline;
    QAction* drawrect;
    QAction* drawcircle;
    QAction* drawellipse;
    QAction* drawbrush;   //画刷(填充)
    QAction* selectArea;  //选择区域
    QAction* moveArea;    //平移区域
    QAction* rotateClockWise;           //顺时针旋转
    QAction* rotateCounterClockWise;    //逆时针旋转
    QAction* zoomInArea;    //放大
    QAction* zoomOutArea;   //缩小
    QAction* cutLine;       //裁剪线段(CS算法)
    QAction* cutPolygon;    //裁剪多边形(SH算法)
    QAction* cutArea;       //裁剪区域
    QAction* removeArea;    //删除

    QAction* open3DModel;   //打开3D模型

    //颜色切换
    QAction* colorRed;
    QAction* colorGreen;
    QAction* colorYellow;
    QAction* colorBlue;
    QAction* colorCyan;
    QAction* colorPurple;
    QAction* colorOrange;
    QAction* colorBlack;
    QAction* colorWhite;

    //文件菜单和另存为菜单
    QMenu* filemenu;
    QMenu* saveasmenu;


};

#endif // MAINWINDOW_H
