/* =====================================================================
   任务五：轨迹绘制（示教 / 直线 / 图形 / 折线 / 曲线）
   ---------------------------------------------------------------------
    按键安排：
     短按 5 = 记一个示教点（最多 5 个）       长按 5 = 清空示教点
     短按 6 = 开始画 / 正在画时再按一次 = 停  长按 6 = 换子任务
     短按 7 = 暂停 / 继续                    短按 8 = 取消（停住回待机）          

    坐标系设置：
      设置基坐标系和纸面坐标系，好处是增加两个函数，大大减小了纸面挪移时修改坐标代码的难度，只需要更改paperX/paperY
    
    坐标系分工：
      纸面坐标系 -> 设置直线端点/设置图形端点
      基坐标系 -> 正运动学/逆运动学/记录示教点/驱动舵机(移动笔尖)
   ----------------------------------------------------------------------
    绘图状态机(drawstate)
      0 = 待机（遥杆可以动，可以示教）
      1 = 回待机状态（线性插值走回去）
      2 = 抬笔去第一个点
      3 = 落笔画线中
===================================================================== */

/*========================第0层：相关变量定义=========================*/
//1.示教点
float teachX[MAX_POINTS];
float teachY[MAX_POINTS];

//2.路径点
float pathX[MAX_POINTS];
float pathY[MAX_POINTS];
int pathCount = 0;

//3.配置绘图待机状态
int homeBase = 90, homeSh = 110, homeEl = 85, homeGr = 155;

//4.绘图相关
//根据子任务不同，设置不同的绘制段数
//根据插值比例，设置绘制每段所需的步数
int segIndex = 0; //本次绘制画到第几段
int segTotalSteps = 0; //绘制本段需要多少步，设定每一小步走1.5mm
int segNowStep = 0; //本段绘制到第几步

float segStartX = 0; //这一段绘制开始时的起点x(相对基坐标系)
float segStartY = 0; //这一段绘制开始时的起点y(相对基坐标系)
float segEndX = 0; //这一段绘制结束时的终点x(相对基坐标系)
float segEndY = 0; //这一段绘制结束时的终点x(相对基坐标系)

float nowX = 0; //在该段中绘制到某一步时的x坐标(相对基坐标系)
float nowY = 0; //在该段中绘制到某一步时的y坐标(相对基坐标系)

//5.回待机状态
int afterHome = 0; //回完待机以后去哪个状态
float StartB, StartS, StartE; //从这三个初始角度开始回待机
float homeB = 90, homeS = 110, homeE = 85, homeG = 155; //标准待机角度
int homeStep = 0, homeSteps = 30;

//子任务一直线的首尾点坐标规定(相对于纸面坐标系)
float lineX1_inpaper = 0.0, lineY1_inpaper = -30.0;
float lineX2_inpaper = 0.0, lineY2_inpaper = 30.0;

//子任务二绘制三角形，定义三角形在纸面上的坐标
int shapePoints[8] = {-20,-20,  20,-20,  0,20,  -23,-23};

//绘图中无法到达的点位计数
int missCount = 0;

/*========================第1层：工具层=========================*/
// 1. 把纸面参考系中的笔尖坐标转换回绝对参考系中的笔尖坐标
// 沟通两个坐标系的关键
float paperToWorldX(float u){return u + paperX;}
float paperToWorldY(float v){return v + paperY;}

// 2. 当前子任务叫什么名字
const char* taskName(){
   if(taskType == 0) {return "line";}//子任务一
   if(taskType == 1) {return "shape";}//子任务二
   if(taskType == 2) {return "polyline";}//子任务四
   return "curve";//子任务五

}

// 3. 记一个示教点
void recordTeachPoints(){
   if(drawState != 0){//在绘制，拒绝记录
    Serial.println(F("Busy!"));
    return;      
   }

   if(teachCount >= MAX_POINTS){//记满了，提醒清零
    Serial.println(F("# 已经记满 5 个点了(长按按键5 清空)"));
    return;      
   }
   calcTipPosition();//过了上面两关，计算绝对坐标
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

//4.切换子任务
void switchTask(){
   if(drawState != 0){
   Serial.println(F("# 正在动，先取消再换任务"));
   return;
   }
   taskType++;
   if(taskType > 3) taskType = 0;
   Serial.print(F("# 任务类型 = "));
   Serial.print(taskType);
   Serial.print(F(" ("));
   Serial.print(taskName());
   Serial.println(F(")"));   
}

/*========================第2层：插值准备层=========================*/
// 1. 准备当前线段插值，算出起点、终点，共有几步
void initSegment(){
   if(segIndex < 0){ //此时准备的是从待机状态走到第一个点的插值
      segStartX = tipX;
      segStartY = tipY;
      segEndX = pathX[0];
      segEndY = pathY[0];
   }
   else{ //常规线段插值
      segStartX = pathX[segIndex];
      segStartY = pathY[segIndex];
      segEndX = pathX[segIndex + 1];
      segEndY = pathY[segIndex + 1];      
   }

   float dx = segEndX - segStartX;
   float dy = segEndY - segStartY;
   float len = sqrt(dx*dx + dy*dy);//计算该段的长度

   segTotalSteps = (int)(len / 1.5);//计算要走几步
   segNowStep = 0;
}

// 2. 准备曲线插值，采用Catmull-Rom样条曲线进行插值，用于子任务五画曲线，t的范围在0~1
/*返回值的定义为由四个控制点 p0、p1、p2、p3 定义的 Catmull-Rom 样条曲线，在参数 t 处的插值结果
  核心：输入 4 个控制点 + 进度 t，输出该点的插值坐标*/    
float initSplineCurve(float p0, float p1, float p2, float p3, float t){
   float a = 2.0 * p1;
   float b = -p0 + p2;
   float c = 2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3;
   float d = -p0 + 3.0*p1 - 3.0*p2 + p3;
   float t1 = t;
   float t2 = t * t;
   float t3 = t * t * t;
   return 0.5 * (a + b*t1 + c*t2 + d*t3);
}

// 3. 计算当前这一步的笔尖位置
//    核心：用一个函数处理 4 种任务类型，通过 taskType 区分插值算法
void calcStepPointXY(){
   float t = (float)segNowStep / (float)segTotalSteps; //比例（画线进度）0~1
   
   if(segIndex >= 0 && taskType == 3){ //曲线插值
      int i = segIndex;
      // 曲线：用前后各一个点做样条，首尾没有点就拿自己代替
      // 路径点： P0  P1  P2  P3  P4
      // 索引：   0   1   2   3   4
      float p0x = (i == 0)? pathX[0] : pathX[i - 1];
      float p0y = (i == 0)? pathY[0] : pathY[i - 1];
      float p3x = (i + 2 <= pathCount - 1)? pathX[i + 2] : pathX[pathCount - 1];
      float p3y = (i + 2 <= pathCount - 1)? pathY[i + 2] : pathY[pathCount - 1];

      nowX = initSplineCurve(p0x, pathX[i], pathX[i + 1], p3x, t);
      nowY = initSplineCurve(p0y, pathY[i], pathY[i + 1], p3y, t);
   } 
   else { //直线、图形、三角形：线性插值
      nowX = segStartX + (segEndX - segStartX) * t;
      nowY = segStartY + (segEndY - segStartY) * t;
   }
}

/*========================第3层：触发层=========================*/
// 1. 回待机状态(next = 回完舵机以后去哪 0=保持待机 2=去画第一个点)
void startGoHome(int next){
   drawState = 1;
   StartB = baseAngle;
   StartE = elAngle;
   StartS = shAngle;
   homeStep = 0;
   homeSteps = 30;
   afterHome = next;
   isAutoRunning = true;
   lastStepTime = millis();
}

// 2. 去第一个点位
void startGoFirstPoint(){
   segIndex = -1;  // -1 表示这一段是从当前位置走到 pathX[0],即第一个点位
   drawState = 2;
   initSegment(); //对该段进行插值计算
   lastStepTime = millis(); //重新对齐计时
}

// 3. 开始画
void startDraw(){
   if(drawState != 0){
      Serial.println(F("# 正在画，用按键 7 暂停/继续 用按键8取消当前状态回待机"));
      return;      
   }
   // 按任务类型先把这一趟的路径点准备好
   if(taskType == 0){
      pathCount = 2;
      pathX[0] = paperToWorldX(lineX1_inpaper);
      pathY[0] = paperToWorldY(lineY1_inpaper);
      pathX[1] = paperToWorldX(lineX2_inpaper);
      pathY[1] = paperToWorldY(lineY2_inpaper);
   }
   else if(taskType == 1){
      pathCount = 4;
      for(int i = 0 ; i < pathCount ; i++){
         pathX[i] = paperToWorldX(shapePoints[i * 2]);
         pathY[i] = paperToWorldY(shapePoints[i * 2 + 1]);
      }
   }
   else{
      if(teachCount < 2){
         Serial.println(F("# 示教点不够,先短按按键5 记点"));
         return;
      }
      pathCount = teachCount;
      for(int i = 0 ; i < pathCount ; i++){
         pathX[i] = teachX[i];
         pathY[i] = teachY[i];
      }
   }
   //先回待机姿态，再抬着笔去第 1 个点
   isAutoRunning = true;
   paused = false;
   missCount = 0;
   Serial.print(F("# start drawing, task="));
   Serial.print(taskName());
   Serial.print(F(" points="));
   Serial.println(pathCount);
   startGoHome(2);
}

/*========================第4层：状态机层=========================*/
//每轮 loop() 走一小步。非阻塞状态机
void tickDraw(){
   if(drawState == 0 || paused) return; //待机或者暂停
   if(millis() - lastStepTime < nowStepTime) return; //时间间隔没走完
   lastStepTime = millis(); //实时记录时间
   nowStepTime = drawStepTime; //复位节拍

   if(drawState == 1){//回待机
      homeStep++;
      if(homeStep >= homeSteps){
         setAngles(homeB, homeS, homeE, homeG);
         calcTipPosition();
         if(afterHome == 2){
            startGoFirstPoint();
         } else {
            drawState = 0;
            isAutoRunning = false;
            Serial.println(F("# home, ready"));
         }
         return;
      }
      float b = StartB + (homeB - StartB) * (homeStep / homeSteps);
      float s = StartB + (homeS - StartS) * (homeStep / homeSteps);
      float e = StartB + (homeE - StartE) * (homeStep / homeSteps);
      setAngles(b, s, e, grAngle);
      return;
   }

   if(drawState == 2){
      segNowStep++;
      if(segNowStep > segTotalSteps){
         moveTipTo(pathX[0], pathY[0], paperZ - penPress);
         segIndex = 0;
         initSegment();
         drawState = 3;
         nowStepTime = drawStepTime + connerWaitTime;
         return;
      }
      calcStepPointXY();
      if(!moveTipTo(nowX, nowY, paperZ + liftZ)) missCount++;
      return;
   }

   if(drawState == 3){
      segNowStep++;
      if(segNowStep > segTotalSteps){
         segIndex++;
         if(segIndex > pathCount - 2){
            Serial.print(F("# drawing done  misscount = "));
            Serial.println(missCount);
            startGoHome(0);
            nowStepTime = drawStepTime + connerWaitTime; //笔尖停一下再起来
            return;
         }
         initSegment();
         nowStepTime = drawStepTime + connerWaitTime;
         return;
      }
      calcStepPointXY();
      if(!moveTipTo(nowX, nowY, paperZ - penPress)) missCount++;
      return;
   }
}
