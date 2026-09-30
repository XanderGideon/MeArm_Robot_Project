void HandleSerial(){
  if(inputComplete){
    inputString.trim(); // 去掉回车换行
    
    // 【修复1】如果去掉换行后是空字符串（只按了回车），直接跳过，不报错
    if (inputString.length() == 0) {
      inputComplete = false;
      return;
    }

    if(inputString.length() == 1){
      // 【修复2】关键！不要再从 Serial.read() 读了，直接从字符串里取第一个字符
      char cmd = inputString[0]; 

      switch(cmd){
        case 'O': gripper.write(0); Serial.println("Gripper:Open"); break;
        case 'S': gripper.write(90); Serial.println("Gripper:Close"); break;       
        case 'H': motorSpeed = 10; Serial.println("MotorSpeed:High"); break;  
        case 'L': motorSpeed = 30; Serial.println("MotorSpeed:Low"); break; 
        case 'A': doGrab(0); break;
        case 'B': doGrab(1); break;
        case 'C': doGrab(2); break;
        default: break;  
      }              
    }
    else if(inputString.startsWith("x") || inputString.startsWith("X")){
      parseXYZ(inputString);
    }
    else{
      Serial.println("Error!"); // 只有真正乱码的指令才会走到这里
    }
    inputString = "";
    inputComplete = false; 
  }
}

//实现一条指令控制三个舵机
void parseXYZ(String str){
  // 目标格式: x90,y90,z90 (假设没有空格)

  //找逗号并返回索引（字符数组/字符串）
  int firstComma = str.indexOf(',');
  int secondComma = str.indexOf(',',firstComma+1);

  // 如果两个逗号都存在，说明格式基本正确
  if(firstComma > 0 && secondComma > 0){
    // 提取 x10, y30, z20 中的数字（substring函数范围是左开右闭）
    //toInt把字符串转成整数
    int xValue = str.substring(1,firstComma).toInt();
    int yValue = str.substring(firstComma + 2, secondComma).toInt();
    int zValue = str.substring(secondComma + 2).toInt();

    //限制安全范围
    int xVal = constrain(xValue, baseMin, baseMax);
    int yVal = constrain(yValue, shMin, shMax);
    int zVal = constrain(zValue, elMin, elMax);

    base.write(xVal);
    shoulder.write(yVal);
    elbow.write(zVal);

    Serial.print("Executed -> x:");
    Serial.print(xVal);
    Serial.print(" y:");
    Serial.print(yVal);
    Serial.print(" z:");
    Serial.println(zVal);    
  }
  else{
    Serial.println("Error!");
  }
}



