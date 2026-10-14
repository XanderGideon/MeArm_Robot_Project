#include <Servo.h>

Servo base, shoulder, elbow, gripper;

// 舵机引脚
const int basePin = 9;//底座
const int shoulderPin = 7;//大臂
const int elbowPin = 8;//小臂
const int gripperPin = 6;//夹爪

// 安全角度范围
int baseMin = 0,   baseMax = 180;//底座
int shMin   = 80,  shMax   = 155;//大臂
int elMin   = 35,   elMax   = 135;//小臂
int grMin   = 80,  grMax   = 180;//爪子

//肘部最多能折回來多少
float foldMin = -120.0;

// 当前角度（唯一真值：所有控制方式都改这几个变量）
int baseAngle = 90;
int shAngle   = 90;
int elAngle   = 90;
int grAngle   = 145;//上电即可夹笔

//摇杆引脚
const int joy1X = A0; // 底座
const int joy1Y = A1; // 大臂
const int joy2X = A3; // 小臂
const int joy2Y = A2; // 夹爪

//摇杆回中死区：模拟量偏离512超过这个值才算“真的推了摇杆”
const int joyDeadzone = 60;

//===============速度控制=================
int motorSpeed = 30;//正常速度
const int speedCount = 3;
const int joyDelayTable[speedCount] = {10, 30, 50};//三个摇杆延时档位
int speedLevel = 1;//初始设置为正常速度

//==============回中姿态====================
//全局都用这个来回中，不再单独写
int homePose[3] = {90, 80, 100}; // 底座、大臂、小臂
int homeGripperAngle = 80; //回中时爪子张开，方便衔接

//串口接收缓冲区与标志位
String inputString = "";
bool inputComplete = false;

//自动任务开关状态标志
bool isAutoRunning = false;

//任务三相关：循环抓取计数器cnt
int currentgrab = 0;//0 = A,1 = B,2 = C

//任务三相关：录制
#define MAX_RECORDS 200      //一共200个采样点
byte recordData[MAX_RECORDS][4];   //每个点记录四个舵机此时的角度
int recordCount = 0;               //已记录数
bool isRecording = false;          //标志位
unsigned long lastRecordTime = 0;  //用于非阻塞计时
const int recordInterval = 60;     //每60ms记一次，共 200*60ms = 12s > 10s

//任务五： 画图标定区 
/*1）"小臂"+笔相关*/
float foreArm = 116.6; //实际测算得出轴转轴到爪缝中心距离为116.6mm
float penDown = 70.0; //笔尖到爪缝的距离
float penPress = 1.5; // 笔尖可在纸面上下压

/* 2) 底座舵机的零度角与方向*/
float baseZero = 90.0,  baseDir =  1.0;   // 底座：数学角从 +x 轴逆时针量

/* 3) 大臂舵机角度+轴转轴相关查表 */
#define elbowJointPoints 5
float shoulderServoAngles[elbowJointPoints]= {95.0, 110.0, 125.0, 140.0, 155.0};
float elbowJointR[elbowJointPoints] = {39.6,  52.6,  72.6,  90.6,  95.6};
float elbowJointH[elbowJointPoints] = {114.8, 107.8,  95.8,  75.8,  59.8};

/* 4) 等效大臂长+拟合肩关节位置*/
float shoulderJointR = 20.1;
float shoulderJointH = 36.7;
//float upperArm = 79.5;

/* 5) 小臂传动比 */
float elbowServoNode = 90.0;    // 临界点小臂舵机角度
float foreArmAbsAngle = -7.5;    // 临界点小臂舵机角度90° 时的绝对角
float armGainDown = 0.883;   // 下段斜率
float armGainUp = 0.817;   // 上段斜率

/* 6) 抬笔高度 单位 mm */
float liftZ  = 12.0;     // 抬笔画线的时候抬多高

/* 7) 纸放在哪：纸面坐标的原点 (0,0) 在机械臂坐标里的位置 也就是纸面中心和底座舵机转轴所在竖线距离为185.0mm
      此处定义为纸面中心在绝对参考系之中的坐标*/
float paperX = 185.0;
float paperY = 0.0;
float paperZ = 0.0;      

/* 8) 爪子相关参数 */
float gripR = 0.0, gripH = 0.0;//爪子距离底座舵机转轴的水平距离与竖直距离

/* 9) 判断能否到达远端的容忍误差，残差超过这个容忍误差，返回false */
float reachTolerance = 0.6;

// ===================== 正运动学算出来的笔尖位置 =====================
//此处定义为笔尖在绝对参考系之中的坐标
float tipX = 0.0, tipY = 0.0, tipZ = 0.0;

// ===================== 逆运动学算出来的舵机角度 =====================
float ikBase, ikSh, ikEl;

//=======================任务五绘图状态相关变量========================
#define  MAX_POINTS 5  //最多5个示教点
int taskType = 0; // 0=直线， 1=图形， 2=折线， 3=曲线
int drawState = 0; //当前任务进行状态 0=正在待机 1=回到待机状态 2=抬笔去第一个点 3=落笔开始画
bool paused = false;
int nowStepTime = 80;
int connerWaitTime = 500; //拐角等待时间加长，完成回差补偿
int drawStepTime = 80; //每画一步进行等待
unsigned long lastStepTime = 0; //画图状态机的节拍(按键取消画图状态时需要复位)
int teachCount = 0;

//精确输出脉冲，实现更精确写舵机角度
float p = (2400.0 - 544.0) / 180.0;
void writeServoAngle(Servo *servo, float angle){
  if(angle < 0.0) angle = 0.0;
  if(angle > 180.0) angle = 180.0;
  servo -> writeMicroseconds((int)(544.0 + angle * p + 0.5));//加0.5防止int四舍五入截断
}

// 一个改变舵机角度的入口，将原本散乱的write函数全部集合于此，改变时只需调用函数
void setAngles(int b, int s, int e, int g){
  baseAngle = constrain(b, baseMin, baseMax);
  shAngle = constrain(s, shMin, shMax);
  elAngle = constrain(e, elMin, elMax);
  grAngle = constrain(g, grMin, grMax);

  //防止自撞(计算小臂绝对角与大臂虚拟)
  float upperArmTheta = upperArmServoToAbsAngle(shAngle);
  float foreArmTheta = forearmServoToAbsAngle(elAngle);
  if(foreArmTheta - upperArmTheta < foldMin){
    elAngle = constrain(AbsAngleToforearmServo(upperArmTheta + foldMin), (int)elMin, (int)elMax);
  }

  writeServoAngle(&base, baseAngle);
  writeServoAngle(&shoulder, shAngle);
  writeServoAngle(&elbow, elAngle);  
  writeServoAngle(&gripper, grAngle);
}

// ===== 按 H / L 设定的整体速度等待 =====
// motorSpeed = 30 是正常速度，10 是快，50 是慢；自动动作也跟着这个速度变
void waitMs(int baseMs){
  delay(baseMs);
}

// 摇杆输入 -> 增量式改变角度
//不用以前的绝对位置映射，保证只在摇杆移动时设置新角度
void updateJoysticks(){
  if(isAutoRunning || drawState != 0) return;

  int v1x = analogRead(joy1X);
  int v1y = analogRead(joy1Y);
  int v2x = analogRead(joy2X);
  int v2y = analogRead(joy2Y);

  bool moved = false;
  int b = baseAngle, s = shAngle, e = elAngle, g = grAngle;
  
  // 每个轴：(摇杆值 - 512) / 100 => 死区外每 100 个模拟量变化约 1 度，
  // 由于舵机硬件存在客观问题，故而设置死区
  if(v1x < 512 - joyDeadzone || v1x > 512 + joyDeadzone) {b += (512 - v1x)/100; moved = true;}
  if(v1y < 512 - joyDeadzone || v1y > 512 + joyDeadzone) {s += (512 - v1y)/100; moved = true;}
  if(v2x < 512 - joyDeadzone || v2x > 512 + joyDeadzone) {e += (512 - v2x)/100; moved = true;}
  if(v2y < 512 - joyDeadzone || v2y > 512 + joyDeadzone) {g += (512 - v2y)/100; moved = true;}

  if(moved){
    setAngles(b, s, e, g);
    calcTipPosition();
  }
}

void setup() {
  //开启串口
  Serial.begin(9600);
  Serial.println(F("System Ready! ")); // 开机提示

  //缓冲区预留内存
  inputString.reserve(64);

  //连接舵机
  base.attach(basePin);
  shoulder.attach(shoulderPin);
  elbow.attach(elbowPin);
  gripper.attach(gripperPin);

  //设置速度
  motorSpeed = joyDelayTable[speedLevel];
  // 初始姿态，统一设置角度
  setAngles(baseAngle, shAngle, elAngle, grAngle);

  //开机打印
  Serial.println("System is Ready!");
  printHelp();
}

void loop() {
  HandleSerial();   //接收串口cmd

  //============任务三：录制采样============
  if(isRecording){
    if(millis() - lastRecordTime >= recordInterval){
      lastRecordTime = millis();
      if(recordCount < MAX_RECORDS){
        recordData[recordCount][0] = base.read();
        recordData[recordCount][1] = shoulder.read();
        recordData[recordCount][2] = elbow.read();
        recordData[recordCount][3] = gripper.read();
        recordCount++;
      }
      if(recordCount >= MAX_RECORDS){
        isRecording = false;
        printRecordFull();
      }
    }
  }

  // 只有在没有自动任务时，摇杆才控制舵机
  //录制时必须用摇杆,故而不判断isRecording
  if(!isAutoRunning && drawState == 0) updateJoysticks();

  tickDraw();

  delay(motorSpeed); // 小延时，保证摇杆响应流畅
}

// ================= 串口接收中断（每轮 loop 之间自动调用）=================
void serialEvent() {
  while (Serial.available() > 0) {
    char ch = (char)Serial.read();
    inputString += ch;
    if (ch == '\n') {
      inputComplete = true;
    }
  }
}
