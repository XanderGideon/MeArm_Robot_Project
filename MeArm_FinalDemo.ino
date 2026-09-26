#include <Servo.h>

Servo base, shoulder, elbow, gripper;

// 舵机引脚
const int basePin = 9;
const int shoulderPin = 10;
const int elbowPin = 11;
const int gripperPin = 6;

//全局变量：控制机械臂整体运行速度
int motorSpeed = 20;

//串口接收缓冲区与标志位
String inputString = "";
bool inputComplete = false;
// 摇杆引脚
const int joy1X = A0; // 底座
const int joy1Y = A1; // 大臂
const int joy2X = A2; // 小臂
const int joy2Y = A3; // 夹爪

// 安全角度范围
int baseMin = 0,   baseMax = 180;
int shMin   = 15,  shMax   = 165;
int elMin   = 0,   elMax   = 180;
int grMin   = 20,  grMax   = 90;

// 当前角度
int baseAngle = 90;
int shAngle   = 90;
int elAngle   = 90;
int grAngle   = 45;

void setup() {
  //开启串口
  Serial.begin(9600);

  //预留内存
  inputString.reserve(200);

  //连接舵机
  base.attach(basePin);
  shoulder.attach(shoulderPin);
  elbow.attach(elbowPin);
  gripper.attach(gripperPin);

  // 初始姿态
  base.write(baseAngle);
  shoulder.write(shAngle);
  elbow.write(elAngle);
  gripper.write(grAngle);

  Serial.begin(9600);
  
  //测试限幅
  testLimits();
}

void loop() {
  HandleSerial();//接收串口cmd

  // 读取摇杆值
  int v1x = analogRead(joy1X);
  int v1y = analogRead(joy1Y);
  int v2x = analogRead(joy2X);
  int v2y = analogRead(joy2Y);

  // 映射到角度，并限制在安全范围内
  baseAngle = map(v1x, 0, 1023, baseMin, baseMax);
  shAngle   = map(v1y, 0, 1023, shMin, shMax);
  elAngle   = map(v2x, 0, 1023, elMin, elMax);
  grAngle   = map(v2y, 0, 1023, grMin, grMax);

  // 写入舵机
  base.write(baseAngle);
  shoulder.write(shAngle);
  elbow.write(elAngle);
  gripper.write(grAngle);

  //调试打印
  Serial.print("Base:"); Serial.print(baseAngle);
  Serial.print(" Sh:"); Serial.print(shAngle);
  Serial.print(" El:"); Serial.print(elAngle);
  Serial.print(" Gr:"); Serial.println(grAngle);

  delay(motorSpeed); // 小延时，保证摇杆响应流畅
}

void SerialEvent(){
  while(Serial.available()){
    char ch = (char)Serial.read() ;
    inputString += ch;
    if(ch == '\n'){
      inputComplete = true;
    }
  }
}

