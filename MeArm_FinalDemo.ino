#include <Servo.h>

Servo base, shoulder, elbow, gripper;

// 舵机引脚
const int basePin = 9;//底座
const int shoulderPin = 7;//大臂
const int elbowPin = 8;//小臂
const int gripperPin = 6;//夹爪

// 安全角度范围
int baseMin = 0,   baseMax = 180;//底座
int shMin   = 35,  shMax   = 155;//大臂
int elMin   = 25,   elMax   = 155;//小臂
int grMin   = 0,  grMax   = 90;//爪子

//肘部最多能折回來多少
float foldMin = -115.0;

// 当前角度（唯一真值：所有控制方式都改这几个变量）
int baseAngle = 90;
int shAngle   = 90;
int elAngle   = 90;
int grAngle   = 90;

//摇杆引脚
const int joy1X = A0; // 底座
const int joy1Y = A1; // 大臂
const int joy2X = A3; // 小臂
const int joy2Y = A2; // 夹爪

//摇杆回中死区：模拟量偏离512超过这个值才算“真的推了摇杆”
const int joyDeadzone = 60;

//===============速度控制=================
int motorSpeed = 20;//正常速度
const int speedCount = 3;
const int joyDelayTable[speedCount] = {10, 20, 40};//三个摇杆延时档位
int speedLevel = 1;//初始设置为正常速度

//==============回中姿态====================
//全局都用这个来回中，不再单独写
int homePose[3] = {90, 90, 90}; // 底座、大臂、小臂
int homeGripperAngle = 20; //回中时爪子张开，方便衔接

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
/* 1) 机械臂的尺寸，单位 mm，拿尺子量到转轴中心：
      armLen1（大臂：肩关节转轴 -> 肘关节转轴）= 82
      armLen2    小臂：肘关节转轴 -> 爪心（夹笔的那道缝） = 140
      penDown    爪心 -> 笔尖（笔始终竖直）= 68  
      baseHeight 肩关节转轴离桌面的高度 = 82
      baseOffset 底座转轴 -> 肩关节转轴的水平距离  = 20

      本机结构:
    1.小臂舵机装在底座上、用两根长连杆驱动小臂：小臂与水平面绝对角（记a）
      只跟小臂舵机角有关，跟大臂转到哪完全无关。
    2.同一套连杆让爪子始终水平，所以夹在爪缝里的笔永远竖直。
      那么可得正解就是"两连杆 + 末端一段竖直的笔"：
      t1 = (baseAngle - baseZero) * baseDir            // 底座与Z轴的夹角
      t2 = (shAngle - shZero) * shDir                  // 大臂与水平面夹角
      a  = (elAngle - elZero) * elGain                 // 小臂与水平面夹角（与 t2 无关）
      r  = baseOffset + armLen1*cos(t2) + armLen2*cos(a)   // 笔尖离底座转轴的距离
      z  = (baseHeight - penDown) + armLen1*sin(t2) + armLen2*sin(a)  // 笔尖距离桌面的高度

      为什么有 baseOffset：肩舵机装在一个往前偏 20mm 的支架上。
      少了这一项，整机会以为笔尖比真身近 20mm —— 实测
      x90,y155,z90 时尺子量"底座转轴->笔尖"=226mm，不加偏置只能算出 206mm。

      怎么量（都要量到各部位中心）：
      armLen1    = 肩转轴中心 -> 肘转轴中心
      armLen2    = 肘转轴中心 -> 爪缝中心（笔被夹住的那条缝）
      penDown    = 爪缝中心 -> 笔尖，笔夹紧后量竖直距离
      baseHeight = 肩转轴中心 -> 桌面
      baseOffset = 底座转轴心 -> 肩转轴心，水平距离(注意肩关节在底座舵机的后侧)
*/
float armLen1    = 82.0;
float armLen2    = 179.0; //各种姿态下实测出来的结果
float penDown    = 68.0;
float baseHeight = 82.0;
float baseOffset = 20.0; 

/* 2) 舵机角度和数学角度的换算：
      baseZero：机械臂朝前方时的底座舵机角度
      shZero：是大臂与水平面平行时的大臂舵机角度
      elZero：是小臂与水平面平行时的小臂舵机角度
      elGain：是实测多次后拟合的传动比
      大臂：t2 = (shAngle - shZero) * shDir            —— 零位 + 方向
      小臂：a  = (elAngle - elZero) * elGain           —— 零位 + 连杆传动比（涵盖方向）
*/
float baseZero = 90.0,  baseDir =  1.0;   // 底座：数学角从 +x 轴逆时针量
float shZero   = 180.0, shDir   = -1.0;   // 大臂：数学角从水平面逆时针量
float elZero   = 105.0, elGain  =  0.89;  // 小臂：绝对角 = (elAngle - elZero) * elGain
float elbowSign = -1; //解决机械臂在物理层面的多解问题，elbowSign = -1 说明取肘部在上的解

/* 3) 抬笔高度 单位 mm */
float liftZ  = 15.0;     // 抬笔画线的时候抬多高

/* 4) 纸放在哪：纸面坐标的原点 (0,0) 在机械臂坐标里的位置 也就是纸面中心和底座舵机转轴所在竖线距离为187mm
      此处定义为纸面中心在绝对参考系之中的坐标*/
float paperX = 187.0;
float paperY = 0.0;
float paperZ = 0.0;      

// ===================== 正运动学算出来的笔尖位置 =====================
//此处定义为笔尖在绝对参考系之中的坐标
float tipX = 0.0, tipY = 0.0, tipZ = 0.0;

// ===================== 逆运动学算出来的舵机角度 =====================
int ikBase = 90, ikSh = 90, ikEl = 90;

//任务五绘图相关变量
#define  MAX_POINTS 5

// 一个改变舵机角度的入口，将原本散乱的write函数全部集合于此，改变时只需调用函数
void setAngles(int b, int s, int e, int g){
  baseAngle = constrain(b, baseMin, baseMax);
  shAngle = constrain(s, shMin, shMax);
  elAngle = constrain(e, elMin, elMax);
  grAngle = constrain(g, grMin, grMax);

  //防止自撞
  int _t2 = (shAngle - shZero) * shDir;
  int _alpha = (elAngle - elZero) * elGain;

  if(_alpha - _t2 < foldMin) {
    elAngle = constrain(roundToInt(elZero + (_t2 + foldMin) / elGain), elMin, elMax);
  }

  base.write(baseAngle);
  shoulder.write(shAngle);
  elbow.write(elAngle);
  gripper.write(grAngle);
}

// ===== 按 H / L 设定的整体速度等待 =====
// motorSpeed = 20 是正常速度，10 是快，30 是慢；自动动作也跟着这个速度变
void waitMs(int baseMs){
  delay(baseMs);
}

// 摇杆输入 -> 增量式改变角度
//不用以前的绝对位置映射，保证只在摇杆移动时设置新角度
void updateJoysticks(){
  if(isAutoRunning) return;

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
  if(!isAutoRunning) updateJoysticks();

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
