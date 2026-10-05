//// ===== 把串口里堆积的旧指令丢掉 =====
// 一个抓取动作要好几秒，动作过程中按下的按键会堆在串口缓冲区里，
void clearSerial(){
  while(Serial.available() > 0){
    Serial.read();
  }
  inputString = "";
  inputComplete = false;
}

const char* speedName(int level){
  if(level <= 0) return "fast";
  if(level == 1) return "normal";
  return "slow";
}

void printHelp(){
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

void printSpeed(){
  Serial.print(F("Speed -> "));
  Serial.print(speedName(speedLevel));
  Serial.print(F("  (joystick delay "));
  Serial.print(motorSpeed);
  Serial.println(F("ms, only task 1)"));  
}
// ================= 串口指令分发 =================
void HandleSerial(){
  if(!inputComplete) return;

  inputString.trim(); // 去掉回车换行

  if (inputString.length() == 0) {//如果去掉换行后是空字符串（只按了回车），直接跳过，不报错
    inputComplete = false;
    return;
  }

  if(inputString.length() == 1){
    char cmd = inputString[0]; //从字符串里取第一个字符
    
    switch(cmd){
      // ------------- 任务一：固定指令 ------------
      case 'O': if (isAutoRunning) { Serial.println(F("Busy!")); break; } setAngles(baseAngle, shAngle, elAngle, grMin); Serial.println("Gripper:Open"); break;
      case 'S': if (isAutoRunning) { Serial.println(F("Busy!")); break; } setAngles(baseAngle, shAngle, elAngle, grMax); Serial.println("Gripper:Close"); break;
      case 'H': 
        if(speedLevel > 0) speedLevel--;
        motorSpeed = speedTable[speedLevel]; 
        Serial.println(F("MotorSpeed:High")); 
        break;
      case 'L': 
        if(speedLevel < speedCount - 1) speedLevel++;
        motorSpeed = speedTable[speedLevel]; 
        Serial.println(F("MotorSpeed:Low")); 
        break;

      // ---------- 任务二：上位机发 A / B / C ----------
      case 'A': doGrab(0); break;
      case 'B': doGrab(1); break;
      case 'C': doGrab(2); break;

    // ---------- 任务三：遥控板四个按键 ----------
      case '1'://循环夹取ABC三个物体
        if(isAutoRunning || isRecording) {Serial.println(F("Busy!"));break;}

        Serial.print(F("Key1 Pressed -> object:"));
        Serial.println(currentgrab);//0=A, 1=B, 2=C
        doGrab(currentgrab);
        currentgrab++;
        if(currentgrab > 2) currentgrab = 0;
        break;

      case '2'://录制
        if(isAutoRunning){Serial.println(F("Busy!"));break;}
        //开始录制
        if(isRecording == false){
          isRecording = true;
          recordCount = 0;
          lastRecordTime = millis();
          Serial.println(F("Record Start"));
        }
        //结束录制
        else {
          isRecording = false;
          Serial.print("Record:STOP  points=");
          Serial.print(recordCount);
          Serial.print("  time=");
          Serial.print((long)recordCount * recordInterval / 1000);
          Serial.println(" s");
          // 考核要求录制时长必须大于 10 秒，不够就当场提醒，别等演示完才发现
          if ((long)recordCount * recordInterval < 10000) {
            Serial.println("WARNING: time <= 10s, need >10s !");
          }
        }
        break;

      case '3'://播放
        if(isAutoRunning || isRecording){Serial.println(F("Busy!"));break;}

        isAutoRunning = true;
        Serial.println(F("Play Start"));
        for(int i = 0 ; i < recordCount ; i++){
          setAngles(recordData[i][0],recordData[i][1],recordData[i][2],recordData[i][3]);
          delay(recordInterval);
        }
        isAutoRunning = false;
        clearSerial();
        Serial.println(F("play Stop"));
        break;

      case '4'://回中
        setAngles(homePose[0], homePose[1], homePose[2], homeGripperAngle);
        break;

      // -------------自检-------------
      case 'T': servoTest(); Serial.println(F("SelfTest:Done")); break;
      
      default: Serial.println(F("Error!")); break;
    }
  }
  else if(inputString.startsWith("x") || inputString.startsWith("X")){
    if (isAutoRunning || isRecording) {
      Serial.println(F("Busy!"));
      return;
    }
    SerialCommandXYZ(inputString);
  }
  else{
    Serial.println("Error!"); // 只有真正乱码的指令才会走到这里
  }
  inputString = ""; //清除接收区
  inputComplete = false;
  
}

//实现一条指令控制三个舵机
void SerialCommandXYZ(String str){
  // 目标格式: x90,y90,z90 (假设没有空格)

  //找逗号并返回索引（字符数组/字符串）
  int firstComma = str.indexOf(',');
  int secondComma = str.indexOf(',', firstComma + 1);

  // 如果两个逗号都存在，说明格式基本正确
  if(firstComma > 0 && secondComma > 0){
    // 提取 x10, y30, z20 中的数字（substring函数范围是左开右闭）
    //toInt把字符串转成整数
    int xValue = str.substring(1, firstComma).toInt();
    int yValue = str.substring(firstComma + 2, secondComma).toInt(); // 多跳了一个字符
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
  }
  else{
    Serial.println(F("Error!"));
  }
}








