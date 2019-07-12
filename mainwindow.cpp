#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMenu>
#include <QImageWriter>
#include <QString>
#include <QDir>
#include <QActionGroup>
#include <QMimeData>

int MainWindow::countGLWidgets = 0;
//TODO:(BUG)打开一个三维模型，再接着打开另一个一个，发生显示错误
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //设置窗口名称
    setWindowTitle("MyPaintingBoard");
    //设置窗口启用拖动
    setAcceptDrops(true);
    //创建画板
    mypaintingboard=new PaintBoard;
    //创建3D面板
    for(int i=0;i<100;i++)
        glWidgets[i] = new MyGLWidget;
    //myglWidget = new MyGLWidget;
    //将画板显示在屏幕中间
    setCentralWidget(mypaintingboard);

    //创建"打开"动作
    openfile=new QAction(QIcon(":/icons/open.ico"),tr("打开"),this);
    openfile->setShortcut(tr("Ctrl+O"));
    openfile->setToolTip(tr("打开"));
    connect(openfile,SIGNAL(triggered()),this,SLOT(open()));


    //遍历QImageWriter支持的所有类型,依次为其添加Action
    foreach(QByteArray format,QImageWriter::supportedImageFormats()){
        //QByteArray可直接通过强制类型转换得到其它类型的数据
        //把类型的名称格式化为QString,同时全部转换为小写字母
        QString text=tr("%1").arg(QString(format).toLower());
        QAction* tmp=new QAction(text,this);    //"文本,父组件"的构造函数
        tmp->setData(format);   //根据format设置数据类型
        //每个动作都连接到save()槽
        connect(tmp,SIGNAL(triggered()),this,SLOT(save()));
        savefileas.append(tmp); //向"另存为"动作列表中加入一个动作
    }

    //主菜单栏添加"文件"菜单
    filemenu=new QMenu(tr("文件"),this);
    ui->menuBar->addMenu(filemenu);
    //新建"另存为"菜单并添加支持的格式(每种格式对应一个QAction)
    saveasmenu=new QMenu(tr("另存为"),this);
    saveasmenu->setIcon(QIcon(":/icons/save_as.ico"));  //设置"另存为"菜单的图标
    saveasmenu->addActions(savefileas);
    //向文件菜单中依次添加"打开"(QAction)和"另存为"(QMenu)
    filemenu->addAction(openfile);
    filemenu->addMenu(saveasmenu);
    //工具栏也添加"打开"动作
    ui->mainToolBar->addAction(openfile);

    //创建退出动作
    exit=new QAction(QIcon(":/icons/exit.ico"),tr("退出"),this);
    exit->setShortcut(tr("Ctrl+E"));
    exit->setToolTip(tr("退出"));
    //exit动作将触发close()槽(QWidget自带函数)
    connect(exit,SIGNAL(triggered()),this,SLOT(close()));
    filemenu->addAction(exit);

    //创建自由画线动作
    drawpen=new QAction(QIcon(":/icons/pen.ico"),tr("自由画线"),this);
    drawpen->setToolTip(tr("自由画线"));
    drawpen->setCheckable(true);
    drawpen->setChecked(true);
    connect(drawpen,SIGNAL(triggered()),mypaintingboard,SLOT(setstatuspen()));
    //创建画直线动作
    drawline=new QAction(QIcon(":/icons/line.ico"),tr("直线"),this);
    drawline->setToolTip(tr("直线"));
    drawline->setCheckable(true);
    connect(drawline,SIGNAL(triggered()),mypaintingboard,SLOT(setstatusline()));
    //创建画矩形动作
    drawrect=new QAction(QIcon(":/icons/rectangle.ico"),tr("矩形"),this);
    drawrect->setToolTip(tr("矩形"));
    drawrect->setCheckable(true);
    connect(drawrect,SIGNAL(triggered()),mypaintingboard,SLOT(setstatusrect()));
    //创建画圆动作
    drawcircle=new QAction(QIcon(":/icons/circle.ico"),tr("圆"),this);
    drawcircle->setToolTip(tr("圆"));
    drawcircle->setCheckable(true);
    connect(drawcircle,SIGNAL(triggered()),mypaintingboard,SLOT(setstatuscircle()));
    //添加画椭圆动作
    drawellipse=new QAction(QIcon(":/icons/ellipse.ico"),tr("椭圆"),this);
    drawellipse->setToolTip(tr("椭圆"));
    drawellipse->setCheckable(true);
    connect(drawellipse,SIGNAL(triggered()),mypaintingboard,SLOT(setstatusellipse()));
    //添加画刷动作
    drawbrush=new QAction(QIcon(":/icons/brush.ico"),tr("画刷"),this);
    drawbrush->setToolTip(tr("画刷"));
    drawbrush->setCheckable(true);
    connect(drawbrush,SIGNAL(triggered()),mypaintingboard,SLOT(setstatusbrush()));
    //添加选中区域动作
    selectArea = new QAction(QIcon(":/icons/choose.ico"),tr("选择"),this);
    selectArea->setToolTip(tr("选择一块区域"));
    selectArea->setCheckable(true);
    connect(selectArea,SIGNAL(triggered()),mypaintingboard,SLOT(setstatusSelect()));
    //添加平移区域动作
    moveArea = new QAction(QIcon(":/icons/move.ico"),tr("平移"),this);
    moveArea->setToolTip(tr("平移区域"));
    moveArea->setCheckable(true);
    connect(moveArea,SIGNAL(triggered()),mypaintingboard,SLOT(setstatusMove()));
    //旋转
    rotateClockWise = new QAction(QIcon(":/icons/clockwise.ico"),tr("顺时针旋转"),this);
    rotateClockWise->setToolTip(tr("顺时针旋转"));
    connect(rotateClockWise,SIGNAL(triggered(bool)),mypaintingboard,SLOT(RotateClockWise()));
    rotateCounterClockWise = new QAction(QIcon(":/icons/counterclockwise.ico"),tr("逆时针旋转"),this);
    rotateCounterClockWise->setToolTip(tr("逆时针旋转"));
    connect(rotateCounterClockWise,SIGNAL(triggered(bool)),mypaintingboard,SLOT(RotateCounterClockWise()));
    //放大
    zoomInArea = new QAction(QIcon(":/icons/zoomIn.ico"),tr("放大"),this);
    zoomInArea->setToolTip(tr("放大"));
    //zoomInArea->setCheckable(true);
    connect(zoomInArea,SIGNAL(triggered(bool)),mypaintingboard,SLOT(ZoomInArea()));
    //缩小
    zoomOutArea = new QAction(QIcon(":/icons/zoomOut.ico"),tr("缩小"),this);
    zoomOutArea->setToolTip(tr("缩小"));
    //zoomOutArea->setCheckable(true);
    connect(zoomOutArea,SIGNAL(triggered(bool)),mypaintingboard,SLOT(ZoomOutArea()));
    //裁剪
    cutLine = new QAction(QIcon(":/icons/cutLine.ico"),tr("裁剪"),this);
    cutLine->setToolTip(tr("裁剪线段"));
    connect(cutLine,SIGNAL(triggered(bool)),mypaintingboard,SLOT(CutLineCohenSutherland()));
    cutPolygon = new QAction(QIcon(":/icons/cutPolygon.ico"),tr("裁剪"),this);
    cutPolygon->setToolTip(tr("裁剪多边形"));
    connect(cutPolygon,SIGNAL(triggered(bool)),mypaintingboard,SLOT(CutPolygonSutherlandHodgman()));
    cutArea = new QAction(QIcon(":/icons/cutArea.ico"),tr("裁剪"),this);
    cutArea->setToolTip(tr("裁剪区域"));
    connect(cutArea,SIGNAL(triggered(bool)),mypaintingboard,SLOT(CutArea()));
    //删除
    removeArea = new QAction(QIcon(":/icons/remove.ico"),tr("删除"),this);
    removeArea->setToolTip(tr("删除"));
    connect(removeArea,SIGNAL(triggered(bool)),mypaintingboard,SLOT(RemoveSelectedArea()));

    //打开一个3D模型
    open3DModel = new QAction(QIcon(":/icons/open3Dmodel.ico"),tr("打开3D模型"),this);
    open3DModel->setStatusTip(tr("打开一个3D模型文件(off格式)"));
    open3DModel->setToolTip(tr("打开3D模型"));
    connect(open3DModel,SIGNAL(triggered()),this,SLOT(open3D()));

    //加入工具栏
    QActionGroup* shapegroup=new QActionGroup(this);
    shapegroup->addAction(drawpen);
    shapegroup->addAction(drawline);
    shapegroup->addAction(drawrect);
    shapegroup->addAction(drawcircle);
    shapegroup->addAction(drawellipse);
    shapegroup->addAction(drawbrush);
    shapegroup->addAction(selectArea);
    shapegroup->addAction(moveArea);
    shapegroup->addAction(rotateClockWise);
    shapegroup->addAction(rotateCounterClockWise);
    shapegroup->addAction(zoomInArea);
    shapegroup->addAction(zoomOutArea);
    shapegroup->addAction(cutLine);
    shapegroup->addAction(cutPolygon);
    shapegroup->addAction(cutArea);
    shapegroup->addAction(removeArea);
    ui->mainToolBar->addAction(drawpen);
    ui->mainToolBar->addAction(drawline);
    ui->mainToolBar->addAction(drawrect);
    ui->mainToolBar->addAction(drawcircle);
    ui->mainToolBar->addAction(drawellipse);
    ui->mainToolBar->addAction(drawbrush);
    ui->mainToolBar->addSeparator();

    ui->mainToolBar->addAction(selectArea);
    ui->mainToolBar->addAction(moveArea);
    ui->mainToolBar->addSeparator();

    ui->mainToolBar->addAction(rotateClockWise);
    ui->mainToolBar->addAction(rotateCounterClockWise);
    ui->mainToolBar->addAction(zoomInArea);
    ui->mainToolBar->addAction(zoomOutArea);
    ui->mainToolBar->addAction(cutLine);
    ui->mainToolBar->addAction(cutPolygon);
    ui->mainToolBar->addAction(cutArea);
    ui->mainToolBar->addAction(removeArea);
    ui->mainToolBar->addSeparator();


    //添加并设置颜色动作
    colorBlack=new QAction(QIcon(":/icons/black.ico"),tr("黑色"),this);
    colorBlack->setToolTip(tr("黑色"));
    colorBlack->setCheckable(true);
    colorBlack->setChecked(true);
    connect(colorBlack,SIGNAL(triggered()),mypaintingboard,SLOT(setcolorBlack()));

    colorWhite= new QAction(QIcon(":/icons/white.ico"),tr("白色"),this);
    colorWhite->setToolTip(tr("白色"));
    colorWhite->setCheckable(true);
    connect(colorWhite,SIGNAL(triggered()),mypaintingboard,SLOT(setcolorWhite()));

    colorRed=new QAction(QIcon(":/icons/red.ico"),tr("红色"),this);
    colorRed->setToolTip(tr("红色"));
    colorRed->setCheckable(true);
    connect(colorRed,SIGNAL(triggered()),mypaintingboard,SLOT(setcolorRed()));

    colorOrange=new QAction(QIcon(":/icons/orange.ico"),tr("橙色"),this);
    colorOrange->setToolTip(tr("橙色"));
    colorOrange->setCheckable(true);
    connect(colorOrange,SIGNAL(triggered()),mypaintingboard,SLOT(setcolorOrange()));

    colorYellow=new QAction(QIcon(":/icons/yellow.ico"),tr("黄色"),this);
    colorYellow->setToolTip(tr("黄色"));
    colorYellow->setCheckable(true);
    connect(colorYellow,SIGNAL(triggered()),mypaintingboard,SLOT(setcolorYellow()));

    colorGreen=new QAction(QIcon(":/icons/green.ico"),tr("绿色"),this);
    colorGreen->setToolTip(tr("绿色"));
    colorGreen->setCheckable(true);
    connect(colorGreen,SIGNAL(triggered()),mypaintingboard,SLOT(setcolorGreen()));

    colorCyan=new QAction(QIcon(":/icons/cyan.ico"),tr("青色"),this);
    colorCyan->setToolTip(tr("青色"));
    colorCyan->setCheckable(true);
    connect(colorCyan,SIGNAL(triggered()),mypaintingboard,SLOT(setcolorCyan()));

    colorBlue=new QAction(QIcon(":/icons/blue.ico"),tr("蓝色"),this);
    colorBlue->setToolTip(tr("蓝色"));
    colorBlue->setCheckable(true);
    connect(colorBlue,SIGNAL(triggered()),mypaintingboard,SLOT(setcolorBlue()));

    colorPurple=new QAction(QIcon(":/icons/purple.ico"),tr("紫色"),this);
    colorPurple->setToolTip(tr("紫色"));
    colorPurple->setCheckable(true);
    connect(colorPurple,SIGNAL(triggered()),mypaintingboard,SLOT(setcolorPurple()));

    QActionGroup* colorgroup=new QActionGroup(this);
    colorgroup->addAction(colorBlack);
    colorgroup->addAction(colorRed);
    colorgroup->addAction(colorOrange);
    colorgroup->addAction(colorYellow);
    colorgroup->addAction(colorGreen);
    colorgroup->addAction(colorCyan);
    colorgroup->addAction(colorBlue);
    colorgroup->addAction(colorPurple);
    colorgroup->addAction(colorWhite);
    ui->mainToolBar->addAction(colorBlack);
    ui->mainToolBar->addAction(colorWhite);
    ui->mainToolBar->addAction(colorRed);
    ui->mainToolBar->addAction(colorOrange);
    ui->mainToolBar->addAction(colorYellow);
    ui->mainToolBar->addAction(colorGreen);
    ui->mainToolBar->addAction(colorCyan);
    ui->mainToolBar->addAction(colorBlue);
    ui->mainToolBar->addAction(colorPurple);
    ui->mainToolBar->addSeparator();

    ui->mainToolBar->addAction(open3DModel);

}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::open(){
    if(maybeSave()){
        //getOpenFileName获取打开文件的文件名,QDir::currentPath()获取当前可执行文件的路径作为初始路径
        QString fileName=QFileDialog::getOpenFileName(this,tr("打开文件"),QDir::currentPath());
        if(!fileName.isEmpty()){
            mypaintingboard->openImage(fileName);
        }
    }
}
void MainWindow::save(){
    //由connect函数从slot逆向回溯到发出信号的action
    QAction* sender_action=qobject_cast<QAction*>(sender());
    //再从发出信号的action中获取其创建时传入的文件格式类型
    QByteArray fileformat=sender_action->data().toByteArray();
    //根据对应的文件格式保存文件
    saveFile(fileformat);
}
//判断画板是否被修改过并提供保存选项
bool MainWindow::maybeSave(){
    //若画板被修改过
    if(mypaintingboard->isModified()){
        QMessageBox::StandardButton ret=QMessageBox::warning(this,tr("我的画板"),
                 tr("是否保存当前更改？"),QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);
        //用户选择"保存"
        if(ret==QMessageBox::Save){
            return saveFile("jpg"); //默认以jpg格式保存
        }
        //用户选择"取消",返回false
        else if(ret==QMessageBox::Cancel){
            return false;
        }
    }
    //画板未被修改过,返回true
    return true;
}
bool MainWindow::saveFile(const QByteArray &fileFormat){
    //默认的文件保存路径
    QString initialPath=QDir::currentPath()+"/未命名."+QString(fileFormat);
    //获取保存文件的路径和文件名。第三个和第四个参数分别为初始路径和文件类型筛选
    //仅显示与目标保存类型相同格式的文件
    QString fileName=QFileDialog::getSaveFileName(this,tr("保存"),initialPath,
                                         tr("Images(*.%1)").arg(QString(fileFormat)));
    if(!fileName.isEmpty()){
        mypaintingboard->saveImage(fileName,fileFormat);
        return true;
    }
    else{
        return false;
    }
}

//显示3D窗口并打开文件
void MainWindow::open3D(){

    glWidgets[countGLWidgets]->show();
    glWidgets[countGLWidgets]->openFile();
    countGLWidgets++;
    //myglWidget->show();
    //myglWidget->openFile();

}

//实现拖入事件
void MainWindow::dragEnterEvent(QDragEnterEvent *event){
    //获取拖入的文件名
    QString filename = event->mimeData()->urls()[0].toLocalFile();
    //拖放支持jpg,png,bmp,jpeg常见的图片格式(大小写不敏感)
    if(!filename.right(3).compare("jpg",Qt::CaseInsensitive)
            ||filename.right(3).compare("png",Qt::CaseInsensitive)
            ||filename.right(3).compare("bmp",Qt::CaseInsensitive)
            ||filename.right(4).compare("jpeg",Qt::CaseInsensitive))
        event->acceptProposedAction();

    //否则忽略事件
    else
        event->ignore();

}
//窗口部件放下一个对象时,调用该函数
void MainWindow::dropEvent(QDropEvent *event){

    //获取event->mimeData中的url
    QList <QUrl> urls = event->mimeData()->urls();
    if(urls.isEmpty())
        return;
    else{
        //toLocalFile()将一个QUrl转换为本地文件路径的格式,返回一个QString
        QString fileName = urls.first().toLocalFile();
        mypaintingboard->openImage(fileName);
    }
}

