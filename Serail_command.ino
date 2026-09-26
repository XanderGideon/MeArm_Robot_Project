void HandleSerial(){
  if(inputComplete){
    inputString.trim();// 去掉回车换行
    if(inputString.length() == 1){
      char cmd = Serial.read();//读取缓冲区的单个字节

      switch(cmd){
        case 'O':
          gripper.write(grMax);
          Serial.println("Gripper:Open");
          break;

        case 'S':
          gripper.write(grMin);
          Serial.println("Gripper:Close");
          break;       

        case 'H':
          motorSpeed = 10;
          Serial.println("MotorSpeed:High");
          break;  

        case 'L':
          motorSpeed = 30;
          Serial.println("MotorSpeed:Low");
          break; 

        case 'A': doGrab(0); break;

        case 'B': doGrab(1); break;
        
        case 'C': doGrab(2); break;

        default:
          break;  
      }                
    }
    else if(inputString.startsWith("x") || inputString.startsWith("X")){
      parseXYZ(inputString);
    }
    else{
      Serial.println("Error!");
    }
    inputString = "";
    inputComplete = false; 
  }
}

void parseXYZ(String str){
  // 目标格式: x90,y90,z90 (假设没有空格)
  int firstComma = str.indexOf(',');
  int secondComma = str.indexOf(',',firstComma+1);

  // 如果两个逗号都存在，说明格式基本正确
  if(firstComma > 0 && secondComma > 0){
    // 提取 x10, y30, z20 中的数字
    int xValue = str.substring(1,firstComma).toInt();
    int yValue = str.substring(firstComma + 2, secondComma).toInt();
    int zValue = str.substring(secondComma + 2).toInt();

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



