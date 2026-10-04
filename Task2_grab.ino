/* 每个物体只记两个姿态：抓取姿态 + 放置姿态
   一行 6 个数：{ 抓取base, 抓取shoulder, 抓取elbow,
                  放置base, 放置shoulder, 放置elbow }
 */
int objectPos[3][6] = {
  { 30, 130, 80,   135, 125, 80},   // 物体 A：抓取点 base=30，放置点 base=135
  { 75, 130, 80,   155, 125, 80},   // 物体 B：抓取点 base=75，放置点 base=155
  {120, 130, 80,   170, 125, 80},   // 物体 C：抓取点 base=120，放置点 base=170
};


int gripperCloseAngle[3] = {85, 70, 75};//三个物体不同夹爪夹紧角度

int gripperOpenAngle = 20;//夹爪张角,需要在范围之内
int liftAngle = 25;//大臂抬升高度，设置时大臂角度减去这个值

// 目的：让 doGrab 也走 setAngles，自动任务结束后角度变量是同步的
void doGrab(int index){
  if (isAutoRunning || isRecording) {
    Serial.println(F("Busy!"));
    return;
  }
  isAutoRunning = true;   // 动作期间摇杆不生效 

  Serial.print(F("Grab object "));
  Serial.println(index);  // 0=A 1=B 2=C

  int grabBase  = objectPos[index][0];
  int grabSh    = objectPos[index][1];
  int grabEl    = objectPos[index][2];
  int placeBase = objectPos[index][3];
  int placeSh   = objectPos[index][4];
  int placeEl   = objectPos[index][5];
  int closeAngle = gripperCloseAngle[index]; // 获取该物体的专属闭合角度

  //在放置高度和抓取高度中选择高的那一个，设置为移动过程中的高度，防止撞到物体
  int tallSh = grabSh;
  if(placeSh < tallSh) tallSh = placeSh;
  tallSh = tallSh - liftAngle;//角度越小，爪子越高
  if(tallSh < grMin) tallSh = grMin;//限幅

  // 1. 底座转到抓取点，同时抬高大臂，爪子张开
  setAngles(grabBase, tallSh, grabEl, gripperOpenAngle);
  waitMs(800);

  // 2. 下降到抓取高度
  setAngles(grabBase, grabSh, grabEl, gripperOpenAngle);
  waitMs(800);

  // 3. 使用该物体专属角度夹住物体
  setAngles(grabBase, grabSh, grabEl, closeAngle);
  waitMs(500);

  // 4. 抬升
  setAngles(grabBase, tallSh, grabEl, closeAngle);
  waitMs(800);

  // 5. 底座转到放置点
  setAngles(placeBase, tallSh, grabEl, closeAngle);
  waitMs(800);

  // 6. 下降到放置高度
  setAngles(placeBase, placeSh, placeEl, closeAngle);
  waitMs(800);

  // 7. 松开
  setAngles(placeBase, placeSh, placeEl, gripperOpenAngle);
  waitMs(500);

  // 8. 抬升并回中
  setAngles(homePose[0], homePose[1], homePose[2], homeGripperAngle);
  waitMs(800);

  isAutoRunning = false;
  clearSerial();   // 动作过程中按下的按键全部丢掉
  Serial.println(F("Grab done."));  
}
