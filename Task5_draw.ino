//子任务一直线的首尾点坐标规定(相对于纸面中心)
float lineX1_inpaper = -30.0, lineY1_inpaper = 0.0;
float lineX2_inpaper = 30.0, lineY2_inpaper = 0.0;

//把纸面参考系中的笔尖坐标转换回绝对参考系中的笔尖坐标
float xPaperToWorld(float u){return u + paperX;}
float yPaperToWorld(float v){return v + paperY;}
