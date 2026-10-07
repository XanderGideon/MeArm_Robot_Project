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
