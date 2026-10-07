/* =====================================================================
     1) clearSerial()       把动作期间堆积的旧按键/旧指令丢掉
     2) HandleSerial()      串口指令分发（唯一的入口，loop 每轮调一次）
     3) SerialCommandXYZ()  x90,y60,z30 这种写法的数字解析
    外加打印函数 printHelp() printSpeed()，以及遥控板长按码 L1~L8。
   ---------------------------------------------------------------------
   串口指令表（9600，每条结尾必须带换行）：
     O            夹爪张开           S            夹爪闭合
     H            任务一摇杆提速      L      任务一摇杆降速
     M            打印这张表                  
     A / B / C    夹取物体 A / B / C
     x90,y60,z30  三个关节一起给角度（字母后面的数；逗号后面不能加空格，
                  也不认 90,60,30 这种不带字母的写法）
     1~8          等于遥控板短按 1~8
     L5~L8        等于遥控板长按 5~8
   ===================================================================== */

// ===== 把串口里堆积的旧指令丢掉 =====
// 一个抓取动作要好几秒，动作过程中按下的按键会堆在串口缓冲区里，
void clearSerial(){
  if(Serial.available() > 0) Serial.read();
  inputString = "";
  inputComplete = false;
}

const char* speedName(int level) {
  if (level <= 0) return "fast";
  if (level == 1) return "normal";
  return "slow";
}

void printHelp() {
  Serial.println(F("----- meArm 全部任务（一 / 二 / 三 / 五),共八个按键 -----"));
  Serial.println(F("按键 1~4(任务一 / 二 / 三）："));
  Serial.println(F("  1=循环夹取 A/B/C      2=录制(再按一次结束)"));
  Serial.println(F("  3=回放上一次录的      4=回中"));
  Serial.println(F("按键 5~8(任务五):"));
  Serial.println(F("短按:5=记一个示教点       6=开始画 / 停下"));
  Serial.println(F("  7=暂停 / 继续        8=取消（停住回待机）"));
  Serial.println(F("长按:5=清空示教点     6=换任务类型"));
  Serial.println(F("      7=换图形         8=打印状态"));
  Serial.println(F("串口:O=张开 S=闭合 H/L=任务一摇杆提速/降速"));
  Serial.println(F("      M=按键表 P=状态 T=自检(后续任务速度固定,不受H/L影响)"));
  Serial.println(F("      x90,y60,z30  -> 三个舵机一起动(只认这一个格式)"));
  Serial.println(F("      A / B / C    -> 夹取物体 A / B / C"));
  Serial.println(F("      1~8 / L1~L8  -> 遥控板短按 / 长按"));
  Serial.println(F("--------------------------------------------"));
}

void printSpeed() {
  Serial.print(F("Speed -> "));
  Serial.print(speedName(speedLevel));
  Serial.print(F("  (joystick delay "));
  Serial.print(motorSpeed);
  Serial.println(F("ms, only task 1)"));
}
// ================= 串口指令分发 =================
void HandleSerial() {
  if(!inputComplete) return;

  inputString.trim(); //去掉回车换行和前后空格

  if(inputString.length() == 0){//如果去掉换行后是空字符串（只按了回车），直接跳过，不报错
    inputString = "";
    inputComplete = false;
    return;
  }

  char cmd = inputString[0];
  int len = inputString.length();

  // ---------------- 单字符指令 ----------------
  if(len == 1){
    switch(cmd){
      case 'O':
        if(autoBusy()){ Serial.println(F("Busy!")); break; }
        setAngles(baseAngle, shAngle, elAngle, grMin);
        Serial.println(F("Gripper:Open")); 
        break;    
      

      case 'S':
        if(autoBusy()){ Serial.println(F("Busy!")); break; }
        setAngles(baseAngle, shAngle, elAngle, grMax);
        Serial.println(F("Gripper:Open")); 
        break;    

      case 'H':
        if(speedLevel > 0) speedLevel--;
        motorSpeed = joyDelayTable[speedLevel];
        printSpeed();
        break;

      case 'L':
        if(speedLevel < speedCount - 1) speedLevel ++;
        motorSpeed = joyDelayTable[speedLevel];
        printSpeed();
        break;

      // ---------- 任务二：上位机发 A / B / C ----------
      case 'A':doGrab(0);break;
      case 'B':doGrab(1);break;
      case 'C':doGrab(2);break;

      // ---------- 任务三 / 五：遥控板短按发过来的就是单个数字 ----------
      case '1': case '2':case '3': case '4':
      case '5': case '6':case '7': case '8':
        doKey(cmd - '0', false);
        break;
      
      // -------------- 打印操作指南 ---------------
      case 'M': printHelp();

      case 'P':
        Serial.println(base.read());
        Serial.println(shoulder.read());
        Serial.println(elbow.read());
        Serial.println(gripper.read());
        break;
        
      default: Serial.println(F("Error!")); break;
    }
  }

  // ------------------ 遥控板长按 --------------------
  else if(len == 2 && cmd == 'L' && inputString[1] >= '5' && inputString[1] <= '8'){
    doKey(inputString[1] - '0', true);
  }

  // ---------------- x90,y60,z30：一条指令给三个角度 ----------------
  else if(cmd == 'x'){
    if(autoBusy()){
      Serial.println(F("Busy!"));
    } 
    else{
      SerialCommandXYZ(inputString);
    } 
  }

  else{
    Serial.println(F("Error!")); // 写法不对或乱码才会走到这里
  }

  inputComplete = false;
  inputString = "";
}

//实现一条指令控制三个舵机
void SerialCommandXYZ(String str) {
  // 目标格式: x90,y90,z90 (假设没有空格)

  //找逗号并返回索引（字符数组/字符串）
  int firstComma = str.indexOf(',');
  int secondComma = str.indexOf(',', firstComma + 1);

  // 如果两个逗号都存在，说明格式基本正确
  if (firstComma > 0 && secondComma > 0) {
    // 提取 x10, y30, z20 中的数字（substring函数范围是左开右闭）
    //toInt把字符串转成整数
    int xValue = str.substring(1, firstComma).toInt();
    int yValue = str.substring(firstComma + 2, secondComma).toInt();  // 多跳了一个字符
    int zValue = str.substring(secondComma + 2).toInt();

    // 写舵机的同时更新 baseAngle/shAngle/elAngle，
    // 否则摇杆那一轮会把刚下发的角度覆盖掉，看起来就是“串口指令没反应”
    setAngles(xValue, yValue, zValue, grAngle);

    Serial.print(F("Executed -> x:"));
    Serial.print(baseAngle);
    Serial.print(F(" y:"));
    Serial.print(shAngle);
    Serial.print(F(" z:"));
    Serial.println(elAngle);
  } else {
    Serial.println(F("Error!"));
  }
}
