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
int grMin   = 20,  grMax   = 90;//爪子

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
int currentgrab = 0;//0A,1B,2C

//任务三相关：录制
#define MAX_RECORDS 200 //一共200个采样点
byte recordData[MAX_RECORDS][4];  //每个点记录四个舵机此时的角度
int recordCount = 0;   //已记录数
bool isRecording = false;  //标志位
unsigned long lastRecordTime = 0;  //用于非阻塞计时
const int recordInterval = 60;  //每60ms记一次时，共0.2*60 = 12s > 10s  

// 一个改变舵机角度的入口，将原本散乱的write函数全部集合于此，改变时只需调用函数
void setAngles(int b, int s, int e, int g){
  baseAngle = constrain(b, baseMin, baseMax);
  shAngle = constrain(s, shMin, shMax);
  elAngle = constrain(e, elMin, elMax);
  grAngle = constrain(g, grMin, grMax);

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

bool autoBusy(){
  return isAutoRunning || isRecording;
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
  if(isRecording){//录制开始
    if(millis() - lastRecordTime >= recordInterval){//非阻塞式定时
      lastRecordTime = millis();
      if(recordCount < MAX_RECORDS){//判断记录次数
        //记录各个舵机的角度
        recordData[recordCount][0] = base.read();
        recordData[recordCount][1] = shoulder.read();
        recordData[recordCount][2] = elbow.read();
        recordData[recordCount][3] = gripper.read();
        recordCount++;        
      }
      if(recordCount >= MAX_RECORDS){// 录满了自动停，并且提示一下
        isRecording = false;
        Serial.println(F("Record buffer is full , Stop Recording"));
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
