
#define CamPinA           3
#define CamPinB           9
#define LEDPinA1          5
#define LEDPinA2          6
#define LEDPinB1          10
#define LEDPinB2          11
#define FPS               3
//Flashing time is flash+camFlashOffset+Trigertime
//Workflow: Flash on -> wait the offset -> trigger cam -> wait triggertime
//          -> untrigger cam -> wait flash -> Flash off 
#define flash             75
#define second            1000

// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(CamPinA, OUTPUT);
  pinMode(CamPinB, OUTPUT);
  pinMode(LEDPinA1, OUTPUT);
  pinMode(LEDPinB1, OUTPUT);
  pinMode(LEDPinA2, OUTPUT);
  pinMode(LEDPinB2, OUTPUT);
}

void triggerDual(int portCam, int portLED, int portLED2){
  analogWrite(portCam,255);
  delay(5);
  analogWrite(portCam,0);
  
  delay(50);
  
  analogWrite(portLED,255);  
  analogWrite(portLED2,255);  
  delay(20);
  analogWrite(portLED,0);
  analogWrite(portLED2,0);
}

// the loop function runs over and over again forever
void loop() {  
 
  //E.g. we have two fps: flash at 0 cam A, 250 cam B, 500 cam A ...
  int halfFlash = second/FPS/2-flash;

  int i=0;
  for(i=0 ; i<FPS ; i++){
    
    //LED for debug
    digitalWrite(13, HIGH);

    //flash camera A and wait half a frame
    triggerDual(CamPinA,LEDPinA1,LEDPinA2); 
    digitalWrite(13, LOW);
    delay(halfFlash);
    triggerDual(CamPinB,LEDPinB1,LEDPinB2); 
    delay(halfFlash);
  }
}


