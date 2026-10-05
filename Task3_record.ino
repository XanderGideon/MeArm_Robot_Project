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
