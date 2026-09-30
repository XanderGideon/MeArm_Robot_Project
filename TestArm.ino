// // 测试机械臂各个舵机的安全极限
// void testLimits() {
//   Serial.println("=================================");
//   Serial.println("开始测试极限角度！请手放在电源旁！");
//   Serial.println("听到嗡嗡声或卡住，立刻断电！");
//   Serial.println("=================================");
//   delay(2000); // 留给你准备的时间

//   // 1. 测试底座 (base)
//   Serial.println("测试底座 (0 -> 180)...");
//   for (int angle = 0; angle <= 180; angle += 5) {
//     base.write(angle);
//     Serial.print("Base: "); Serial.println(angle);
//     delay(400); // 每动5度停一下，方便观察
//   }
  
//   // 2. 测试大臂 (shoulder)
//   Serial.println("测试大臂 (0 -> 180)...");
//   for (int angle = 0; angle <= 180; angle += 5) {
//     shoulder.write(angle);
//     Serial.print("Shoulder: "); Serial.println(angle);
//     delay(400);
//   }

//   // 3. 测试小臂 (elbow)
//   Serial.println("测试小臂 (0 -> 180)...");
//   for (int angle = 0; angle <= 180; angle += 5) {
//     elbow.write(angle);
//     Serial.print("Elbow: "); Serial.println(angle);
//     delay(400);
//   }

//   // 4. 测试夹爪 (gripper)
//   Serial.println("测试夹爪 (20 -> 90)...");
//   for (int angle = 20; angle <= 90; angle += 5) {
//     gripper.write(angle);
//     Serial.print("Gripper: "); Serial.println(angle);
//     delay(400);
//   }

//   Serial.println("测试结束！请记录下卡住前的角度。");
  
//   // 测试完恢复到安全的初始姿态
//   base.write(90);
//   shoulder.write(90);
//   elbow.write(90);
//   gripper.write(45);
// }
