//// ===== 把串口里堆积的旧指令丢掉 =====
// 一个抓取动作要好几秒，动作过程中按下的按键会堆在串口缓冲区里，
// 动作一结束就被当成新指令执行（看起来就是“莫名其妙又动一下”），所以动作结束后清一次
void clearSerial(){
  while(Serial.available() > 0){
    Serial.read();
  }
  inputString = "";
  inputComplete = false;
}

// ================= 串口指令分发 =================/
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
      case 'O': if (isAutoRunning) { Serial.println("Busy!"); break; } setAngles(baseAngle, shAngle, elAngle, grMin); Serial.println("Gripper:Open"); break;
      case 'S': if (isAutoRunning) { Serial.println("Busy!"); break; } setAngles(baseAngle, shAngle, elAngle, grMax); Serial.println("Gripper:Close"); break;
      case 'H': motorSpeed = 10; Serial.println("MotorSpeed:High"); break;
      case 'L': motorSpeed = 30; Serial.println("MotorSpeed:Low"); break;

      // ---------- 任务二：上位机发 A / B / C ----------
      case 'A': doGrab(0); break;
      case 'B': doGrab(1); break;
      case 'C': doGrab(2); break;

    // ---------- 任务三：遥控板四个按键 ----------
      case '1':
        if(isAutoRunning || isRecording) Serial.println("Busy!"); break;

        Serial.print("Key1 -> object:");
        Serial.println(currentgrab);//0=A, 1=B, 2=C
        doGrab(currentgrab);
        currentgrab++;
        if(currentgrab > 2) currentgrab = 0;
        break;

      case '2':
        if(isAutoRunning) Serial.println("Busy!"); break;
        //第一次按下
        Serial.println("Record Start");
        isRecording = true;
        recordCount = 0;
        lastRecordTime = millis();
        //第二次按下
        if(isRecording == true){
          isRecording = false;
          Serial.println("Record Stop");
        }
        break;

      case '3':
        if(isAutoRunning || isRecording) Serial.println("Busy!"); break;

        isAutoRunning = true;
        Serial.println("Play Start");
        for(int i = 0 ; i < recordCount ; i++){
          setAngles(recordData[i][0],recordData[i][1],
          recordData[i][2],recordData[i][3]);
        }
        Serial.println("play Stop");
        break;

      case '4':
        setAngles(homePose[0], homePose[1], homePose[2], homeGripperAngle);
        break;

      // -------------自检-------------
      case 'T': servoTest(); Serial.println("SelfTest:Done"); break;
      
      default: Serial.println("Error!"); break;
    }
  }
  else if(inputString.startsWith("x") || inputString.startsWith("X")){
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
    int yValue = str.substring(firstComma + 1, secondComma).toInt(); // 多跳了一个字符
    int zValue = str.substring(secondComma + 1).toInt();             

    // 写舵机的同时更新 baseAngle/shAngle/elAngle，
    // 否则摇杆那一轮会把刚下发的角度覆盖掉，看起来就是“串口指令没反应”
    setAngles(xValue, yValue, zValue, grAngle);

    Serial.print("Executed -> x:");
    Serial.print(baseAngle);
    Serial.print(" y:");
    Serial.print(shAngle);
    Serial.print(" z:");
    Serial.println(elAngle);
  }
  else{
    Serial.println("Error!");
  }
}
