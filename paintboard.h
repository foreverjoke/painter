#ifndef PAINTBOARD_H
#define PAINTBOARD_H
#include <QImage>
#include <QWidget>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QRect>
#include <QColor>
#include <QMessageBox>
#include <vector>
using namespace std;
//线段结构体(记录起点和终点)
struct Line{
    QPoint start,end;
    Line() = default;
    ~Line() = default;
    Line(QPoint s,QPoint e){
        this->start = s;
        this->end = e;
    }
    Line& operator=(const Line& other){
        this->start = other.start;
        this->end = other.end;
        return *this;
    }
    bool operator == (Line& other){
        if(this->start == other.start
                && this->end == other.end)
            return true;
        else
            return false;
    }
    bool operator !=(Line& other){
        return !(*this==other);
    }
};
//多边形结构体
struct Polygon{
    vector <QPoint> points; //顶点集,注意按顺时针顺序存放
    Polygon() = default;
    ~Polygon() = default;
    //四边形
    Polygon(QPoint p1,QPoint p2,QPoint p3,QPoint p4){
        points.push_back(p1);
        points.push_back(p2);
        points.push_back(p3);
        points.push_back(p4);
    }
    //五边形
    Polygon(QPoint p1, QPoint p2, QPoint p3, QPoint p4,QPoint p5){
        points.push_back(p1);
        points.push_back(p2);
        points.push_back(p3);
        points.push_back(p4);
        points.push_back(p5);
    }

    Polygon(vector<QPoint> p){
        this->points.assign(p.begin(),p.end());
    }

    Polygon& operator=(const Polygon& poly){
        //vector赋值
        (this->points).assign(poly.points.begin(),poly.points.end());
        return *this;
    }
    bool operator == (const Polygon& poly){
        if((this->points)==poly.points)
            return true;
        else
            return false;
    }
    bool operator !=(const Polygon& poly){
        return !(*this == poly);
    }
};

class PaintBoard : public QWidget
{
    Q_OBJECT

public:
    //表示当前画板状态的枚举变量
    enum Status{drawPen,drawLine,drawRect,drawCircle,drawEllipse,drawBrush,
               select,moving,stop};
    explicit PaintBoard(QWidget *parent = 0);

    bool openImage(const QString &fileName);    //打开图片
    bool saveImage(const QString &fileName, const char* fileFormat);    //保存图片
    bool isModified()const{         //是否被修改过
        return modified;
    }

protected:
    //定义鼠标事件、画图事件、尺寸的变换事件
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
private:

    QImage image;
    QPoint p1,p2;
    QPoint lastpoint,endpoint;
    QPoint lineStart,lineEnd;   //画线段时记录线段的起点和终点
    QPoint circle_center;      //画圆时记录圆心
    QColor pre_color;       //记录上一时刻画笔的颜色(用于边界填充算法)
    QPoint fill_center;     //填充算法的起始点
    int radius;         //画圆时记录半径
    bool painting;      //标记绘画是否正在进行
    bool modified;      //标记当前图片是否被修改
    QColor pencolor;    //记录画笔颜色
    QPoint TopLeft,TopRight,BottomLeft,BottomRight; //选中区域的四个坐标点
    QPoint moveStart,moveEnd;       //平移变换起始和结束位置
    //QPoint zoomCenter;          //缩放的基准点
    //各种操作状态的标记
    bool moveFlag,cutFlag,zoomInFlag,zoomOutFlag,rotateFlag;
    QImage selectedArea;    //选中区域
    QImage selectBuffer;    //选择区域过程中的缓存
    QImage transferBuffer;  //图形平移过程中的缓存
    //QImage zoomBuffer;      //图形放大/缩小过程中的缓存
    QImage rotateBuffer;    //图形旋转的缓存

    Status status;           //画板当前状态
    vector <Line> lines;     //记录画板上的线段
    Polygon tmpPoly;
    vector <Polygon> polygons;  //记录画板上的多边形
    void drawLineTo(QPoint);
    void resizeImage(QImage* , const QSize&);
    void paint(QPainter*);  //自定义的作图函数
    //调整选中区域的四个点的位置
    void setPoints();
    //判断点是否在选中区域内
    bool insideSelectedArea(int xp,int yp);
    //用DDA算法画直线
    void drawLine_DDA(QPainter* painter,QPoint loc_start,QPoint loc_end);
    //用中点算法画直线
    void drawLine_MID(QPainter* painter,QPoint loc_start,QPoint loc_end);
    //基于DDA算法画矩形
    void drawRect_DDA(QPainter* painter,QPoint leftup,QPoint rightdown);
    //用中点算法画圆和椭圆
    void drawCircle_MID(QPainter* painter,QPoint center,int radius);
    void drawEllipse_MID(QPainter* painter, QRect rect);
    //用递归种子填充算法填充指定区域
    void FloodSeedFill(QPainter* painter,QPoint p,QColor current_color);
    //用递归扫描线填充算法填充指定区域
    void ScanSeedFill(QPainter* painter,QPoint p,QColor current_color);
    //画出当前选中的区域
    void drawSelectedArea();
    //获取选中区域
    void getSelectedArea();
    //平移选中的区域(目前只支持矩形区域)
    void drawTransferImage(QPoint setPoint);
    //根据设置的放缩中心进行放缩
    QPointF zoomInReverseLoc(QPoint dst,QPoint center,double ratio);   //放大时点的坐标的逆变换
    //双线性插值获得一个位置的像素值
    QRgb BilinearInterpolation(qreal x,qreal y);
    //用Cohen-Sutherland算法进行直线裁剪
    //仅支持矩形裁剪区域,且擦除原有线段的方法待改进
    void CohenSutherland();
    //根据给出的多边形顶点坐标和指定颜色画出多边形
    //顶点坐标已经按顺时针排列
    void drawPolygon(vector<QPoint>polygon,QColor color);
    //获得擦除某个位置所需的颜色(不同色的临近点的均值)
    QColor getEraseColor(QPoint pos);
public slots:
    //设置画笔类型的槽
    void setstatuspen();
    void setstatusline();
    void setstatusrect();
    void setstatuscircle();
    void setstatusellipse();
    void setstatusbrush();

    void setstatusSelect();     //选择区域

    void setstatusMove();       //移动区域
    void ZoomInArea();          //放大(双线性插值算法)
    void ZoomOutArea();         //缩小

    void RotateClockWise();         //顺时针旋转
    void RotateCounterClockWise();  //逆时针旋转

    //裁剪线段、多边形、区域
    void CutLineCohenSutherland();
    void CutPolygonSutherlandHodgman();
    void CutArea();

    void RemoveSelectedArea();  //擦除选中区域
    //设置画笔颜色的槽
    void setcolorBlack();
    void setcolorWhite();
    void setcolorRed();
    void setcolorOrange();
    void setcolorYellow();
    void setcolorGreen();
    void setcolorCyan();
    void setcolorBlue();
    void setcolorPurple();
};

#endif // PAINTBOARD_H
