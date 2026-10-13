/*=====================================================================
  任务四：机械臂正 / 逆运动学
    ---------------------------------------------------------------------
   坐标系：底座转轴正下方的桌面点是原点 (0,0,0)，x 朝机械臂方向、y在x左边、z 竖直向上，单位 mm。
      r = 笔尖离底座转轴这条竖线的水平距离（
      正解 calcTipPosition()：舵机角 -> 笔尖坐标 tipX / tipY / tipZ
      逆解 solveIK()        ：笔尖坐标 -> 舵机角 ikBase / ikSh / ikEl
      包一层 moveTipTo()    ：算完直接把舵机写下去（超范围会被 setAngles 卡住）

      大臂舵机 y ──查表（线性插值）──> 肘转轴的真实位置 (er, eh) // 半径, 离桌高
      小臂舵机 z ──分两段线性────────> 小臂的绝对角 a // 与水平面的夹角
      爪缝中心 = ( er + armLen2*cos(a),  eh + armLen2*sin(a) )
      笔尖 = 爪缝中心正下方 penDown 处 //笔始终竖直
      底座舵机转角t
      x = r*cos(t)   y = r*sin(t)   z = 爪缝高度 - penDown

   ---------------------------------------------------------------------
  第 4 层：驱动层                              
    moveTipTo()           逆解 + 写舵机      

                    ↑ 调用

  第 3 层：运动学层                            
    calcTipPosition()     正解（角度→坐标）  
    solveIK()             逆解（坐标→角度）  

                    ↑ 调用

  第 2 层：传动换算层                          
    forearmServoToAbsAngle()  舵机角→绝对角   
    AbsAngleToforearmServo()  绝对角→舵机角   
    shoulderServoToAbsAngle()          等效大臂角       

                    ↑ 调用

  第 1 层：查表工具层                          
    shoulderServoToElbowR()       大臂角→肘水平距离  
    shoulderServoToElbowH()       大臂角→肘高度      
    elbowDistanceTo()     肘到目标点的距离   

*/

/*========================第1层：查表工具层========================*/
/*1) 输入大臂舵机角 y，进行查表与插值计算，返回肘转轴离底座转轴的水平距离 er*/
float shoulderServoToElbowR(float ShAngle){
  if(ShAngle < shoulderServoAngles[0]) ShAngle = shoulderServoAngles[0];
  if(ShAngle > shoulderServoAngles[4]) ShAngle = shoulderServoAngles[4];
  for(int i = 0 ; i < elbowJointPoints - 1 ; i++){
    if(ShAngle <= shoulderServoAngles[i + 1]){
      float k = (shAngle - shoulderServoAngles[i]) / (shoulderServoAngles[i + 1] - shoulderServoAngles[i]);
      float er = elbowJointH[i] + (elbowJointH[i+1] - elbowJointH[i]) * k;
      return er;
    }
  }
}

/*2) 输入大臂舵机角 y，进行查表与插值计算，返回肘转轴离桌面的高度 eh*/
float shoulderServoToElbowH(float ShAngle){
  if(ShAngle < shoulderServoAngles[0]) ShAngle = shoulderServoAngles[0];
  if(ShAngle > shoulderServoAngles[4]) ShAngle = shoulderServoAngles[4];
  for(int i = 0 ; i < (elbowJointPoints - 1) ; i++){
    if(ShAngle <= shoulderServoAngles[i + 1]){
      float k = (shAngle - shoulderServoAngles[i]) / (shoulderServoAngles[i + 1] - shoulderServoAngles[i]);
      float eh = elbowJointH[i] + (elbowJointH[i+1] - elbowJointH[i]) * k;
      return eh;
    }
  }
}

/*3) 输入大臂角 y （利用上面两个函数算肘关节位置）和目标点 (r, h)，返回肘到目标点的直线距离h*/
float elbowDistanceTo(float ShAngle, float r, float h){
  float dr = r - shoulderServoToElbowR(ShAngle);
  float dh = h - shoulderServoToElbowH(ShAngle);
  float d = sqrt(dr * dr + dh * dh);
  return d;
}

/*========================第2层：传动换算层========================*/
/*1) 输入小臂舵机角，返回小臂的绝对角（相对于水平面）*/
float forearmServoToAbsAngle(float ElAngle){
  float deltaAngle = ElAngle - foreArmAbsAngle; 
  if(deltaAngle <= 0) return foreArmAbsAngle + deltaAngle * armGainDown;
  return foreArmAbsAngle + deltaAngle * armGainUp;
}

/*2) forearmServoToAbsAngle的逆函数，输入小臂绝对角（相对于水平面），返回小臂舵机角*/
float AbsAngleToforearmServo(float AbsAngle){
  float deltaAlpha = AbsAngle - foreArmAbsAngle;
  if(deltaAlpha <= 0) return elbowServoNode + deltaAlpha / armGainDown;
  return elbowServoNode + deltaAlpha / armGainUp;
}

/*3) 输入大臂舵机角度，返回等效大臂绝对角(只用于防自撞的判断)*/
float upperArmServoToAbsAngle(float ShAngle){
  float ddr = shoulderServoToElbowR(ShAngle) - shoulderJointR;
  float ddh = shoulderServoToElbowH(ShAngle) - shoulderJointH;
  return atan2(ddh, ddr) / PI * 180.0 ;
}

/*========================第3层：运动学层=======================*/
/*1)  正运动学：根据三个舵机当前角度，算出笔尖坐标 (tipX, tipY, tipZ)
      用的是几何拼接*/
void calcTipPosition(){
  // 1. 底座舵机角 → 数学方位角
  float t = (baseAngle - baseZero) * baseDir / 180.0 * PI;

  // 2. 大臂舵机角 → 查表得肘位置
  float er = shoulderServoToElbowR(shAngle);
  float eh = shoulderServoToElbowH(shAngle);

  // 3. 小臂舵机角 → 小臂绝对角
  float a = forearmServoToAbsAngle(elAngle);

  // 4. 算爪缝中心位置（从肘位置，沿小臂方向延伸 armLen2）
  gripR = er + foreArm * cos(a);
  gripH = eh + foreArm * sin(a);

  // 5. 算笔尖坐标（笔竖直，在爪缝正下方 penDown 处）
  tipX = gripR * cos(t);
  tipY = gripR * sin(t);
  tipZ = gripH - penDown;
}

/*2) 逆运动学：根据目标笔尖坐标，算出三个舵机应该转多少度
      核心是通过粗解和二分法解一个方程零点*/
bool solveIK(float x, float y, float z){
  //1.
  float r = sqrt(x * x + y * y);// 爪缝距离底座转轴的水平距离
  float gh = z + penDown;// 爪缝中心的目标高度

  //2.在大臂舵机限幅中找粗略的结果（精度为1度）
  float bestShServo = shMin, Error = 999;
  for(int i = 0 ; i < (shMax - shMin) ; i++){
    float err = fabs(elbowDistanceTo((float)shAngle+i, r, gh) - foreArm);//不断算残差，并取绝对值
    if(err < Error) {bestShServo = shAngle + i; Error = err;}//利用绝对值判断残差之间的关系，找该精度下残差最小的大臂角
  }

  //3.根据上一步粗解的舵机角，找到准确的大臂舵机角(二分法找零点)
  float dBest = elbowDistanceTo(bestShServo, r, gh) - foreArm;
  float dFormer = elbowDistanceTo(bestShServo - 1.0, r, gh) - foreArm;
  float dNext = elbowDistanceTo(bestShServo + 1.0, r, gh) - foreArm;
  float lowAngle = bestShServo, highAngle = bestShServo;

  //确定二分区间
  if(dFormer * dBest < 0) {lowAngle = bestShServo - 1.0; highAngle = bestShServo;}
  else if(dNext * dBest < 0) {lowAngle = bestShServo; highAngle = bestShServo + 1.0;}
  //进行二分查找，32次更为保险
  for(int i = 0 ; i < 32 ; i++){
    float midAngle = (lowAngle + highAngle) / 2.0;
    float fLow = elbowDistanceTo(lowAngle, r, gh) - foreArm;
    float fMid = elbowDistanceTo(midAngle, r, gh) - foreArm;
    if(fLow * fMid <= 0) highAngle = midAngle;
    else lowAngle = midAngle;
  }
  float shIKAngle = (lowAngle + highAngle) / 2.0;//二分法收尾求中点

  if(fabs(elbowDistanceTo(shIKAngle, r, gh) - foreArm) > reachTolerance) return false;//确保找到了残差最小的大臂舵机角

  float er = shoulderServoToElbowR(shIKAngle);//肘关节位置1
  float eh = shoulderServoToElbowH(shIKAngle);//肘关节位置2
  float a = atan2(gh - eh, r - er) * 180.0 / PI;//小臂绝对角

  float elIKAngle = AbsAngleToforearmServo(a);
  if(elIKAngle < elMin || elIKAngle > elMax) return false;//保证反解角不会超过限幅

  ikBase = baseZero + atan2(y, x) * 180.0 / PI * baseDir;
  ikSh = shIKAngle;
  ikEl = elIKAngle;
  return true;
}

/*========================第4层：实际驱动层=======================*/
//判断逆解是否成功，写舵机
bool moveTipTo(float x, float y, float z){
  if(!solveIK(x,y,z)) return false;
  setAngles(ikBase, ikSh, ikEl, grAngle);
  calcTipPosition();
  return true;
}