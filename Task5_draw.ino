/* =====================================================================
   任务五：轨迹绘制（示教 / 直线 / 图形 / 折线 / 曲线）
   ---------------------------------------------------------------------
    按键安排：
     短按 5 = 记一个示教点（最多 5 个）      长按 5 = 清空示教点
     短按 6 = 开始画 / 正在画时再按一次 = 停  长按 6 = 换子任务
     短按 7 = 暂停 / 继续                 短按 8 = 取消（停住回待机）          
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
int segTotalSteps = 0; //绘制本段需要多少步
int segCurrentStep = 0; //本段绘制到第几步

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
float paperToWorldX(float u){return u + paperX;}
float paperToWorldY(float v){return v + paperY;}

void initSegment(){

}

//任务五：开始画
void startDraw(){
   if(drawState != 0){
    Serial.println(F("Busy!"));
    return;      
   }   

   if(taskType == 0){//画直线

   }
   else if(taskType == 1){//画图形

   }
   else if(taskType == 2){//画折线

   }
   else if(taskType == 3){//画曲线

   }
   else{
      
   }
}

void startGohome(){

}

//记一个示教点
void recordTeachPoints(){
   if(drawState != 0){
    Serial.println(F("Busy!"));
    return;      
   }

   if(teachCount >= MAX_POINTS){
    Serial.println(F("# 已经记满 5 个点了(长按按键5 清空)"));
    return;      
   }
   calcTipPosition();
   teachX[teachCount] = tipX;
   teachY[teachCount] = tipY;
   teachCount++;

   Serial.print(F("# P"));
   Serial.print(teachCount);
   Serial.print(F(" u="));
   Serial.print(tipX - paperX, 1);
   Serial.print(F(" v="));
   Serial.print(tipY - paperY, 1);
   Serial.print(F(" x="));
   Serial.print(tipX, 1);
   Serial.print(F(" y="));
   Serial.println(tipY, 1);
}

//切换任务状态
void switchTask(){
   if(drawState != 0){
   Serial.println(F("# 正在动，先取消再换任务"));
   return;
   }
   taskType++;
   if(taskType > 3) taskType = 0;
   Serial.print(F("# 任务类型 = "));
   Serial.print(taskType);
}
