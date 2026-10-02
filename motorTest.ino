// ================= 自检：逐个舵机扫一遍，定位硬件问题 =================
/* 串口发 T 触发。请仔细听/看，日志里每一个舵机扫完都会停顿 1 秒。
   - 某个舵机完全不动 / 嘀嘀响 / 抖动 => 供电不足或接线/舵机坏
   - 两三个一起动或一起卡 => 电源带不动（USB 5V 供电是常见原因） */
void servoTest() {
  Serial.println(F("=== SELF TEST START ==="));
  Serial.println(F(">> BASE pin9"));
  for (int a = 0; a <= 180; a += 30) { base.write(a);     delay(300); Serial.print(F("base="));     Serial.println(a); }
  delay(1000);

  Serial.println(F(">> SHOULDER pin7"));
  for (int a = 15; a <= 165; a += 30) { shoulder.write(a); delay(300); Serial.print(F("shoulder=")); Serial.println(a); }
  delay(1000);

  Serial.println(F(">> ELBOW pin8"));
  for (int a = 0; a <= 180; a += 30) { elbow.write(a);    delay(300); Serial.print(F("elbow="));    Serial.println(a); }
  delay(1000);

  Serial.println(F(">> GRIPPER pin6"));
  for (int a = 20; a <= 90; a += 15) { gripper.write(a);  delay(300); Serial.print(F("gripper="));  Serial.println(a); }
  delay(1000);

  setAngles(90, 90, 90, 90);
  Serial.println(F("=== SELF TEST END, back to 90/90/90/90 ==="));
}
