/*每个物体只记录两个位置：抓取点，放置点
格式为{抓取base，抓取shoulder，抓取elbow，
放置base，放置shoulder，放置elbow}*/

//----以下代码为测试版本------
//位置与角度均为测试用

int objectPos[3][6] = {
  {45, 130, 80,   135, 130, 80},  // 物体A：抓取点(45,130,80)，放置点(135,130,80)
  {90, 130, 80,   160, 130, 80},  // 物体B：抓取点(90,130,80)，放置点(160,130,80)
  {135, 130, 80,  45, 130, 80},   // 物体C：抓取点(135,130,80)，放置点(45,130,80)
};

//三个物体不同夹爪夹紧角度
int gripperCloseAngle[3]={30,45,60};

int gripperOpenAngle = 85;//夹爪张角
int liftAngle = 25;//大臂抬升高度

void doGrab(int index){
  int grabBase = objectPos[index][0];
  int grabSh = objectPos[index][1];
  int grabEl = objectPos[index][2];
  int placeBase = objectPos[index][3];
  int placeSh = objectPos[index][4];
  int placeEl = objectPos[index][5];
  int closeAngle = gripperCloseAngle[index];

  // 1. 底座转到抓取点，同时抬高大臂
  base.write(grabBase);
  shoulder.write(constrain(grabSh - liftAngle,shMin,shMax));
  elbow.write(grabEl);
  gripper.write(gripperOpenAngle);
  delay(800);

  // 2. 下降到抓取高度
  shoulder.write(grabSh);
  delay(800);

  // 3. 夹紧（使用该物体专属角度）
  gripper.write(gripperCloseAngle[index]);
  delay(500);

  // 4. 抬升
  shoulder.write(constrain(grabSh - liftAngle, shMin, shMax));
  delay(800);

  // 5. 底座转到放置点
  base.write(placeBase);
  delay(800);

  // 6. 下降到放置高度
  shoulder.write(placeSh);
  elbow.write(placeEl);
  delay(800);

  // 7. 松开
  gripper.write(gripperOpenAngle);
  delay(500);

  // 8. 抬升并回中
  shoulder.write(constrain(placeSh - liftAngle, shMin, shMax));
  base.write(90);
  shoulder.write(90);
  elbow.write(90);
  delay(800);  

}

