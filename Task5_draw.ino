/* =====================================================================
   任务五：轨迹绘制（示教 / 直线 / 图形 / 折线 / 曲线）
===================================================================== */

//示教点
int teachX[MAX_POINTS];
int teachY[MAX_POINTS];
int teachCount = 0;

//路径点
int pathX[MAX_POINTS];
int pathY[MAX_POINTS];
int pathCount = 0;

//配置绘图待机状态
int homeBase = 90, homeSh = 110, homeEl = 85, homeGr = 70;

//绘图相关
int segIndex = 0; //本次绘制画到第几段
int segTotalSteps = 0; //本次绘制需要多少步
int segCurrentStep = 0; //本次绘制到第几步

float segStartX = 0; //绘制开始时的起点x
float segStartY = 0; //绘制开始时的起点y
float segEndX = 0; //绘制结束时的终点x
float segEndY = 0; //绘制结束时的终点x

float nowX = 0; //绘制到某一步时的x坐标
float nowY = 0; //绘制到某一步时的y坐标

//子任务一直线的首尾点坐标规定(相对于纸面中心)
float lineX1_inpaper = -30.0, lineY1_inpaper = 0.0;
float lineX2_inpaper = 30.0, lineY2_inpaper = 0.0;

//把纸面参考系中的笔尖坐标转换回绝对参考系中的笔尖坐标
float xPaperToWorld(float u){return u + paperX;}
float yPaperToWorld(float v){return v + paperY;}
