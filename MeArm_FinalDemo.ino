/* =====================================================================
   meArm 机械臂 —— 二面（算法方向）任务一 / 任务二 / 任务三   完整程序
   ---------------------------------------------------------------------
   硬件：Arduino UNO + meArm（4 个舵机）+ 2 个摇杆
         串口那一头接：上位机（电脑）  或者  自制按键遥控板
   ---------------------------------------------------------------------
   串口指令表（波特率 9600，每条指令结尾必须带“换行”）
     O             爪子张开
     S             爪子关闭
     H             整体运行速度变快
     L             整体运行速度变慢
     x90,y60,z30   一条指令同时控制 3 个舵机（x=底座 y=大臂 z=小臂）
                   也可以直接写 90,60,30；字母大小写都认
     A / B / C     夹取物体 A / B / C 并放到各自的位置
     1             按键 1：循环夹取（A -> B -> C -> A ...）
     2             按键 2：录制（第一次按下开始，第二次按下结束）
     3             按键 3：回放上一次录制的动作
     4             按键 4：回中
     T             自检：四个舵机一个一个扫一遍
   ---------------------------------------------------------------------
   注意：下面标了 ★ 的几张表都是测试值，上机以后按实际情况改表就行，
         程序逻辑不用动。
   ===================================================================== */

#include <Servo.h>

Servo base, shoulder, elbow, gripper;

// 舵机引脚
const int basePin = 9;//底座
const int shoulderPin = 7;//大臂
const int elbowPin = 8;//小臂
const int gripperPin = 6;//夹爪

//全局变量：控制机械臂整体运行速度
int motorSpeed = 20;

//串口接收缓冲区与标志位
String inputString = "";
bool inputComplete = false;

//摇杆引脚
const int joy1X = A0; // 底座
const int joy1Y = A1; // 大臂
const int joy2X = A2; // 小臂
const int joy2Y = A3; // 夹爪

//摇杆回中死区：模拟量偏离512超过这个值才算“真的推了摇杆”
const int JOY_DEADZONE = 60;

// 安全角度范围
int baseMin = 25,   baseMax = 170;//底座
int shMin   = 35,  shMax   = 155;//大臂
int elMin   = 25,   elMax   = 155;//小臂
int grMin   = 20,  grMax   = 90;//爪子

// 当前角度（唯一真值：所有控制方式都改这几个变量）
int baseAngle = 90;
int shAngle   = 90;
int elAngle   = 90;
int grAngle   = 90;

//自动任务开关状态标志
bool isAutoRunning = false;

//任务三相关：循环抓取计数器cnt
int currentgrab = 0;//0A,1B,2C

//任务三相关：录制
#define MAX_RECORDS 250 //一共250个采样点
byte recordData[MAX_RECORDS][4];  //每个点记录四个舵机此时的角度
int recordCount = 0;   //已记录数
bool isRecording = false;  //标志位
unsigned long lastRecordTime = 0;  //用于非阻塞计时
const int recordInterval = 60;  //每60ms记一次时，共0.25*60 = 15s > 10s  

// ================= 统一写舵机：写的同时记住角度 =================
/* 【修复核心】以前是“每一轮 loop 都拿摇杆的绝对位置直接覆盖舵机角度”，
   所以：1)摇杆一松手(回中)舵机立刻回到 90°；2)串口刚下发的角度马上被覆盖掉。
   现在改成：摇杆只在被推动时小步改变角度，松手后保持不动。 */
void setAngles(int b, int s, int e, int g) {
  baseAngle = constrain(b, baseMin, baseMax);
  shAngle   = constrain(s, shMin,   shMax);
  elAngle   = constrain(e, elMin,   elMax);
  grAngle   = constrain(g, grMin,   grMax);

  base.write(baseAngle);
  shoulder.write(shAngle);
  elbow.write(elAngle);
  gripper.write(grAngle);
}

// ===== 按 H / L 设定的整体速度等待 =====
// motorSpeed = 20 是正常速度，10 是快，30 是慢；自动动作也跟着这个速度变
void waitMs(int baseMs){
  delay((long)baseMs * motorSpeed /20);
}

// 摇杆输入 -> 增量式改变角度
//不用以前的绝对位置映射，保证只在摇杆移动时设置新角度
void updateJoysticks() {
  if (isAutoRunning) return;
  
  //非自动任务时进入
  int v1x = analogRead(joy1X);
  int v1y = analogRead(joy1Y);
  int v2x = analogRead(joy2X);
  int v2y = analogRead(joy2Y);

  bool moved = false;
  int b = baseAngle, s = shAngle, e = elAngle, g = grAngle;

  // 每个轴：(摇杆值 - 512) / 100 => 死区外每 100 个模拟量变化约 1 度，由于舵机硬件存在客观问题，故而设置死区
  if (v1x < 512 - JOY_DEADZONE || v1x > 512 + JOY_DEADZONE) { b += (v1x - 512) / 100; moved = true; }
  if (v1y < 512 - JOY_DEADZONE || v1y > 512 + JOY_DEADZONE) { s += (v1y - 512) / 100; moved = true; }
  if (v2x < 512 - JOY_DEADZONE || v2x > 512 + JOY_DEADZONE) { e += (v2x - 512) / 100; moved = true; }
  if (v2y < 512 - JOY_DEADZONE || v2y > 512 + JOY_DEADZONE) { g += (v2y - 512) / 100; moved = true; }

  // 摇杆在中间(松手)时什么都不写 -> 舵机保持在当前角度
  if (moved) setAngles(b, s, e, g);
}

// ================= 自检：逐个舵机扫一遍，定位硬件问题 =================
/* 串口发 T 触发。请仔细听/看，日志里每一个舵机扫完都会停顿 1 秒。
   - 某个舵机完全不动 / 嘀嘀响 / 抖动 => 供电不足或接线/舵机坏
   - 两三个一起动或一起卡 => 电源带不动（USB 5V 供电是常见原因） */
void servoTest() {
  Serial.println(F("=== SELF TEST START ==="));
  Serial.println(F(">> BASE pin9"));
  for (int a = 0; a <= 180; a += 30) { base.write(a);     delay(300); Serial.print(F("base="));     Serial.println(a); }
  delay(1000);

  Serial.println(F(">> SHOULDER pin7"));
  for (int a = 15; a <= 165; a += 30) { shoulder.write(a); delay(300); Serial.print(F("shoulder=")); Serial.println(a); }
  delay(1000);

  Serial.println(F(">> ELBOW pin8"));
  for (int a = 0; a <= 180; a += 30) { elbow.write(a);    delay(300); Serial.print(F("elbow="));    Serial.println(a); }
  delay(1000);

  Serial.println(F(">> GRIPPER pin6"));
  for (int a = 20; a <= 90; a += 15) { gripper.write(a);  delay(300); Serial.print(F("gripper="));  Serial.println(a); }
  delay(1000);

  setAngles(90, 90, 90, 90);
  Serial.println(F("=== SELF TEST END, back to 90/90/90/90 ==="));
}

void setup() {
  //开启串口
  Serial.begin(9600);
  Serial.println("System Ready! "); // 开机提示

  //缓冲区预留内存
  inputString.reserve(200);

  //连接舵机
  base.attach(basePin);
  shoulder.attach(shoulderPin);
  elbow.attach(elbowPin);
  gripper.attach(gripperPin);

  // 初始姿态，统一设置角度
  setAngles(baseAngle, shAngle, elAngle, grAngle);
}

void loop() {
  HandleSerial();   //接收串口cmd

  if(isRecording){
    if(millis() - lastRecordTime >= recordInterval){//非阻塞式定时
      lastRecordTime = millis();
      if(recordCount < MAX_RECORDS){
        recordData[recordCount][0] = base.read();
        recordData[recordCount][1] = shoulder.read();
        recordData[recordCount][2] = elbow.read();
        recordData[recordCount][3] = gripper.read();
        recordCount++;        
      }
    }
  }

  // 只有在没有自动任务时，摇杆才控制舵机
  if(!isAutoRunning) updateJoysticks();

  delay(motorSpeed); // 小延时，保证摇杆响应流畅
}

void serialEvent() {
  while (Serial.available() > 0) {
    char ch = (char)Serial.read();
    inputString += ch;
    if (ch == '\n') {
      inputComplete = true;
    }
  }
}
