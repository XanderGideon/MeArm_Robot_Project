/* =====================================================================
    任务三：录制 / 回放（数据定义与开关）
   ---------------------------------------------------------------------
   三个按键的入口在 Key_control.ino（短按 2 开始/结束、短按 3 回放），
   真正"一个点一个点地记"在 loop()（主文件）里做，用 millis() 非阻塞定时，
   所以录制过程中摇杆照样能用 —— 记录的就是摇杆做出来的动作。
   ---------------------------------------------------------------------
   数据都定义在主文件里（MAX_RECORDS / recordData / recordCount /
   isRecording / lastRecordTime / recordInterval），因为主文件排在最前面，
   用常量初始化的全局量必须放那儿；这一片只放行为函数。
   ===================================================================== */

/* ===================== 录制开始 / 结束 =====================
   第一次调：开始录制（并清空上一次的采样点）
   第二次调：结束录制，打印点数、时长和 10 秒校验 */

void recordToggle(){
  if(isRecording == false){ //开始录制
    isRecording = true;
    recordCount = 0;
    lastRecordTime = millis();
    Serial.println(F("Record Start"));
    Serial.println(F("# 用摇杆做动作，超过 10 秒后再按一次 2 结束"));

  } else {//结束录制
    isRecording = false;
    Serial.print("Record:STOP  points=");
    Serial.print(recordCount);
    Serial.print("  time=");
    Serial.print((long)recordCount * recordInterval / 1000);
    Serial.println(" s");

    //录制时长必须大于 10 秒，不够就打印提醒
    if((long)recordCount * recordInterval < 10000){
      Serial.println("WARNING: time <= 10s, need >10s !");
    }
  }
} 

/* ===================== 录满自动停后的统一提示 =====================
   200 个点 × 60ms = 12 秒，录满了自己停并打印一次，格式和手动停一致。
   停止代码在主文件中 */
void printRecordFull() {
  Serial.print(F("Record buffer full -> STOP. points="));
  Serial.print(recordCount);
  Serial.print(F("  time="));
  Serial.print((long)recordCount * recordInterval / 1000);
  Serial.println(F(" s"));
}
