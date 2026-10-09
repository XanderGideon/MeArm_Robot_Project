/* =====================================================================
   遥控板通过串口把按键发过来（短按发 1~8，长按发 L1~L8），
     按键 1~4   任务一 / 二 / 三
     按键 5~8   任务五

     短按 1 = 循环夹取 A→B→C（用 currentgrab 计数，0=A/1=B/2=C）
     短按 2 = 录制开始 / 结束
     短按 3 = 回放
     短按 4 = 回中
     长按 1~4 无效
     短按 5 = 记一个示教点（最多 5 个）      长按 5 = 清空示教点
     短按 6 = 开始画 / 正在画时再按一次 = 停  长按 6 = 换子任务
     短按 7 = 暂停 / 继续                 
     短按 8 = 取消（停住回待机）     
   ===================================================================== */

bool autoBusy(){
  return isAutoRunning || isRecording;
}

// ===================== 任务一 / 二 / 三：按键 1~4 =====================
void doKeyTask(int n){
   // ---------- 按键1：循环夹取 A -> B -> C -> A ... ----------
  if(n == 1){
    if(autoBusy()){
      Serial.println(F("Busy!"));
      return;
    }
    Serial.print(F("Key1 Pressed -> object:"));
    Serial.println(currentgrab);   // 0=A, 1=B, 2=C
    if(currentgrab < 2){
      doGrab(currentgrab);
      currentgrab++;
    }
    if(currentgrab > 2) currentgrab = 0;
    return;
  }

  // ---------- 按键2：录制（第一次按开始，第二次按结束）----------
  /* 这里故意只判断 isAutoRunning / drawState，不判断 isRecording：
     第二次按下的作用就是"结束录制"，要是也拦着，就永远停不下来了。 */
  if(n == 2){
    if(isAutoRunning){
      Serial.println(F("Busy!"));
      return;
    }
    recordToggle();
    return;
  }

    // ---------- 按键3：回放上一次录制的动作 ----------
  if(n == 3){
    if(autoBusy()){//判断状态
      Serial.println(F("Busy!"));
      return;
    }
    if(recordCount == 0){//检测数据合理性
      Serial.println(F("No record!"));
      return;      
    }

    isAutoRunning = true;
    Serial.println(F("Play Start"));
    for(int i = 0 ; i < recordCount ; i++){
      setAngles(recordData[i][0], recordData[i][1], recordData[i][2], recordData[i][3]);
      delay(recordInterval);   // 必须按录制节拍复现，否则动作会变形
    }
    isAutoRunning = false;
    Serial.println(F("play Stop"));
    return;
  }

  // ---------- 按键4：回中 ----------
  if(n == 4){
    if(autoBusy()){
      Serial.println(F("Busy!"));
      return;    
    }
    isAutoRunning = true;
    setAngles(homePose[0], homePose[1], homePose[2], homePose[3]);
    waitMs(800);
    isAutoRunning = false;
    clearSerial();
    return;
  }

  Serial.println(F("# 按键1..4 的长按没有定义(长按只用在按键 5~8)"));
}

void doTask5(int n , bool isLong){
  //代码
}




// ===================== 按键总入口 =====================
/* n = 1..8，isLong = 是不是长按（遥控板任意按键被按住超过 0.8 秒）
   1~4 是任务一/二/三，5~8 是任务五，直接按号码分发，没有工作模式。
   长按只用在按键 5~8 */
void doKey(int n, bool isLong){
  if (n < 1 || n > 8) return;
  if(n <= 4){
    doKeyTask(n);
  } else {
    //任务五按键代码
  }
}

