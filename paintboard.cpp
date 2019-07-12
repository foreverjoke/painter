#include "paintboard.h"
#include <QRectF>
#include <QMouseEvent>
#include <QPen>
#include <QScreen>
#include <QDesktopWidget>
#include <QGuiApplication>
#include <QDebug>
#include <QRgb>
#include <cmath>
#include <iostream>
#include <stack>
#define PI 3.1415926535
int direction[4][2]={{0,-1},{1,0},{0,1},{-1,0}};    //全局变量,记录填充算法时点位置的变化方向
bool insideArea(QPoint tl,QPoint tr,QPoint bl,QPoint br,int xp,int yp);
PaintBoard::PaintBoard(QWidget *parent) : QWidget(parent)
{

    setAttribute(Qt::WA_StaticContents);//避免当窗口大小改变时对内容进行重新绘制
    modified=false;     //初始化为未修改
    painting=false;     //初始化为当前未进行作图
    status=drawPen;     //初始化状态为自由画线
    radius=0;
    moveFlag = false;   //初始状态下禁止移动
    cutFlag = false;    //初始状态下禁止裁剪
    zoomInFlag = false;
    zoomOutFlag = false;
    rotateFlag = false;
}

bool PaintBoard::openImage(const QString &fileName){
    QImage loadedImage;
    //根据传入的文件名,调用QImage类的load函数打开一张图片
    if(!loadedImage.load(fileName))
        return false;
    //将主窗口"重刷",并加上打开的图片
    QSize newSize = loadedImage.size().expandedTo(size());
    resizeImage(&loadedImage, newSize);
    image=loadedImage;
    modified=false; //打开图片后视为未修改
    update();
    return true;
}

bool PaintBoard::saveImage(const QString &fileName, const char *fileFormat){
    QImage visibleImage=image;
    //直接调用QImage类的save函数,按照对应的格式保存图片
    if(visibleImage.save(fileName,fileFormat)){
        modified=false;
        return true;
    }
    else{
        return false;
    }
}

void PaintBoard::setstatuspen(){
    status=drawPen;
}
void PaintBoard::setstatusline(){
    status=drawLine;
}
void PaintBoard::setstatusrect(){
    status=drawRect;
}
void PaintBoard::setstatuscircle(){
    status=drawCircle;
}
void PaintBoard::setstatusellipse(){
    status=drawEllipse;
}
void PaintBoard::setstatusbrush(){
    status=drawBrush;
}
void PaintBoard::setstatusSelect(){
    status=select;
    //setCursor(Qt::PointingHandCursor);
}
void PaintBoard::setstatusMove(){
    status=moving;
    //setCursor(Qt::SizeAllCursor);
}

//逆向求算放大前的位置
//参数分别为放大后的位置,放大中心的位置和放大的倍数(>1)
QPointF PaintBoard::zoomInReverseLoc(QPoint dst, QPoint center,double ratio){
    unsigned deltax = abs(center.x()-dst.x());
    unsigned deltay = abs(center.y()-dst.y());
    //位置判断
    int flagx = dst.x()>center.x()?1:-1;
    int flagy = dst.y()>center.y()?1:-1;
    //坐标放缩逆运算
    qreal xsrc = center.x()+deltax/ratio*flagx;
    qreal ysrc = center.y()+deltay/ratio*flagy;

    return QPointF(xsrc,ysrc);
}
//双线性插值
QRgb PaintBoard::BilinearInterpolation(qreal x, qreal y){

    int xleft=(int)floor(x),xright=(int)ceil(x);
    int ytop=(int)ceil(y),ybottom=(int)floor(y);
    QRgb rgb_lt = image.pixel(xleft,ytop);
    QRgb rgb_lb = image.pixel(xleft,ybottom);
    QRgb rgb_rt = image.pixel(xright,ytop);
    QRgb rgb_rb = image.pixel(xright,ybottom);

    QRgb get;
    //x,y都是整数
    if(abs(x-(int)x)<=1e-5 && abs(y-(int)y)<=1e-5){
        get = image.pixel((int)x,(int)y);
    }
    //只有x是整数,取上下两个点进行插值
    else if(abs(x-(int)x)<=1e-5){
        get = (y-ytop)/(ybottom-ytop)*rgb_lt+(ybottom-y)/(ybottom-ytop)*rgb_lb;
    }
    //只有y是整数,取左右两个点进行插值
    else if(abs(y-(int)y)<=1e-5){
        get = (x-xleft)/(xright-xleft)*rgb_lt+(xright-x)/(xright-xleft)*rgb_rt;
    }
    //x和y都不是整数
    else{
        QRgb rgb_top = (x-xleft)/(xright-xleft)*rgb_rt + (xright-x)/(xright-xleft)*rgb_lt;
        QRgb rgb_bottom = (x-xleft)/(xright-xleft)*rgb_rb + (xright-x)/(xright-xleft)*rgb_lb;
        get = (y-ytop)/(ybottom-ytop)*rgb_bottom + (ybottom-y)/(ybottom-ytop)*rgb_top;
    }

    return get;
}

void PaintBoard::ZoomInArea(){

    QImage buffer = image;
    QPainter pBuffer(&buffer);
    //将原有区域擦除
    QPen whitePen;
    QBrush whiteBrush;
    whiteBrush.setStyle(Qt::SolidPattern);
    whiteBrush.setColor(QColor(255,255,255));
    whitePen.setColor(QColor(255,255,255));
    pBuffer.setBrush(whiteBrush);
    pBuffer.setPen(whitePen);
    pBuffer.drawRect(QRect(TopLeft,BottomRight));

    double ratio = 1.2;     //放大倍率
    int xcenter = (TopLeft.x() + BottomRight.x())/2;
    int ycenter = (TopLeft.y() + BottomRight.y())/2;

    for(int i=0;i<buffer.width();i++){
        for(int j=0;j<buffer.height();j++){
            //获取放大之前的源位置
            QPointF srcPoint = zoomInReverseLoc(QPoint(i,j),QPoint(xcenter,ycenter),ratio);
            qreal xsrc = srcPoint.x(),ysrc = srcPoint.y();
            QRgb getRGB;
            //源位置在选择的区域内
            if(insideSelectedArea((int)xsrc,(int)ysrc)){
                getRGB = BilinearInterpolation(xsrc,ysrc);
            }
            //源位置不在选择的区域内,则直接将同位置的像素点赋值过来
            else{
                getRGB = image.pixel(i,j);
            }

            buffer.setPixel(i,j,getRGB);
        }
    }
    //修改选中区域的位置
    TopLeft = zoomInReverseLoc(TopLeft,QPoint(xcenter,ycenter),1/ratio).toPoint();
    TopRight = zoomInReverseLoc(TopRight,QPoint(xcenter,ycenter),1/ratio).toPoint();
    BottomLeft = zoomInReverseLoc(BottomLeft,QPoint(xcenter,ycenter),1/ratio).toPoint();
    BottomRight = zoomInReverseLoc(BottomRight,QPoint(xcenter,ycenter),1/ratio).toPoint();

    //修改画面
    image = buffer;
    update();
    moveFlag = false;   //需要重新选中才能移动

    //调用库函数的放大
    /*if(!zoomInFlag){
        //zoomBuffer保存最原始的image
        zoomBuffer = image;
        zoomInFlag = true;
    }

    double ratio = 1.1;
    int width = selectedArea.width()*ratio,height=selectedArea.height()*ratio;
    int centerX=(TopLeft.x()+BottomRight.x())/2;
    int centerY=(TopLeft.y()+BottomRight.y())/2;
    int x = centerX-width/2,y=centerY-height/2;
    QImage getImage = selectedArea.scaled(width,height);
    QPainter p(&image);
    p.drawImage(QPoint(x,y),getImage);

    //抹去虚线
    QPen whitePen;
    whitePen.setColor(QColor(255,255,255));
    whitePen.setStyle(Qt::SolidLine);
    p.setPen(whitePen);
    p.drawRect(x,y,width,height);

    update();
    //更新标记值
    selectedArea = getImage;
    TopLeft.setX(x);TopLeft.setY(y);
    BottomRight.setX(x+width);
    BottomRight.setY(y+height);*/
}
void PaintBoard::ZoomOutArea(){

    QImage buffer = image;
    QPainter pBuffer(&buffer);
    //将原有区域擦除
    QPen whitePen;
    QBrush whiteBrush;
    whiteBrush.setStyle(Qt::SolidPattern);
    whiteBrush.setColor(QColor(255,255,255));
    whitePen.setColor(QColor(255,255,255));
    pBuffer.setBrush(whiteBrush);
    pBuffer.setPen(whitePen);
    pBuffer.drawRect(QRect(TopLeft,BottomRight));

    double ratio = 0.8;     //放大倍率
    int xcenter = (TopLeft.x() + BottomRight.x())/2;
    int ycenter = (TopLeft.y() + BottomRight.y())/2;

    for(int i=0;i<buffer.width();i++){
        for(int j=0;j<buffer.height();j++){
            //获取放大之前的源位置
            QPointF srcPoint = zoomInReverseLoc(QPoint(i,j),QPoint(xcenter,ycenter),ratio);
            qreal xsrc = srcPoint.x(),ysrc = srcPoint.y();

            //源位置在选择的区域内,此时现位置一定在缩小后的区域内,进行双线性插值
            if(insideSelectedArea((int)xsrc,(int)ysrc)){
                buffer.setPixel(i,j,BilinearInterpolation(xsrc,ysrc));
            }
            //源位置不在选择的区域内
            else{
                //但现位置在选择的区域内,则直接赋白(擦除)
                if(insideSelectedArea(i,j))
                    buffer.setPixelColor(i,j,QColor(255,255,255));
                //现位置也不在选择的区域内,则拷贝原有像素
                else{
                    buffer.setPixel(i,j,image.pixel(i,j));
                }
            }
        }
    }

    //修改选中区域的位置
    TopLeft = zoomInReverseLoc(TopLeft,QPoint(xcenter,ycenter),1/ratio).toPoint();
    TopRight = zoomInReverseLoc(TopRight,QPoint(xcenter,ycenter),1/ratio).toPoint();
    BottomLeft = zoomInReverseLoc(BottomLeft,QPoint(xcenter,ycenter),1/ratio).toPoint();
    BottomRight = zoomInReverseLoc(BottomRight,QPoint(xcenter,ycenter),1/ratio).toPoint();

    image = buffer;
    update();

    moveFlag = false;
    //调用库函数的缩小
    /*if(!zoomOutFlag){
        //zoomBuffer保存最原始的image
        zoomBuffer = image;
        zoomOutFlag = true;
    }
    QPainter p(&image);
    p.fillRect(QRect(TopLeft,BottomRight),QColor(255,255,255));

    double ratio = 1.1;
    int width = selectedArea.width()/ratio,height=selectedArea.height()/ratio;
    int centerX=(TopLeft.x()+BottomRight.x())/2;
    int centerY=(TopLeft.y()+BottomRight.y())/2;
    int x = centerX-width/2,y=centerY-height/2;
    QImage getImage = selectedArea.scaled(width,height);
    p.drawImage(QPoint(x,y),getImage);

    //抹去虚线
    QPen whitePen;
    whitePen.setColor(QColor(255,255,255));
    whitePen.setStyle(Qt::SolidLine);
    p.setPen(whitePen);
    p.drawRect(x,y,width,height);

    update();
    //更新标记值
    selectedArea = getImage;
    TopLeft.setX(x);TopLeft.setY(y);
    BottomRight.setX(x+width);
    BottomRight.setY(y+height);*/
}
//旋转点坐标计算(分象限讨论)
vector<QPoint> rotateDegree(int x,int y,int xCenter,int yCenter,double degree){
    double d = degree*(PI/180); //角度制转弧度制
    double xnew,ynew;
    if(x<=xCenter && y<=yCenter){
        xnew = xCenter+(yCenter-y)*sin(d)-(xCenter-x)*cos(d);
        ynew = yCenter-(xCenter-x)*sin(d)-(yCenter-y)*cos(d);
    }
    else if(x>xCenter && y<=yCenter){
        xnew = xCenter+(x-xCenter)*cos(d)+(yCenter-y)*sin(d);
        ynew = yCenter+(x-xCenter)*sin(d)-(yCenter-y)*cos(d);
    }
    else if(x>xCenter && y>yCenter){
        xnew = xCenter+(x-xCenter)*cos(d)-(y-yCenter)*sin(d);
        ynew = yCenter+(x-xCenter)*sin(d)+(y-yCenter)*cos(d);
    }
    else if(x<=xCenter && y>yCenter){
        xnew = xCenter-(xCenter-x)*cos(d)-(y-yCenter)*sin(d);
        ynew = yCenter-(xCenter-x)*sin(d)+(y-yCenter)*cos(d);
    }

    vector<QPoint> points;
    QPoint p1((int)floor(xnew),(int)floor(ynew));
    QPoint p2((int)floor(xnew),(int)ceil(ynew));
    QPoint p3((int)ceil(xnew),(int)floor(ynew));
    QPoint p4((int)ceil(xnew),(int)ceil(ynew));
    points.push_back(p1);
    points.push_back(p2);
    points.push_back(p3);
    points.push_back(p4);

    return points;
}
//顺时针和逆时针旋转
void PaintBoard::RotateClockWise(){
    if(!rotateFlag){
        rotateBuffer = image;
        rotateFlag = true;
    }
    QImage tmp = image;
    //将原有区域擦除
    for(int i=0;i<tmp.width();i++){
        for(int j=0;j<tmp.height();j++){
            if(insideSelectedArea(i,j))
                tmp.setPixelColor(i,j,QColor(255,255,255));
        }
    }

    //旋转角度(角度制)
    int angle = 30;
    int xStart = TopLeft.x(),xEnd = BottomRight.x();
    int yStart = TopLeft.y(),yEnd = BottomRight.y();
    int xCenter = (xStart + xEnd) / 2;
    int yCenter = (yStart + yEnd) / 2;

    for(int i=0;i<tmp.width();i++){
        for(int j=0;j<tmp.height();j++){
            if(insideSelectedArea(i,j)){
                QRgb getRGB= image.pixel(i,j);
                vector<QPoint> getPoints = rotateDegree(i,j,xCenter,yCenter,angle);
                for(unsigned i=0;i<getPoints.size();i++){
                    QPoint getPoint = getPoints[i];
                    if(getPoint.x()>=0 && getPoint.x()<=tmp.width()-1 &&
                            getPoint.y()>=0 && getPoint.y()<=tmp.height()-1){
                        tmp.setPixel(getPoint,getRGB);
                    }
                }
            }
        }
    }

    //修改边界范围
    TopLeft = rotateDegree(TopLeft.x(),TopLeft.y(),xCenter,yCenter,angle).at(0);
    TopRight = rotateDegree(TopRight.x(),TopRight.y(),xCenter,yCenter,angle).at(0);
    BottomLeft = rotateDegree(BottomLeft.x(),BottomLeft.y(),xCenter,yCenter,angle).at(0);
    BottomRight = rotateDegree(BottomRight.x(),BottomRight.y(),xCenter,yCenter,angle).at(0);
    image = tmp;

    update();
}
void PaintBoard::RotateCounterClockWise(){
    if(!rotateFlag){
        rotateBuffer = image;
        rotateFlag = true;
    }
    QImage tmp = image;
    //将原有区域擦除
    for(int i=0;i<tmp.width();i++){
        for(int j=0;j<tmp.height();j++){
            if(insideSelectedArea(i,j))
                tmp.setPixelColor(i,j,QColor(255,255,255));
        }
    }

    //旋转角度(角度制)
    int angle = -30;
    int xStart = TopLeft.x(),xEnd = BottomRight.x();
    int yStart = TopLeft.y(),yEnd = BottomRight.y();
    int xCenter = (xStart + xEnd) / 2;
    int yCenter = (yStart + yEnd) / 2;

    for(int i=0;i<tmp.width();i++){
        for(int j=0;j<tmp.height();j++){
            if(insideSelectedArea(i,j)){
                QRgb getRGB= image.pixel(i,j);
                vector<QPoint> getPoints = rotateDegree(i,j,xCenter,yCenter,angle);
                for(unsigned i=0;i<getPoints.size();i++){
                    QPoint getPoint = getPoints[i];
                    if(getPoint.x()>=0 && getPoint.x()<=tmp.width()-1 &&
                            getPoint.y()>=0 && getPoint.y()<=tmp.height()-1){
                        tmp.setPixel(getPoint,getRGB);
                    }
                }
            }
        }
    }

    //修改边界范围
    TopLeft = rotateDegree(TopLeft.x(),TopLeft.y(),xCenter,yCenter,angle).at(0);
    TopRight = rotateDegree(TopRight.x(),TopRight.y(),xCenter,yCenter,angle).at(0);
    BottomLeft = rotateDegree(BottomLeft.x(),BottomLeft.y(),xCenter,yCenter,angle).at(0);
    BottomRight = rotateDegree(BottomRight.x(),BottomRight.y(),xCenter,yCenter,angle).at(0);
    image = tmp;

    update();
}

void PaintBoard::setcolorBlack(){
    pencolor=QColor(0,0,0); //黑色的rgb值
}
void PaintBoard::setcolorWhite(){
    pencolor=QColor(255,255,255);   //白色的RGB值
}
void PaintBoard::setcolorRed(){
    pencolor=QColor(255,0,0);   //红色的rgb值
}
void PaintBoard::setcolorOrange(){
    pencolor=QColor(255,165,0); //橙色的rgb值
}
void PaintBoard::setcolorYellow(){
    pencolor=QColor(255,255,0);
}
void PaintBoard::setcolorGreen(){
    pencolor=QColor(0,255,0);
}
void PaintBoard::setcolorCyan(){
    pencolor=QColor(0,255,255);
}
void PaintBoard::setcolorBlue(){
    pencolor=QColor(0,0,255);
}
void PaintBoard::setcolorPurple(){
    pencolor=QColor(138,43,226);
}

void PaintBoard::mousePressEvent(QMouseEvent *event){
    //用于测试
    /*if(status == stop){
        if(insideSelectedArea(event->pos().x(),event->pos().y()))
            cout<<"inside"<<endl;
        else
            cout<<"outside"<<endl;
    }*/

    if(painting)    //正在绘画时按下鼠标(事实上不存在),不需要作任何反应
        return;
    if(status==drawCircle)  //正在画圆
        circle_center=event->pos(); //按下鼠标设置圆心位置
    if(status==drawBrush){  //正在填充
        //FloodSeedFill(event->pos(),pencolor);   //调用填充算法
        fill_center=event->pos();
    }
    if(status==select){
        selectBuffer = image;       //保存当前状态
    }
    if(status==moving){
        transferBuffer=image;       //保存当前状态
        moveStart=event->pos();     //设置平移起始位置
    }
    /*if(status == zoomIn || status == zoomOut){
        zoomBuffer = image;
        zoomCenter = event->pos();  //设置缩放中心点
    }*/
    lastpoint=event->pos();
    p1.setX(event->pos().x());
    p1.setY(event->pos().y());
    painting=true;

}
void PaintBoard::mouseMoveEvent(QMouseEvent *event){
    if(!painting)   //不在绘画时移动鼠标,不需要作任何反应
        return;
    p2.setX(event->pos().x());
    p2.setY(event->pos().y());


    //自由画线功能在这里实现:移动鼠标过程中不断更新线条
    //if(status=drawPen)        //低级错误!!!
    if(status==drawPen)
        drawLineTo(event->pos());
    if(status==drawCircle){
        int dx=circle_center.x()-event->pos().x();
        int dy=circle_center.y()-event->pos().y();
        radius=(int)sqrt((double)(dx*dx+dy*dy));
    }
    if(status==select){
        setPoints();    //设置选中区域的四个坐标位置
    }
    if(status==moving){
        moveEnd = p2;
    }
    update();
}
void PaintBoard::mouseReleaseEvent(QMouseEvent *event){
    p2.setX(event->pos().x());
    p2.setY(event->pos().y());
    if(!painting)   //不在绘画时松开鼠标,不需要作任何反应
        return;
    if(!p2.isNull()){
        drawLineTo(event->pos());
        painting=false; //结束绘画状态
    }
    //将线段位置放入记录中
    if(status == drawLine){
        lines.push_back(Line(lineStart,lineEnd));
        //测试代码
        /*for(unsigned i=0;i<lines.size();i++){
            Line tmp = lines[i];
            QPoint a=tmp.start,b=tmp.end;
            cout<<"[("<<a.x()<<","<<a.y()<<")("<<b.x()<<","<<b.y()<<")]  ";
        }
        cout<<endl;*/
    }
    if(status == drawRect){
        //测试代码
        /*setPoints();
        QPainter pai(&image);
        drawRect_DDA(&pai,TopLeft,BottomRight);*/

        polygons.push_back(tmpPoly);
        //测试代码
        cout<<"Polygons:"<<endl;
        for(unsigned i = 0;i<polygons.size();i++){
            Polygon tmp = polygons[i];
            for(unsigned j = 0;j<tmp.points.size();j++){
                QPoint p = tmp.points[j];
                cout<<"("<<p.x()<<","<<p.y()<<")";
            }
            cout<<endl;
        }
    }
    if(status == drawEllipse){
        setPoints();
        QPainter p(&image);
        drawEllipse_MID(&p,QRect(TopLeft,BottomRight));
    }
    //选择状态
    if(status == select){
        getSelectedArea();
        //status = stop;
    }
    //移动状态
    if(status == moving){
        moveEnd = p2;
        status = stop;
        //drawTransferImage(moveEnd);
        moveFlag = false;   //移动一次后关闭移动
        //计算坐标变化
        int deltax = moveEnd.x()-moveStart.x();
        int deltay = moveEnd.y()-moveStart.y();
        //改变选中区域以使平移和旋转、缩放等操作具有连贯性
        TopLeft = QPoint(TopLeft.x()+deltax,TopLeft.y()+deltay);
        TopRight = QPoint(TopRight.x()+deltax,TopRight.y()+deltay);
        BottomLeft = QPoint(BottomLeft.x()+deltax,BottomLeft.y()+deltay);
        BottomRight = QPoint(BottomRight.x()+deltax,BottomRight.y()+deltay);
    }

    /*if(status == zoomIn){
        //鼠标释放位置和按下位置相同时触发动作
        if(event->pos() == zoomCenter){
            ZoomInArea();
        }
    }
    if(status == zoomOut){
        if(event->pos() == zoomCenter){
            ZoomOutArea();
        }
    }*/
    //unsetCursor();  //鼠标形状复原
    update();
}
//piantEvent函数在update()时被调用
void PaintBoard::paintEvent(QPaintEvent *){
    QPainter tmp_painter(this);
    //drawImage的用法参见documentation
    tmp_painter.drawImage(QPoint(0,0),image);
    if(painting&&(status))
        paint(&tmp_painter);
}
//调整画布大小
void PaintBoard::resizeEvent(QResizeEvent *)
{

    if (width() > image.width() || height() > image.height()) {
        int newWidth = qMax(width() + 100, image.width());
        int newHeight = qMax(height() + 100, image.height());
        resizeImage(&image, QSize(newWidth, newHeight));
        update();
    }

}

void PaintBoard::paint(QPainter* painter){

    painter->setRenderHint(QPainter::Antialiasing,true);
    painter->setPen(QPen(pencolor));
    switch(status){
    case drawPen:
        break;
    case drawLine:{
        //drawLine_DDA(painter,p1,p2);
        drawLine_MID(painter,p1,p2);
        /*for(unsigned i=0;i<lines.size();i++){
            Line get = lines[i];
            drawLine_MID(painter,get.start,get.end);
        }*/
        break;
    }

    case drawRect:
        drawRect_DDA(painter,p1,p2);
        break;
    case drawCircle:
        drawCircle_MID(painter,circle_center,radius);
        break;
    case drawEllipse:{
        //cout<<"p1("<<p1.x()<<","<<p1.y()<<")  ";
        //cout<<"p2("<<p2.x()<<","<<p2.y()<<")"<<endl;
        setPoints();
        drawEllipse_MID(painter,QRect (TopLeft,BottomRight));
        break;
    }
    case drawBrush:{
        QColor current_color=image.pixelColor(fill_center);  //获取当前位置原有的颜色
        //cout<<"fill_center:("<<fill_center.x()<<","<<fill_center.y()<<")"<<endl;
        //FloodSeedFill(painter,fill_center,current_color);
        ScanSeedFill(painter,fill_center,current_color);
        break;
    }
    case select:{
        //setCursor(Qt::PointingHandCursor);
        drawSelectedArea();
        break;
    }
    case moving:{
        //setCursor(Qt::SizeAllCursor);
        if(moveFlag)
            drawTransferImage(moveEnd);
        break;
    }
    //状态复原后鼠标形状还原
    case stop:{
        //setCursor(Qt::ArrowCursor);
        break;
    }
    default:break;
    }
    update();
}
int min(int x,int y){
    return x<y?x:y;
}
int max(int x,int y){
    return x>y?x:y;
}

void PaintBoard::setPoints(){
    if(p1.x()<p2.x() && p1.y()<p2.y()){ //右下角方向
        TopLeft = p1;
        BottomRight = p2;
        TopRight = QPoint(p2.x(),p1.y());
        BottomLeft = QPoint(p1.x(),p2.y());
    }
    else if(p1.x()<p2.x() && p1.y() > p2.y()){  //右上角方向
        BottomLeft = p1;
        TopRight = p2;
        BottomRight = QPoint(p2.x(),p1.y());
        TopLeft = QPoint(p1.x(),p2.y());
    }
    else if(p1.x()>p2.x() && p1.y() > p2.y()){  //左上角方向
        TopLeft = p2;
        BottomRight = p1;
        TopRight = QPoint(p1.x(),p2.y());
        BottomLeft = QPoint(p2.x(),p1.y());
    }
    else if(p1.x()>p2.x() && p1.y()<p2.y()){    //左下角方向
        TopRight = p1;
        BottomLeft = p2;
        TopLeft = QPoint(p2.x(),p1.y());
        BottomRight = QPoint(p1.x(),p2.y());
    }
}

//判断点是否在选中的区域内
bool PaintBoard::insideSelectedArea(int xp, int yp){
    int xa = TopLeft.x(),ya = TopLeft.y();
    int xb = TopRight.x(),yb = TopRight.y();
    int xc = BottomRight.x(),yc = BottomRight.y();
    int xd = BottomLeft.x(),yd = BottomLeft.y();

    int ABAP = (xb-xa)*(xp-xa)+(yb-ya)*(yp-ya);
    int CDCP = (xd-xc)*(xp-xc)+(yd-yc)*(yp-yc);
    int ADAP = (xd-xa)*(xp-xa)+(yd-ya)*(yp-ya);
    int CBCP = (xb-xc)*(xp-xc)+(yb-yc)*(yp-yc);

    if(ABAP>=0 && CDCP>=0 && ADAP>=0 && CBCP>=0)
        return true;
    else
        return false;
}
//判断某点是否在指定的四个点围成的矩形区域内
bool insideArea(QPoint tl,QPoint tr,QPoint br,QPoint bl,int xp,int yp){
    int xa = tl.x(),ya = tl.y();
    int xb = tr.x(),yb = tr.y();
    int xc = br.x(),yc = br.y();
    int xd = bl.x(),yd = bl.y();

    int ABAP = (xb-xa)*(xp-xa)+(yb-ya)*(yp-ya);
    int CDCP = (xd-xc)*(xp-xc)+(yd-yc)*(yp-yc);
    int ADAP = (xd-xa)*(xp-xa)+(yd-ya)*(yp-ya);
    int CBCP = (xb-xc)*(xp-xc)+(yb-yc)*(yp-yc);

    if(ABAP>=0 && CDCP>=0 && ADAP>=0 && CBCP>=0)
        return true;
    else
        return false;
}

void PaintBoard::drawSelectedArea(){
    //用黑色虚线画出当前选中区域
    QPainter p(this);
    QPen pen;
    pen.setColor(QColor(0,0,0));
    pen.setStyle(Qt::DashLine);
    p.setPen(pen);

    p.drawRect(QRect(TopLeft,BottomRight));
}
void PaintBoard::getSelectedArea(){
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;
    //截取选中区域
    QRect rectArea(TopLeft,BottomRight);
    int x =TopLeft.x(),y=TopLeft.y();
    int width = rectArea.width(),height=rectArea.height();
    selectedArea = screen->grabWindow(this->winId(),x,y,width,height).toImage();

    moveFlag = true;    //选中后允许移动
    cutFlag = true;     //选中后允许裁剪

    //测试
    //QPainter tmp_painter(&image);   //在image上新建一个QPainter
    //tmp_painter.drawImage(QPoint(0,0),selectedArea);
    //update();
}

void PaintBoard::drawTransferImage(QPoint setPoint){
    if(!moveFlag){
        QMessageBox::critical(this,"错误","请先选中区域再进行平移!","确定");
        return;
    }
    //以下方法开销太大,亲测
    //在平移缓冲上将选中区域擦除
    /*for(int i=0;i<transferBuffer.width();i++){
        for(int j=0;j<transferBuffer.height();j++){
            if(insideSelectedArea(i,j))
                transferBuffer.setPixelColor(i,j,QColor(255,255,255));
        }
    }
    //计算坐标变化
    int deltax = setPoint.x()-moveStart.x();
    int deltay = setPoint.y()-moveStart.y();
    //将原有像素点平移到新的位置
    for(int i=0;i<image.width();i++){
        for(int j=0;j<image.height();j++){
            if(insideSelectedArea(i,j)){
                QRgb getRGB = image.pixel(i,j);
                QPoint getPoint = QPoint(i+deltax,j+deltay);
                if(getPoint.x()>=0 && getPoint.x()<transferBuffer.width()
                        && getPoint.y()>=0 && getPoint.y()<transferBuffer.height())
                    transferBuffer.setPixel(getPoint,getRGB);
            }
        }
    }*/

    //更新画面
    //image = transferBuffer;
    //update();

    QPainter pBuffer(&transferBuffer);
    QBrush whiteBrush;
    whiteBrush.setStyle(Qt::SolidPattern);
    whiteBrush.setColor(QColor(255,255,255));
    QPen whitePen;
    whitePen.setColor(QColor(255,255,255));
    pBuffer.setBrush(whiteBrush);
    pBuffer.setPen(whitePen);
    pBuffer.drawRect(QRect(TopLeft,BottomRight));

    //计算坐标变化
    int deltax = setPoint.x()-moveStart.x();
    int deltay = setPoint.y()-moveStart.y();
    //新的位置
    QPoint newTopLeft(TopLeft.x()+deltax,TopLeft.y()+deltay);
    //将移动结果画到图上
    QPainter globalPainter(&image);
    globalPainter.drawImage(QPoint(0,0),transferBuffer);
    globalPainter.drawImage(newTopLeft,selectedArea);
    //将虚线框抹去
    globalPainter.setPen(whitePen);
    globalPainter.drawRect(newTopLeft.x(),newTopLeft.y(),selectedArea.width(),selectedArea.height());

    //因为本函数将被调用很多次,在这里改变选中区域的坐标会导致目标位置的错乱
    //改变选中区域
    /*TopLeft = QPoint(TopLeft.x()+deltax,TopLeft.y()+deltay);
    TopRight = QPoint(TopRight.x()+deltax,TopRight.y()+deltay);
    BottomLeft = QPoint(BottomLeft.x()+deltax,BottomLeft.y()+deltay);
    BottomRight = QPoint(BottomRight.x()+deltax,BottomRight.y()+deltay);*/
    update();
}

//根据点的位置进行编码
int encodePoint(QPoint point,QPoint topleft,QPoint bottomright){
    int x = point.x(),y = point.y();
    int leftx = topleft.x(),rightx = bottomright.x();
    int topy = topleft.y(),bottomy = bottomright.y();

    int code = 0;
    if(x<leftx){
        code += 1;
    }
    else if(x>rightx){
        code += (1<<1);
    }
    if(y<topy){
        code += (1<<3);
    }
    else if(y>bottomy){
        code += (1<<2);
    }
    return code;
}
//删除vector中指定下标的元素
template <typename T>
void removeElement(vector<T>& v,int index){
    auto it = v.begin();
    for(int i=0;i<index;i++)
        ++it;
    v.erase(it);
}
//均值填充法"抹去"一个像素
QColor PaintBoard::getEraseColor(QPoint pos){
    QColor posColor = image.pixelColor(pos);
    QColor leftColor = image.pixelColor(QPoint(pos.x()-1,pos.y()));
    QColor rightColor = image.pixelColor(QPoint(pos.x()+1,pos.y()));
    QColor topColor = image.pixelColor(QPoint(pos.x(),pos.y()-1));
    QColor bottomColor = image.pixelColor(QPoint(pos.x(),pos.y()+1));

    QRgb rgb_pos = posColor.rgb();
    QRgb rgb_left = leftColor.rgb();
    QRgb rgb_right = rightColor.rgb();
    QRgb rgb_top = topColor.rgb();
    QRgb rgb_bottom = bottomColor.rgb();

    QRgb rgbs[4]={rgb_left,rgb_right,rgb_top,rgb_bottom};
    int count = 0;
    long long sum = 0;
    for(int i=0;i<4;i++){
        if(rgbs[i]!=rgb_pos){
            sum+=rgbs[i];
            count++;
        }
    }
    if(count == 0){
        cout<<"("<<pos.x()<<","<<pos.y()<<")无法清除"<<endl;
        return posColor;
    }
    double get = (double)sum/(double)count;
    QRgb average = (unsigned int)get;
    return QColor(average);
}

//Cohen-Sutherland算法裁剪线段
void PaintBoard::CutLineCohenSutherland(){

    //用背景色填充选择区域
    QColor bgColor = QColor(255,255,255);
    QPen bgPen;
    QBrush bgBrush;
    bgPen.setColor(bgColor);
    bgBrush.setColor(bgColor);
    QPainter pRenewImage(&image);
    pRenewImage.setPen(bgPen);
    pRenewImage.setBrush(QColor(255,255,255));
    //pBuffer.drawRect(QRect(TopLeft,BottomRight));
    //各个边界的编码
    int leftBound = 1,rightBound = (1<<1);
    int bottomBound = (1<<2),topBound = (1<<3);

    unsigned i = 0;
    while(i<lines.size()){
        Line get = lines[i];
        QPoint start = get.start,end = get.end; //起点和终点
        //QColor lineColor = image.pixelColor(start);//线段原有的颜色
        double k = (double)(start.y()-end.y())/(start.x()-end.x()); //线段的斜率
        //计算端点的编码
        int codeStart = encodePoint(start,TopLeft,BottomRight);
        int codeEnd = encodePoint(end,TopLeft,BottomRight);
        //两个端点在选中区域之外的同侧
        if((codeStart&codeEnd)!=0){
            //擦除线段
            pRenewImage.drawRect(QRect(start,end));
            //则直接删除该条直线,此时i不必后移
            removeElement(lines,i);
            continue;
        }
        QPoint oldpos;
        //当两者中至少有一个还在区域外时,继续裁剪过程
        while(codeStart!=0 || codeEnd!=0){
            //起始点在区域外,则修改起始点的位置
            if(codeStart!=0){
                //左边界外
                int newx,newy;
                if((codeStart&leftBound)!=0){
                    newy = start.y()+k*(TopLeft.x()-start.x());
                    newx = TopLeft.x();
                }
                //右边界外
                else if((codeStart&rightBound)!=0){
                    newy = start.y()+k*(TopRight.x()-start.x());
                    newx = TopRight.x();
                }
                //下边界外
                else if((codeStart&bottomBound)!=0){
                    newx = start.x()+(double)(BottomRight.y()-start.y())/k;
                    newy = BottomRight.y();
                }
                //上边界外
                else if((codeStart&topBound)!=0){
                    newx = start.x()+(double)(TopLeft.y()-start.y())/k;
                    newy = TopLeft.y();
                }
                oldpos = start;
                start = QPoint(newx,newy);
                codeStart = encodePoint(start,TopLeft,BottomRight);//重新计算编码
                pRenewImage.setPen(bgPen);
                pRenewImage.drawRect(QRect(oldpos,start));  //擦除原有痕迹
            }
            //终点在区域外,则修改终点的位置
            if(codeEnd!=0){
                int newx,newy;
                if((codeEnd&leftBound)!=0){
                    newy = end.y()+k*(TopLeft.x()-end.x());
                    newx = TopLeft.x();
                }
                else if((codeEnd&rightBound)!=0){
                    newy = end.y()+k*(TopRight.x()-end.x());
                    newx = TopRight.x();
                }
                else if((codeEnd&bottomBound)!=0){
                    newx = end.x()+(double)(BottomRight.y()-end.y())/k;
                    newy = BottomRight.y();
                }
                else if((codeEnd&topBound)!=0){
                    newx = end.x()+(double)(TopLeft.y()-end.y())/k;
                    newy = TopLeft.y();
                }
                oldpos = end;
                end = QPoint(newx,newy);
                codeEnd = encodePoint(end,TopLeft,BottomRight);
                pRenewImage.setPen(bgPen);
                pRenewImage.drawRect(QRect(oldpos,end));
            }
        }
        lines[i] = Line(start,end); //修改直线位置
        //重新画线
        //QPen newpen;
        //newpen.setColor(lineColor);
        //pRenewImage.setPen(newpen);
        //drawLine_MID(&pRenewImage,start,end);

        i++;                  //循环后移(裁剪下一条直线)
    }

    update();


}

//计算两个向量的叉积
//叉积为正表示结果向量指向右手系的正向,否则表示指向右手系的负向
//由于屏幕中X轴正向朝右,Y轴正向朝下,所以右手系的正向指向屏幕内
int CrossProduct(QPoint v1Start,QPoint v1End,QPoint v2Start,QPoint v2End){
    return (v1End.x()-v1Start.x())*(v2End.y()-v2Start.y())-
            (v1End.y()-v1Start.y())*(v2End.x()-v2Start.x());
}
//浮点数绝对值
double absDouble(double t){
    return t>=0?t:-t;
}

//分别已知两条直线上的两点,求这两条直线的交点
QPoint IntersectPoint(QPoint pointA,QPoint pointB,QPoint pointC,QPoint pointD){
    int a_line1 = pointB.y()-pointA.y();
    int b_line1 = pointA.x()-pointB.x();
    int c_line1 = (pointB.x()*pointA.y())-(pointA.x()*pointB.y());

    int a_line2 = pointD.y()-pointC.y();
    int b_line2 = pointC.x()-pointD.x();
    int c_line2 = (pointD.x()*pointC.y())-(pointC.x()*pointD.y());

    int D = a_line2*b_line1-a_line1*b_line2;
    int x_inter = (int)round((double)(b_line2*c_line1-b_line1*c_line2)/(double)D);
    int y_inter = (int)round((double)(a_line1*c_line2-a_line2*c_line1)/(double)D);

    return QPoint(x_inter,y_inter);

    //以下方法求的是线段的交点
    //设线段AB和线段CD的交点为E,则CE/ED=abs(AB X AC)/abs(AB X AD)
    /*double ABxAC = (double)CrossProduct(pointA,pointB,pointA,pointC);
    double ABxAD = (double)CrossProduct(pointA,pointB,pointA,pointD);
    double ratio = absDouble(ABxAC/ABxAD);

    //double kCD = (double)(pointC.y()-pointD.y())/(double)(pointC.x()-pointD.x());
    int xE,yE;
    if(pointC.x() == pointD.x()){
        xE = pointC.x();
    }
    else{
        xE = (int)round((pointC.x()+ratio*pointD.x())/(1+ratio));
    }
    yE = (int)round((pointC.y()+ratio*pointD.y())/(1+ratio));
    return QPoint(xE,yE);*/
}


//画出多边形
void PaintBoard::drawPolygon(vector<QPoint> polygon, QColor color){
    QPainter p(&image);
    QPen pen;
    pen.setColor(color);
    p.setPen(pen);

    QPoint start,end;
    for(unsigned i=0;i<polygon.size();i++){
        if(i==polygon.size()-1){
            start = polygon[i];
            end = polygon[0];
        }
        else{
            start = polygon[i];
            end = polygon[i+1];
        }
        drawLine_MID(&p,start,end);
    }
}

//Sutherland-Hodgman算法裁剪多边形
void PaintBoard::CutPolygonSutherlandHodgman(){
    Line polygonSide;       //多边形的一条边
    Line cutFrameSide;      //裁剪框的一条边
    //目前仅支持矩形裁剪框
    vector<QPoint> cutPoints = {TopLeft,TopRight,BottomRight,BottomLeft};
    Polygon cutFrame(cutPoints);

    //更新每次裁剪的进度
    Polygon currentPolygon;

    unsigned i=0;
    while(i<polygons.size()){
        currentPolygon = polygons[i];
        for(unsigned k=0;k<cutFrame.points.size();k++){
            //一轮裁剪得到的坐标点
            vector<QPoint> getPolygon;

            //取出裁剪框的一条边
            if(k==cutFrame.points.size()-1){
                cutFrameSide = Line(cutFrame.points[k],cutFrame.points[0]);
            }
            else{
                cutFrameSide = Line(cutFrame.points[k],cutFrame.points[k+1]);
            }

            for(unsigned j=0;j<currentPolygon.points.size();j++){
                //取出多边形图元的一条边
                if(j==currentPolygon.points.size()-1){
                    polygonSide = Line(currentPolygon.points[j],currentPolygon.points[0]);
                }
                else{
                    polygonSide = Line(currentPolygon.points[j],currentPolygon.points[j+1]);
                }
                //计算裁剪边向量和当前取出的图元的边的端点的相对位置
                int flagStart = CrossProduct(cutFrameSide.start,cutFrameSide.end,cutFrameSide.start,polygonSide.start);
                int flagEnd = CrossProduct(cutFrameSide.start,cutFrameSide.end,cutFrameSide.start,polygonSide.end);
                //由外到内
                if(flagStart<0 && flagEnd>=0){
                    //计算当前多边形边和裁剪边的交点
                    QPoint intersectPoint = IntersectPoint(polygonSide.start,polygonSide.end,cutFrameSide.start,cutFrameSide.end);
                    getPolygon.push_back(intersectPoint);
                    getPolygon.push_back(polygonSide.end);
                }
                //由外到外
                else if(flagStart<0 && flagEnd<0){
                    //do nothing
                }
                //由内到外
                else if(flagStart>=0 && flagEnd<0){
                    QPoint intersectPoint = IntersectPoint(polygonSide.start,polygonSide.end,cutFrameSide.start,cutFrameSide.end);
                    getPolygon.push_back(intersectPoint);
                }
                //由内到内
                else if(flagStart>=0 && flagEnd>=0){
                    getPolygon.push_back(polygonSide.end);
                }

            }

            //更新当前的多边形,作为下一条裁剪边的输入
            currentPolygon = Polygon(getPolygon);
        }


        //修改当前多边形
        polygons[i] = currentPolygon;

        i++;    //遍历下一个多边形
    }

    //画出多边形
    QPainter p (&image);
    p.fillRect(image.rect(),QColor(255,255,255));
    for(unsigned j=0;j<polygons.size();j++){
        Polygon pg = polygons[j];
        vector<QPoint> points = pg.points;
        QPoint start,end;
        for(unsigned i=0;i<points.size();i++){
            if(i==points.size()-1){
                start = points[i];
                end = points[0];
            }
            else{
                start = points[i];
                end = points[i+1];
            }
            drawLine_MID(&p,start,end);
        }
    }

    update();
}

//裁剪区域(仅支持矩形裁剪框和白色背景)
void PaintBoard::CutArea(){
    if(!cutFlag){
        QMessageBox::critical(this,"错误","请先选中区域再进行裁剪!","确定");
        return;
    }
    QPainter p(&image);
    p.fillRect(image.rect(),QColor(255,255,255));
    int x= TopLeft.x(),y=TopLeft.y();
    int width=selectedArea.width(),height=selectedArea.height();
    p.drawImage(x,y,selectedArea);
    //消除虚线框
    QPen pen;
    pen.setColor(QColor(255,255,255));
    p.setPen(pen);
    p.drawRect(QRect(x,y,width,height));

    cutFlag = false;
    update();
}

void PaintBoard::RemoveSelectedArea(){
    //支持斜矩形区域
    for(int i=0;i<image.width();i++){
        for(int j=0;j<image.height();j++){
            if(insideSelectedArea(i,j))
                image.setPixelColor(i,j,QColor(255,255,255));
        }
    }
    update();
}

//根据传入的一个结束位置作出一条直线(任何图形都可视为直线经过一定变换之后的组合)
//此函数之后要改为调用自己的画线算法
void PaintBoard::drawLineTo(QPoint end){
    //构造函数QPainter(QPaintDevice *device)
    //QImage是QPaintDevice的一个子类
    //创建一个在image上作图的QPainter对象
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing,true);    //打开反走样
    painter.setPen(QPen(pencolor));         //设置画笔颜色
    switch(status){
    case drawPen:
        drawLine_DDA(&painter,lastpoint,end);
        break;
    case drawLine:
        //drawLine_DDA(&painter,lastpoint,end);
        drawLine_MID(&painter,lastpoint,end);
        break;

    case drawRect:
        drawRect_DDA(&painter,lastpoint,end);
        break;
    case drawCircle:
        drawCircle_MID(&painter,circle_center,radius);
        break;
    case drawEllipse:{

        drawEllipse_MID(&painter,QRect(lastpoint,end));
        //painter.drawEllipse(QRect(lastpoint,end));
        break;
    }
    case drawBrush:{
        QColor current_color=image.pixelColor(fill_center);  //获取当前位置原有的颜色
        //FloodSeedFill(&painter,fill_center,current_color);
        ScanSeedFill(&painter,fill_center,current_color);
        break;
    }
    default:
        break;
    }
    modified=true;  //对画布进行了修改
    //更新指定的一片矩形区域
    //normalized()保证矩形的长宽不为负
    update(QRect(lastpoint,end).normalized());
    lastpoint=end;  //对上一个位置进行更新
}


void PaintBoard::resizeImage(QImage* im, const QSize &newSize)
{
    if(im->size()==newSize)
        return;
    //创建一个大小为newSize,模式为RGB32的QImage
    QImage newImage(newSize,QImage::Format_RGB32);
    //将这个QImage填充为白色
    newImage.fill(qRgb(255,255,255));
    QPainter painter(&newImage);
    //开启反走样
    painter.setRenderHint(QPainter::Antialiasing,true);
    painter.drawImage(QPoint(0,0),*im);
    *im=newImage;

}
//直线生成的DDA算法
void PaintBoard::drawLine_DDA(QPainter* painter,QPoint loc_start, QPoint loc_end){
    painter->setRenderHint(QPainter::Antialiasing,true);
    if(loc_start.x()>loc_end.x()){  //起点在右侧,则交换两个点的位置
        QPoint tmp=loc_start;
        loc_start=loc_end;
        loc_end=tmp;
    }
    if(loc_start.x()==loc_end.x()){ //垂直线
        int i=min(loc_start.y(),loc_end.y());
        int j=max(loc_start.y(),loc_end.y());
        if(i==j){   //画一个点
            painter->drawPoint(loc_start.x(),i);
        }
        else{       //从较小的纵坐标开始画点
            for(;i<=j;i++)
                painter->drawPoint(loc_start.x(),i);
        }

    }
    else if(loc_start.y()==loc_end.y()){ //水平线
        int i=min(loc_start.x(),loc_end.x());
        int j=max(loc_start.x(),loc_end.x());
        for(;i<=j;i++)
            painter->drawPoint(i,loc_start.y());

    }
    else{
        //非垂直或水平

        double k=(double)(loc_end.y()-loc_start.y())/(double)(loc_end.x()-loc_start.x());    //计算斜率
        //cout<<k<<endl;
        int inc=k>0?1:-1;   //标记斜率的正负
        //inc*k表示斜率的绝对值
        if(inc*k>1){        //斜率绝对值大于1,则每次在y方向递增
            int i=min(loc_start.y(),loc_end.y());
            int j=max(loc_start.y(),loc_end.y());
            double xx=(i==loc_start.y())?loc_start.x():loc_end.x();
            for(;i<=j;i++){
                painter->drawPoint(floor(xx),i);
                xx=xx+(double)1/k;
            }

        }
        else if(inc*k<1){   //斜率绝对值小于1,则每次在x方向递增
            int i=min(loc_start.x(),loc_end.x());
            int j=max(loc_start.x(),loc_end.x());
            double yy=(i==loc_start.x())?loc_start.y():loc_end.y();
            for(;i<=j;i++){
                painter->drawPoint(i,floor(yy));
                yy=yy+(double)k;
            }
        }
        else{       //斜率的绝对值等于1
            int i=loc_start.x(),j=loc_start.y();

            for(;i<=loc_end.x();i++,j+=inc){
                painter->drawPoint(i,j);
            }
        }
    }

    //update();
    //return;

}
//直线生成的中点算法
//注意点:纸上作图时习惯将y轴正方向朝上,但屏幕上的y轴正方向是朝下的
//因此屏幕上的点与直线的相对位置(上方或者下方)与纸上画的是不同的
void PaintBoard::drawLine_MID(QPainter *painter, QPoint loc_start, QPoint loc_end){
    painter->setRenderHint(QPainter::Antialiasing,true);

    if(loc_start.x()==loc_end.x()){ //垂直线
        int i=min(loc_start.y(),loc_end.y());
        int j=max(loc_start.y(),loc_end.y());
        if(i==j){   //画一个点
            painter->drawPoint(loc_start.x(),i);
        }
        else{       //从较小的纵坐标开始画点
            for(;i<=j;i++)
                painter->drawPoint(loc_start.x(),i);
        }

    }
    else if(loc_start.y()==loc_end.y()){ //水平线
        int i=min(loc_start.x(),loc_end.x());
        int j=max(loc_start.x(),loc_end.x());
        for(;i<=j;i++)
            painter->drawPoint(i,loc_start.y());

    }
    else{           //既非水平也非垂直
        double k=(double)(loc_end.y()-loc_start.y())/(double)(loc_end.x()-loc_start.x());
        int inc=k>0?1:-1;

        if(inc*k>1){    //斜率的绝对值大于1
            //交换点的坐标使得loc_start为纵坐标较小的那个
            if(loc_start.y()>loc_end.y()){
                QPoint tmp=loc_start;
                loc_start=loc_end;
                loc_end=tmp;
            }
            int A=loc_end.y()-loc_start.y();
            int B=loc_start.x()-loc_end.x();
            int D=2*B*B+inc*A*B;        //初始决策变量,注意与inc的值有关
            int i=loc_start.x(),j=loc_start.y();

            for(;j<=loc_end.y();j++){
                painter->drawPoint(i,j);
                if(D<0){    //参考点在直线的下方
                    D+=(2*B*B);
                }
                else{       //参考点在直线上或者在直线的上方
                    i+=inc;
                    D+=(2*inc*A*B+2*B*B);
                }
            }

        }
        else if(inc*k<1){   //斜率的绝对值小于1
            //交换坐标位置使得loc_start为横坐标较小的那个
            if(loc_start.x()>loc_end.x()){
                QPoint tmp=loc_start;
                loc_start=loc_end;
                loc_end=tmp;
            }
            int A=loc_end.y()-loc_start.y();
            int B=loc_start.x()-loc_end.x();
            int D=B*(2*A+inc*B);
            int i=loc_start.x(),j=loc_start.y();
            for(;i<=loc_end.x();i++){
                painter->drawPoint(i,j);
                if(D<0){
                    //注意这里还要根据斜率的正负进行讨论
                    if(k>0){
                        j+=inc;
                        D+=(2*A*B+2*inc*B*B);
                    }
                    else{
                        D+=2*A*B;
                    }

                }
                else{
                    if(k<0){
                        j+=inc;
                        D+=(2*A*B+2*inc*B*B);
                    }
                    else{
                        D+=2*A*B;
                    }
                }
            }
        }
        else{       //斜率的绝对值等于1
            int i=loc_start.x(),j=loc_start.y();
            for(;i<=loc_end.x();i++,j+=inc){
                painter->drawPoint(i,j);
            }
        }
    }
    lineStart = loc_start;
    lineEnd = loc_end;
}


//基于DDA算法画矩形
void PaintBoard::drawRect_DDA(QPainter *painter, QPoint pp1, QPoint pp2){
    painter->setRenderHint(QPainter::Antialiasing,true);
    QPoint leftup,rightup,rightdown,leftdown;
    //调整矩形的四个顶点使其按左上、右上、右下、左下的顺序排列

    //左上到右下
    if(pp1.x()<pp2.x() && pp1.y() < pp2.y()){
        leftup = pp1;
        rightup = QPoint(pp2.x(),pp1.y());
        rightdown = pp2;
        leftdown = QPoint(pp1.x(),pp2.y());
    }
    //右上到左下
    else if(pp1.x()>= pp2.x() && pp1.y() < pp2.y()){
        leftup = QPoint(pp2.x(),pp1.y());
        rightup = pp1;
        rightdown = QPoint(pp1.x(),pp2.y());
        leftdown = pp2;
    }
    //右下到左上
    else if(pp1.x()>=pp2.x() && pp1.y()>=pp2.y()){
        leftup = pp2;
        rightup = QPoint(pp1.x(),pp2.y());
        rightdown = pp1;
        leftdown = QPoint(pp2.x(),pp1.y());
    }
    //左下到右上
    else if(pp1.x()<pp2.x() && pp1.y()>=pp2.y()){
        leftup = QPoint(pp1.x(),pp2.y());
        rightup = pp2;
        rightdown = QPoint(pp2.x(),pp1.y());
        leftdown = pp1;
    }

    drawLine_DDA(painter,leftup,rightup);
    drawLine_DDA(painter,leftup,leftdown);
    drawLine_DDA(painter,rightup,rightdown);
    drawLine_DDA(painter,leftdown,rightdown);

    //记录矩形位置(顺时针)
    tmpPoly = Polygon(leftup,rightup,rightdown,leftdown);
}


void PaintBoard::drawCircle_MID(QPainter *painter, QPoint center, int radius){
    painter->setRenderHint(QPainter::Antialiasing,true);
    int xc=center.x(),yc=center.y();
    QPoint leftup(xc-radius,yc-radius);
    QPoint rightdown(xc+radius,yc+radius);
    //构造正方形后调用画椭圆算法
    drawEllipse_MID(painter,QRect(leftup,rightdown));
}
//中点椭圆生成算法
void PaintBoard::drawEllipse_MID(QPainter *painter, QRect rect){

    painter->setRenderHint(QPainter::Antialiasing,true);
    int xc=rect.center().x(),yc=rect.center().y();
    int rx=rect.width()/2, ry=rect.height()/2;

    if(rx==0){      //垂直线
        int i=xc, j=yc-ry;
        for(;j<=yc+ry;j++)
            painter->drawPoint(i,j);
        return;
    }
    if(ry==0){      //水平线
        int i=xc-rx, j=yc;
        for(;i<=xc+rx;i++)
            painter->drawPoint(i,j);
        return;
    }
    int i=xc, j=yc+ry;

    double D=2*ry*ry*(i-xc)+ry*ry-rx*rx*(j-yc)+0.25*rx*rx;
    while(rx*rx*(2*j-2*yc-1)-2*ry*ry*(i-xc+1)>=0 && i<=xc+rx){   //在x递增的区域中循环
        //根据椭圆的对称性画出对应的点
        painter->drawPoint(i,j);
        painter->drawPoint(2*xc-i,j);
        painter->drawPoint(i,2*yc-j);
        painter->drawPoint(2*xc-i,2*yc-j);
        if(D<0){
            D+=(2*ry*ry*(i-xc)+3*ry*ry);
        }
        else{
            j--;
            D+=(2*ry*ry*(i-xc)+3*ry*ry-2*rx*rx*(j-yc)+2*rx*rx);
        }
        i++;
    }

    D=ry*ry*(i-xc)+0.25*ry*ry-2*rx*rx*(j-yc)+rx*rx;
    while(rx*rx*(2*j-2*yc-1)-2*ry*ry*(i-xc+1)<0 && j>=yc){   //在y递减的区域中循环
        painter->drawPoint(i,j);
        painter->drawPoint(2*xc-i,j);
        painter->drawPoint(i,2*yc-j);
        painter->drawPoint(2*xc-i,2*yc-j);
        if(D<0){
            i++;
            D+=(2*ry*ry*(i-xc)+2*ry*ry-2*rx*rx*(j-yc)+3*rx*rx);
        }
        else{
            D+=(3*rx*rx-2*rx*rx*(j-yc));
        }
        j--;
    }
    return;

}

//递归种子填充算法
//优点:实现简单 缺点:开销太大,图形面积大时程序会崩溃...
void PaintBoard::FloodSeedFill(QPainter* painter,QPoint p, QColor current_color){

    painter->drawPoint(p.x(),p.y());
    update();
    int xx,yy;
    for(int i=0;i<4;i++){
        xx=p.x()+direction[i][0];
        yy=p.y()+direction[i][1];
        QPoint tmp(xx,yy);
        if(image.pixelColor(tmp)==current_color)
            FloodSeedFill(painter,tmp,current_color);
    }

}
//递归扫描线填充算法
//大大降低开销,解决图形面积较大时程序崩溃的问题
void PaintBoard::ScanSeedFill(QPainter *painter, QPoint p, QColor current_color){
    //选中区域无须填充
    if(pencolor==current_color)
        return;

    int xleft,xright;       //记录每条扫描线的左端和右端
    stack <QPoint> s;
    s.push(p);
    QPoint get;
    while(!s.empty()){
        update();        //每处理完一条线之后进行更新
        get = s.top();   //从栈中取出一个点
        s.pop();
        int x=get.x(),y=get.y();

        //向右填充
        while(image.pixelColor(x,y)==current_color){
            painter->drawPoint(x,y);
            x++;
            if(x>=image.width())
                break;
        }
        xright=x-1;     //设置扫描线右端点
        //向左填充
        x=get.x()-1;
        while(image.pixelColor(x,y)==current_color){
            painter->drawPoint(x,y);
            x--;
            if(x<0)
                break;
        }
        xleft=x+1;      //设置扫描线左端点

        bool ifNeedFill;    //标记是否需要填充
        //处理上一条扫描线
        x=xleft;
        y=y-1;
        while(x<xright && y>=0){
            ifNeedFill=false;
            while(x<image.width() && image.pixelColor(x,y)==current_color){
                ifNeedFill=true;
                x++;
            }
            if(ifNeedFill){
                QPoint tmp(x-1,y);
                s.push(tmp);
                ifNeedFill=false;
            }
            if(x>=image.width())
                continue;
            while(image.pixelColor(x,y)!=current_color && x<xright)
                x++;
        }
        //处理下一条扫描线
        x=xleft;
        y=y+2;
        while(x<xright && y<image.height()){
            ifNeedFill=false;
            while(x<image.width() && image.pixelColor(x,y)==current_color){
                ifNeedFill=true;
                x++;
            }
            if(ifNeedFill){
                QPoint tmp(x-1,y);
                s.push(tmp);
                ifNeedFill=false;
            }
            if(x>=image.width())
                continue;
            while(image.pixelColor(x,y)!=current_color && x<xright)
                x++;
        }
    }
}







