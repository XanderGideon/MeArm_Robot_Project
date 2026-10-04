const int keyCount = 8; 
const int keyPin[keyCount] = {2, 3, 4, 5, 6, 7, 8, 9};
const unsigned int LongMs = 800;
const unsigned int shortMs = 30;

bool keyDown[keyCount];
unsigned int downTime[keyCount];

void setup(){
  Serial.begin(9600);

  for(int i = 0 ; i < keyCount ; i++){
    pinMode(keyPin[i], INPUT_PULLUP);
    keyDown[i] = false;
  }
}

void sendKey(int n, bool isLong){
  if(isLong) Serial.print('L');
  Serial.println(n);
}

void loop(){
  for(int i = 0 ; i < keyCount ; i++){
    bool nowPress =(digitalRead(keyPin[i]) == LOW);
    unsigned long pressedTime;
    if(nowPress && !keyDown[i]){//按下按键
      downTime[i] = millis();
      keyDown[i] = true;
    } else if(!nowPress && keyDown[i]){//松开按键
      keyDown[i] = false;
      pressedTime = millis() - downTime[i]; 
    }

    if(pressedTime >= shortMs){
      sendKey(i + 1 , pressedTime >= LongMs);
    }

  }

}