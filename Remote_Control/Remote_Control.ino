/*
    测试用：uno板+按键+面包板

    接线（用 UNO 自带的串口发指令给机械臂那块 Arduino）：
     遥控板 D2 -> 按键1 一端，按键另一端接 GND
     遥控板 D3 -> 按键2
     遥控板 D4 -> 按键3
     遥控板 D5 -> 按键4
     遥控板 TX(D1) -> 机械臂板的 RX(D0)  
     遥控板 GND    -> 机械臂板的 GND      

     操作流程：烧录程序时拔传输线，运行时接上
 */

const int key1 = 2;// 循环夹取 A -> B -> C
const int key2 = 3;   // 录制
const int key3 = 4;   // 回放
const int key4 = 5;   // 回中


void setup() {
  Serial.begin(9600);

  //内部上拉：按键一端接面包板gnd，另一端用杜邦线接，和遥控板的母口相接，按下时为低电平
  pinMode(key1, INPUT_PULLUP);
  pinMode(key2, INPUT_PULLUP);
  pinMode(key3, INPUT_PULLUP);
  pinMode(key4, INPUT_PULLUP);

  Serial.println("Remote board ready!");
}

void loop() {
  //采用阻塞式延迟消抖
  if (digitalRead(key1) == LOW) {
    Serial.println("1");
    delay(300);
  }

  if (digitalRead(key2) == LOW) {
    Serial.println("2");
    delay(300);
  }

  if (digitalRead(key3) == LOW) {
    Serial.println("3");
    delay(300);
  }

  if (digitalRead(key4) == LOW) {
    Serial.println("4");
    delay(300);
  }
}
