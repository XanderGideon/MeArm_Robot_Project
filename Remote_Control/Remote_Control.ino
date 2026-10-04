/* =====================================================================
  模拟遥控板
   ---------------------------------------------------------------------
   接线解法：
     按键1 -> D2        按键5 -> D6
     按键2 -> D3        按键6 -> D7
     按键3 -> D4        按键7 -> D8
     按键4 -> D5        按键8 -> D9
     两块板子的 GND 连在一起；串口接法：遥控板的 TX -> 机械臂板的 RX，

   串口发什么（9600，每条后面带换行）：
     短按（按住不到 0.8 秒）-> "1" ... "8"
     长按（按住 0.8 秒以上）-> "L1" ... "L8"

   机械臂那边对应的功能：
     1=循环夹取   2=录制/结束   3=回放   4=回中
     5=记示教点   6=开始画/停   7=暂停/继续   8=取消
     长按5=清空示教点   长按6=更换子任务
     长按7=换图形（子任务二专属）       长按8=打印状态
   ===================================================================== */

const int keyCount = 8; 
const int keyPin[keyCount] = {2, 3, 4, 5, 6, 7, 8, 9};// 按键 1~8 接的引脚
const unsigned int LongMs = 800;//超过这个时间算长按
const unsigned int shortMs = 30;//低于这个时间算短按

bool keyDown[keyCount];//标志位，记住上一次的状态，是刚按下还是刚松开
unsigned int downTime[keyCount];//记录按下的时刻

void setup(){
  Serial.begin(9600);

  for(int i = 0 ; i < keyCount ; i++){
    pinMode(keyPin[i], INPUT_PULLUP);//内部拉高，按下按键读低电平
    keyDown[i] = false;
  }
}

void sendKey(int n, bool isLong){
  if(isLong) Serial.print('L');//长按就多发一个'L'
  Serial.println(n);//短按就只发按键号码
}

void loop(){
  for(int i = 0 ; i < keyCount ; i++){
    bool nowPress =(digitalRead(keyPin[i]) == LOW);//按下为低电平

    if(nowPress && !keyDown[i]){//按下按键,记录时间
      downTime[i] = millis();
      keyDown[i] = true;
    } else if(!nowPress && keyDown[i]){//松开按键，此时才判断是长按还是短按
      keyDown[i] = false;
      unsigned long pressedTime = millis() - downTime[i]; 
      if(pressedTime >= shortMs){
        // 松开才发指令，所以短按和长按用同一个键就能区分开，
        // 整个过程不阻塞，八个键同时按也不会漏掉哪个
        sendKey(i + 1 , pressedTime >= LongMs);
      }
    }
  }
  delay(5);
}