/* =====================================================================
   任务四：机械臂正 / 逆运动学
   ---------------------------------------------------------------------
   坐标系：底座转轴正下方的桌面点是原点 (0,0,0)，
           x 向右、y 向前、z 向上，单位 mm。
     正解 calcTipPosition()：舵机角 -> 笔尖坐标 tipX / tipY / tipZ
     逆解
   ---------------------------------------------------------------------
        t1 = (baseAngle - baseZero) * baseDir  // 底座与Z轴的夹角（度）
        t2 = (shAngle - shZero) * shDir         // 大臂与水平面夹角（度）
        a  = (elAngle - elZero) * elGain        // 小臂与水平面夹角（度），与 t2 无关
        r  = baseOffset + armLen1*cos(t2) + armLen2*cos(a)   // 笔尖离底座转轴的距离
        x  = r*cos(t1)     y = r*sin(t1)
        z  = (baseHeight - penDown) + armLen1*sin(t2) + armLen2*sin(a)
   ===================================================================== */

void calcTipPos(){
  float t1 = (baseAngle - baseZero) * baseDir * PI / 180.0;
  float t2 = (shAngle - shZero) * shDir * PI / 180.0;
  float a = (baseAngle - baseZero) * baseDir * PI / 180.0;

  float r = baseOffset + armLen1 * cos(t2) + armLen2 * cos(a);
  tipX = r * cos(t1);
  tipY = r * sin(t1);
  tipZ = baseHeight + armLen1 * sin(t2) + armLen2 * sin(a) - penDown;
}

//四舍五入，把浮点数角度换算成整数角度
int roundToInt(float t){
  if(t >= 0) return (int)(t + 0.5);
  return (int)(t - 0.5);
}

bool solveIK(float x, float y, float z){
  float r = sqrt(x * x + y * y) + baseOffset; //解出肩关节到爪缝的水平距离
  float dz = z + penDown - baseHeight; //解爪子到肩关节的竖直距离
  float L = sqrt(r * r + dz * dz); //解肩关节到爪缝中心的距离

  float reachMax = armLen1 + armLen2;
  float reachMin = abs(armLen1 - armLen2);
  if(L < reachMin) return false;
  if(L > reachMax) return false;

  float t1 = atan2(y, x); //先解出底座的偏转角，实现解耦，避免t123的三元计算，减少计算量
  float c3 = (L * L - armLen1 * armLen1 - armLen2 * armLen2)/(2.0 * armLen1 * armLen2);
  c3 = constrain(c3, -1.0, 1.0);
  float t3 = elbowSign * acos(c3);
  float t2 = atan2(dz, r) - atan2(armLen2 * sin(t3), armLen1 + armLen2 * cos(t3));
  float alpha = t2 + t3; //小臂与水平面的绝对角

  ikBase = roundToInt(baseZero + (t1 * PI / 180.0) * baseDir);
  ikSh = roundToInt(shZero + (t2 * PI / 180.0) * shDir);
  ikEl = round(elZero + (alpha * PI / 180.0) * elGain);
  return true;
}

