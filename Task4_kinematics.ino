/*
  第 4 层：驱动层                              
    moveTipTo()           逆解 + 写舵机      

                    ↑ 调用

  第 3 层：运动学层                            
    calcTipPosition()     正解（角度→坐标）  
    solveIK()             逆解（坐标→角度）  

                    ↑ 调用

  第 2 层：传动换算层                          
    armAbsoluteAngle()       舵机角→绝对角   
    elbowServoForArmAngle()  绝对角→舵机角   
    bigArmAngleFor()         等效大臂角       

                    ↑ 调用

  第 1 层：查表工具层                          
    elbowRadiusAt()       大臂角→肘水平距离  
    elbowHeightAt()       大臂角→肘高度      
    elbowDistanceTo()     肘到目标点的距离   

*/

/*========================第1层：查表工具层========================*/
/*1) 输入大臂舵机角 y,返回肘转轴离底座转轴的水平距离 er*/
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

/*2) 输入大臂舵机角 y，返回肘转轴离桌面的高度 eh*/
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

/*3) 输入大臂角 y 和目标点 (r, h)，返回肘到目标点的直线距离h*/
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

/*3) 输入大臂舵机角度，返回等效大臂绝对角*/
float shoulderServoToAbsAngle(float ShAngle){
  float ddr = shoulderServoToElbowR(ShAngle) - shoulderJointR;
  float ddh = shoulderServoToElbowH(ShAngle) - shoulderJointH;
}

/*========================第3层：运动学层=======================*/
/*1) 正运动学：根据三个舵机当前角度，算出笔尖坐标 (tipX, tipY, tipZ)*/
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

/*2) 逆运动学：根据目标笔尖坐标，算出三个舵机应该转多少度*/
bool solveIK(float x, float y, float z){
  float r = sqrt(x * x + y * y);// 水平距离
  float gh = z + penDown;// 爪缝中心的目标高度

  float bestShServo = shMin, Error = 999;
  for(int i = 0 ; i < (shMax - shMin) ; i++){
    float err = fabs(elbowDistanceTo((float)shAngle+i, r, gh) - foreArm);
    if(err < Error) {bestShServo = shAngle + i; Error = err;}
  }

  float dBest = (bestShServo, r, gh) - foreArm;
  float dFormer = elbowDistanceTo(bestShServo - 1.0, r, gh) - foreArm;
  float dNext = elbowDistanceTo(bestShServo + 1.0, r, gh) - foreArm;
  float low = bestShServo, high = bestShServo;

  if(dFormer * dBest < 0) {low = bestShServo - 1.0; high = bestShServo;}
  else if(dNext * dBest < 0) {low = bestShServo; high = bestShServo + 1.0;}
  for(int i = 0 ; i < 32 ; i++){
    float mid = (low + high) / 2.0;
    float fLow = elbowDistanceTo(low, r, gh) - foreArm;
    float fMid = elbowDistanceTo(mid, r, gh) - foreArm;
    if(fLow * fMid <= 0) high = mid;
    else low = mid;
  }
  float shIKAngle = (low + high) / 2.0;

  if(fabs(elbowDistanceTo(shIKAngle, r, gh) - foreArm) > reachTolerance) return false;

  float er = shoulderServoToElbowR(shIKAngle);
  float eh = shoulderServoToElbowH(shIKAngle);
  float a = atan2(gh - eh, r - er) * 180.0 / PI;
  float elIKAngle = AbsAngleToforearmServo(a);
  if(elIKAngle < elMin || elIKAngle > elMax) return false;

  ikBase = baseZero + atan2(y, x) * 180.0 / PI * baseDir;
  ikSh = shIKAngle;
  ikEl = elIKAngle;

  return true;
}

//让笔尖走到某个位置
bool moveToTip(float x, float y, float z){
  if(!solveIK(x,y,z)) return false;
  setAngles(ikBase, ikSh, ikEl, grAngle);
  calcTipPosition();
  return true;
}