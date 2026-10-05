/*=========执行任务二与任务三之一抓取=========
输入A/B/C 执行三个不同抓取动作
短按按键1，根据currentgrab循环抓取A/B/C

利用二维数组存储三个物体在关节空间映射的数据（底座，大臂，小臂关节角度）
每一个小数组里面六个角度，前三个是抓取时的角度，后三个时放置时的角度

每个物体只记两个姿态：抓取姿态 + 放置姿态
  一行 6 个数：{ 抓取base, 抓取shoulder, 抓取elbow,
                放置base, 放置shoulder, 放置elbow }
*/

int objectPos[3][6] = {//关节空间
  { 30, 130, 80,   135, 125, 80},   // 物体 A：抓取点 base=30，放置点 base=135
  { 75, 130, 80,   155, 125, 80},   // 物体 B：抓取点 base=75，放置点 base=155
  {120, 130, 80,   170, 125, 80},   // 物体 C：抓取点 base=120，放置点 base=170
};

int gripperCloseAngles[3] = {85, 80, 75};//三个物体不同夹爪夹紧角度
int gripperOpenAngle = 20;//夹爪张角
int liftAngle = 25;//大臂抬升高度，设置时大臂角度减去这个值


void doGrab(int index){//index即为currentgrab
  if(autoBusy()){    // 自动动作中 / 录制中，不许插进来
    Serial.println("Busy!");
    return;
  }
  isAutoRunning = true;//开启自动控制模式

  Serial.print(F("Grab object "));
  Serial.println(index);  // 0=A 1=B 2=C

  int grabBase = objectPos[index][0];
  int grabSh = objectPos[index][1];
  int grabEl = objectPos[index][2];
  int placeBase = objectPos[index][3];
  int placeSh = objectPos[index][4];
  int placeEl = objectPos[index][5];
  int grabCloseAngle = gripperCloseAngles[index];

  //在放置高度和抓取高度中选择高的那一个，设置为移动过程中的高度，防止撞到物体
  int travelSh = grabSh;
  if(placeSh < travelSh) travelSh = placeSh;
  travelSh = travelSh - liftAngle;
  if(travelSh < shMin) travelSh = shMin;

  // 1. 底座转到抓取点，同时抬高大臂，爪子张开  
  setAngles(grabBase, travelSh, grabEl, gripperOpenAngle);
  waitMs(800);

  // 2. 下降到抓取高度
  setAngles(grabBase, grabSh, grabEl, gripperOpenAngle);
  waitMs(800);  
  // 3. 使用该物体专属角度夹住物体
  setAngles(grabBase, travelSh, grabEl, grabCloseAngle);
  waitMs(800);
  // 4. 抬升
  setAngles(grabBase, travelSh, grabEl, grabCloseAngle);
  waitMs(800);
  // 5. 底座转到放置点
  setAngles(placeBase, travelSh, grabEl, grabCloseAngle);
  waitMs(800);
  // 6. 下降到放置高度
  setAngles(placeBase, placeSh, placeEl, grabCloseAngle);
  waitMs(800);
  // 7. 松开
  setAngles(placeBase, placeSh, placeEl, gripperOpenAngle);
  waitMs(800);

  // 8. 抬升并回中
  setAngles(homePose[0], homePose[1], homePose[2], homeGripperAngle);
  waitMs(800);

  isAutoRunning = false; //解除自动控制模式
  clearSerial();//动作过程中的按下的按键全部丢掉
  Serial.println(F("Grab done"));
}