#include <Servo.h>   // Servo
#include <Wire.h>    // I2C


#define SLAVE_ADDRESS 0x04

// I2C stuff
int I2Ccommand = 0;
int state = 0;


// Pin 13 has an LED connected on most Arduino boards.
int led = 13;

//
// Graupner stuff
//
// pins from the RC receiver
int rcPinTakeControl   = 12; 
int rcPinEyesLeftRight =  8;
int rcPinEyesUpDown    =  7;
int rcPinSpeech        =  4;
int rcPinMusic         =  2;

int servoLeftValue  = 110;
int servoRightValue = 140;

int ServoValue = 0;

int value = 0;
int newServoValue = 0;

// for storing the last state of the RC switches or knobs
int controlState = 0;
int musicState = 0;
int speechState = 0;

static int NOTCONTROLLED    = 0;
static int REMOTECONTROLLED = 1;

static int MUSICSTOPPED = 0;
static int MUSICPLAYING = 1;
static int MUSICPAUSED  = 2;

// I2C commands from RasPi to read out the RC switches / their stored values in the local variables here
int commandFromRasPi = 0;

static int GETMUSICSWITCH  = 0;
static int GETSPEECHSWITCH = 1;

// a maximum of eight servo objects can be created!
//
Servo servoRightLR;  // create servo object to control a servo 
Servo servoRightUD;  // create servo object to control a servo 
Servo myservo3;  // create servo object to control a servo > not used, eyebrow
Servo servoLeftLR;  // create servo object to control a servo 
Servo servoLeftUD;  // create servo object to control a servo 
Servo myservo6;  // create servo object to control a servo > not used, eyebrow 

int i = 0;    // variable to store the servo position 
int pause = 250;  // for delay in ms

// servo 1
// right eye
// left/right (looking _from_ head head)
static const int servoRightLRpin      =   5;
static const int servoRightLRstart    =  55;    // left     55 ??? left eye from head?!? wtf?
static const int servoRightLRdefault  = 120;    // neutral 120
static const int servoRightLRend      = 180;    // right   180

// servo 2
// right eye
// up/down
static const int servoRightUDpin      =   3;
static const int servoRightUDstart    =  55;    // down     36
static const int servoRightUDdefault  =  80;    // neutral  80
static const int servoRightUDend      = 124;    // up      124

/* servo 3 >>>>>>>>>> not used
 // right eyebrow
 // left/right
 static const int servo3pin      =   6;
 static const int servo3start    =  50;    // right    50
 static const int servo3default  = 100;    // neutral 100
 static const int servo3end      = 136;    // left    136 */

// servo 4
// left eye
// left/right (looking _from_ head head)
static const int servoLeftLRpin      =   9;
static const int servoLeftLRstart    =  30;    // left     30
static const int servoLeftLRdefault  = 100;    // neutral 100
static const int servoLeftLRend      = 155;    // right   155

// servo 5
// left eye
// up/down
static const int servoLeftUDpin      =   6;
static const int servoLeftUDstart    =  55;    // down     36
static const int servoLeftUDdefault  =  80;    // neutral  80
static const int servoLeftUDend      = 124;    // up      124

/* servo 6 >>>>>>>>>> not used
 // left eyebrow
 // left/right
 static const int servo6pin      =  11;
 static const int servo6start    =  42;    // right    42
 static const int servo6default  =  87;    // neutral  87
 static const int servo6end      = 132;    // left    132 */

//! the possible eyes
static const unsigned char LEFTEYE  = 1;
static const unsigned char RIGHTEYE = 2;

//! the possible eyebrows
static const unsigned char LEFTBROW  = 1;
static const unsigned char RIGHTBROW = 2;

//! the possible servo data
static const unsigned char SVSTART   = 0;
static const unsigned char SVEND     = 1;
static const unsigned char SVDEFAULT = 2;
static const unsigned char SVCURRENT = 3;
static const unsigned char SVMIN     = 4;
static const unsigned char SVMAX     = 5;

//! the servo numbers
// +++ do not play around carelessly with these numbers, since we use them later in the eyeRollPos array +++
static const unsigned char SERVO1 = 0;
static const unsigned char SERVO2 = 1;
static const unsigned char SERVO3 = 2;
static const unsigned char SERVO4 = 3;
static const unsigned char SERVO5 = 4;
static const unsigned char SERVO6 = 5;

//! the head directions
static const unsigned char UP         =  0;
static const unsigned char UPLEFT     =  1;
static const unsigned char UPRIGHT    =  2;
static const unsigned char DOWN       =  3;
static const unsigned char DOWNLEFT   =  4;
static const unsigned char DOWNRIGHT  =  5;
static const unsigned char LEFT       =  6;
static const unsigned char RIGHT      =  7;
static const unsigned char FORWARD    =  8;
static const unsigned char NORMAL     =  9; // eyebrow
static const unsigned char CURIOUS    = 10; // eyebrow
static const unsigned char ANGRY      = 11; // eyebrow


//
// General Setup
//
void setup()
{
  // this is for debugging purpuses only
  Serial.begin(9600);

  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);

  //
  // pin init
  //
  // pins which get signal from RC reciever
  pinMode(rcPinTakeControl, INPUT);
  pinMode(rcPinEyesLeftRight, INPUT);
  pinMode(rcPinEyesUpDown, INPUT);
  pinMode(rcPinSpeech, INPUT);
  pinMode(rcPinMusic, INPUT);


  //
  // servo init
  //
  servoRightLR.attach(servoRightLRpin);  // attaches the servo on pin 3 to the servo object 
  servoRightUD.attach(servoRightUDpin);  // attaches the servo on pin 9 to the servo object 
  //  myservo3.attach(servo3pin);  // attaches the servo on pin 9 to the servo object 
  servoLeftLR.attach(servoLeftLRpin);  // attaches the servo on pin 9 to the servo object 
  servoLeftUD.attach(servoLeftUDpin);  // attaches the servo on pin 9 to the servo object 
  //  myservo6.attach(servo6pin);  // attaches the servo on pin 9 to the servo object 

  //  digitalWrite(led, HIGH);   // turn the LED on (HIGH)

  // I2C
  // initialize i2c as slave
  Wire.begin(SLAVE_ADDRESS);
 
  // define callbacks for i2c communication
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);

  // eye default position
  look(FORWARD);
  delay(500);
}


void loop()
{
  // -----------------------------------
  // read the overall control switch
  // -----------------------------------
  // read PPM signals from RC receiver
  value = pulseIn(rcPinTakeControl, HIGH);

  // map read values to servo compatible values
  ServoValue = map(value, 1089, 1880, 0, 255);

  // map read values
  if (ServoValue < 10) // off
  {
    // off
    controlState = NOTCONTROLLED;
  }
  else
  {
      // on
    controlState = REMOTECONTROLLED;
  }


  // are we under control?
  if (controlState == NOTCONTROLLED)
  {
    look(DOWN);
  }
  else
  {
    // -----------------------------------
    // look LEFT, RIGHT
    // -----------------------------------
    // read PPM signals from RC receiver
    value = pulseIn(rcPinEyesLeftRight, HIGH);
  
    // map read values to servo compatible values
    ServoValue = map(value, 1089, 1880, 0, 255);
  
    // fix servo values to their indidivual maximum
    if (ServoValue < servoRightLRstart)
    {
      newServoValue = servoRightLRstart;
    }
    else
    {
      if (ServoValue > servoRightLRend)
      {
        newServoValue = servoRightLRend;
      }
      else
      {
        newServoValue = ServoValue;
      }
    }
  
    // write to servo
    // left eye, left/right, watched _from_ head
    servoRightLR.write(newServoValue);
  
  
    // fix servo values to their indidivual maximum
    if (ServoValue < servoLeftLRstart)
    {
      newServoValue = servoLeftLRstart;
    }
    else
    {
      if (ServoValue > servoLeftLRend)
      {
        newServoValue = servoLeftLRend;
      }
      else
      {
        newServoValue = ServoValue;
      }
    }
  
    // write to servo
    // right eye, left/right, watched _from_ head
    servoLeftLR.write(newServoValue);
  
  
  
    // -----------------------------------
    // look UP, DOWN
    // -----------------------------------
    // read PPM signals from RC receiver
    value = pulseIn(rcPinEyesUpDown, HIGH);
  
    // map read values to servo compatible values
    ServoValue = map(value, 1089, 1880, 0, 255);
  
    // fix servo values to their indidivual maximum
    if (ServoValue < servoRightUDstart)
    {
      newServoValue = servoRightUDstart;
    }
    else
    {
      if (ServoValue > servoRightUDend)
      {
        newServoValue = servoRightUDend;
      }
      else
      {
        newServoValue = ServoValue;
      }
    }
  
    // write to servo
    // left eye, up/down, watched _from_ head
    servoRightUD.write(newServoValue);
    
    // fix servo values to their indidivual maximum
    if (ServoValue < servoLeftUDstart)
    {
      newServoValue = servoLeftUDstart;
    }
    else
    {
      if (ServoValue > servoLeftUDend)
      {
        newServoValue = servoLeftUDend;
      }
      else
      {
        newServoValue = ServoValue;
      }
    }
      
    // write to servo
    // right eye, up/down, watched _from_ head
    servoLeftUD.write(newServoValue);
  
  
    // -----------------------------------
    // read music switch
    // -----------------------------------
    // read PPM signals from RC receiver
    value = pulseIn(rcPinMusic, HIGH);
  
    // map read values to servo compatible values
    ServoValue = map(value, 1089, 1880, 0, 255);
  
    // map read values
    if (ServoValue < 10) // off
    {
      musicState = MUSICSTOPPED;
    }
    else
    {
      if ((ServoValue > 100) && (ServoValue < 150))// middle
      {
        musicState = MUSICPLAYING;
      }
      else
      {
        // on
        musicState = MUSICPAUSED;
      }
    }
  
  
    // -----------------------------------
    // read speech switch
    // -----------------------------------
    // read PPM signals from RC receiver
    value = pulseIn(rcPinSpeech, HIGH);
  
    // map read values to servo compatible values
    ServoValue = map(value, 1089, 1880, 0, 255);
  
    // map read values
    if (ServoValue < 10) // off
    {
      speechState = 0;
    }
    else
    {
      if ((ServoValue > 100) && (ServoValue < 150))// middle
      {
        speechState = 1;
      }
      else
      {
        // on
        speechState = 2;
      }
    }
    
  } // under remote control
  
}



void look(unsigned char direction)
{
  static int lastDirection = FORWARD;


  if (direction == UP)
  {
    eye(LEFTEYE, direction);
    eye(RIGHTEYE, direction);

    // store current view direction
    lastDirection = direction;
    return;
  }

  if (direction == UPLEFT)
  {
    eye(LEFTEYE, direction);
    eye(RIGHTEYE, direction);
    eye(RIGHTEYE, direction);

    // store current view direction
    lastDirection = direction;
    return;
  }

  if (direction == UPRIGHT)
  {
    eye(LEFTEYE, direction);
    eye(RIGHTEYE, direction);
    eye(RIGHTEYE, direction);

    // store current view direction
    lastDirection = direction;
    return;
  }

  if (direction == DOWN)
  {
    eye(LEFTEYE, direction);
    eye(RIGHTEYE, direction);
    eye(RIGHTEYE, direction);

    // store current view direction
    lastDirection = direction;
    return;
  }

  if (direction == DOWNLEFT)
  {
    eye(LEFTEYE, direction);
    eye(RIGHTEYE, direction);
    eye(RIGHTEYE, direction);

    // store current view direction
    lastDirection = direction;
    return;
  }

  if (direction == DOWNRIGHT)
  {
    eye(LEFTEYE, direction);
    eye(RIGHTEYE, direction);
    eye(RIGHTEYE, direction);

    // store current view direction
    lastDirection = direction;
    return;
  }

  if (direction == LEFT)
  {
    eye(LEFTEYE, direction);
    eye(RIGHTEYE, direction);
    eye(RIGHTEYE, direction);

    // store current view direction
    lastDirection = direction;
    return;
  }

  if (direction == RIGHT)
  {
    eye(LEFTEYE, direction);
    eye(RIGHTEYE, direction);
    eye(RIGHTEYE, direction);

    // store current view direction
    lastDirection = direction;
    return;
  }

  if (direction == FORWARD)
  {
    eye(LEFTEYE, direction);
    eye(RIGHTEYE, direction);
    eye(RIGHTEYE, direction);

    // store current view direction
    lastDirection = direction;
    return;
  }

  if (direction == NORMAL)
  {
    //		eyebrow(LEFTBROW, direction);
    //		eyebrow(RIGHTBROW, direction);
    eye(RIGHTEYE, direction);

    // store current view direction
    lastDirection = direction;
    return;
  }

  if (direction == CURIOUS)
  {
    //		eyebrow(LEFTBROW, UP);
    //		eyebrow(RIGHTBROW, UP);
    eye(RIGHTEYE, direction);

    // store current view direction
    lastDirection = direction;
    return;
  }

  if (direction == ANGRY)
  {
    //		eyebrow(LEFTBROW, DOWN);
    //		eyebrow(RIGHTBROW, DOWN);
    eye(RIGHTEYE, direction);

    // store current view direction
    lastDirection = direction;
    return;
  }
}



void eye(unsigned char whichEye, unsigned char direction)
{
  // move servo to the start or end position (*not* to the min or max posisiotns!)
  if (direction == UP)
  {
    if (whichEye==LEFTEYE)
    {
      // up
      moveServo(SERVO2, servoRightUDend);
      // default
      moveServo(SERVO1, servoRightLRdefault);
      return;
    }

    if (whichEye==RIGHTEYE)
    {
      // up
      moveServo(SERVO5, servoLeftUDend);
      // default
      moveServo( SERVO4, servoLeftLRdefault);
      return;
    }
    return;
  }

  if (direction == UPLEFT)
  {
    if (whichEye==LEFTEYE)
    {
      // up
      moveServo(SERVO2, servoRightUDend);
      //left
      moveServo(SERVO4, servoLeftLRstart);
      return;
    }

    if (whichEye==RIGHTEYE)
    {
      // up
      moveServo(SERVO5, servoLeftUDend);
      // left
      moveServo(SERVO1, servoRightLRstart);
      return;
    }
    return;
  }

  if (direction == UPRIGHT)
  {
    if (whichEye==LEFTEYE)
    {
      // right
      moveServo(SERVO4, servoLeftLRend);
      // up
      moveServo(SERVO2, servoRightUDend);
      return;
    }

    if (whichEye==RIGHTEYE)
    {
      // up
      moveServo(SERVO5, servoLeftUDend);
      // right
      moveServo(SERVO1, servoRightLRend);
      return;
    }
    return;
  }

  // move servo to the start or end position (*not* to the min or max posisiotns!)
  if (direction == DOWN)
  {
    if (whichEye==LEFTEYE)
    {
      // down
      moveServo(SERVO2, servoRightUDstart);
      // default
      moveServo( SERVO1, servoRightLRdefault);
      return;
    }

    if (whichEye==RIGHTEYE)
    {
      // down
      moveServo(SERVO5, servoLeftUDstart);
      // default
      moveServo(SERVO4, servoLeftLRdefault);
      return;
    }
    return;
  }

  if (direction == DOWNLEFT)
  {
    if (whichEye==LEFTEYE)
    {
      // down
      moveServo(SERVO2, servoRightUDstart);
      // left
      moveServo(SERVO4, servoLeftLRstart);
      return;
    }

    if (whichEye==RIGHTEYE)
    {
      // down
      moveServo(SERVO5, servoLeftUDstart);
      // left
      moveServo(SERVO1, servoRightLRstart);
      return;
    }
    return;
  }

  if (direction == DOWNRIGHT)
  {
    if (whichEye==LEFTEYE)
    {
      // down
      moveServo(SERVO2, servoRightUDstart);
      // right
      moveServo(SERVO4, servoLeftLRend);
      return;
    }

    if (whichEye==RIGHTEYE)
    {
      // down
      moveServo(SERVO5, servoLeftUDstart);
      // right
      moveServo(SERVO1, servoRightLRend);
      return;
    }
    return;
  }

  // move servo to the start or end position (*not* to the min or max posisiotns!)
  if (direction == LEFT)
  {
    if (whichEye==LEFTEYE)
    {
      // left
      moveServo(SERVO4, servoLeftLRstart);
      // default
      moveServo(SERVO2, servoRightUDdefault);
      return;
    }

    if (whichEye==RIGHTEYE)
    {
      // left
      moveServo(SERVO1, servoRightLRstart);
      // default
      moveServo(SERVO5, servoLeftUDdefault);
      return;
    }
    return;
  }

  /*
  // move servo to its maximum end position
   if (direction == LEFTMAX)
   {
   if (whichEye==LEFTEYE)
   {
   // 
   moveServo(SERVO4, servoLeftLRstart); // min
   return;
   }
   
   if (whichEye==RIGHTEYE)
   {
   // 
   moveServo(SERVO1, servoRightLRstart); // min
   return;
   }
   return;
   }
   */

  // move servo to the start or end position (*not* to the min or max posisiotns!)
  if (direction == RIGHT)
  {
    //qDebug("right");
    if (whichEye==LEFTEYE)
    {
      // right
      moveServo(SERVO4, servoLeftLRend); // end
      // default
      moveServo(SERVO2, servoRightUDdefault);
      return;
    }

    if (whichEye==RIGHTEYE)
    {
      // right
      moveServo(SERVO1, servoRightLRend); // end
      // default
      moveServo(SERVO5, servoLeftUDdefault);
      return;
    }
    return;
  }

  /*
  // move servo to its maximum end position
   if (direction == RIGHTMAX)
   {
   if (whichEye==LEFTEYE)
   {
   moveServo(SERVO4, servoLeftLRend); // max
   return;
   }
   
   if (whichEye==RIGHTEYE)
   {
   moveServo(SERVO1, servoRightLRend); // max
   return;
   }
   return;
   }
   */

  if (direction == FORWARD)
  {
    moveServo(SERVO1, servoRightLRdefault);
    moveServo(SERVO2, servoRightUDdefault);

    moveServo(SERVO4, servoLeftLRdefault);
    moveServo(SERVO5, servoLeftUDdefault);
    return;
  }
}


void eyebrow(unsigned char whichBrow, unsigned char direction)
{
  /*
  	if (direction == NORMAL)
   	{
   		if (whichBrow==LEFTBROW)
   		{
   			// default pos
   			moveServo(SERVO3, servo3default);
   			return;
   		}
   
   		if (whichBrow==RIGHTBROW)
   		{
   			// default pos
   			moveServo(SERVO6, servo6default);
   			return;
   		}
   		return;
   	}
   
   	if (direction == UP)
   	{
   		if (whichBrow==LEFTBROW)
   		{
   			// inside up
   			moveServo(SERVO3, servo3end);
   			return;
   		}
   
   		if (whichBrow==RIGHTBROW)
   		{
   			// inside up
   			moveServo(SERVO6, servo6start);
   			return;
   		}
   		return;
   	}
   	
   	if (direction == DOWN)
   	{
   		if (whichBrow==LEFTBROW)
   		{
   			// inside down
   			moveServo(SERVO3, servo3start);
   			return;
   		}
   
   		if (whichBrow==RIGHTBROW)
   		{
   			// inside down
   			moveServo(SERVO6, servo6end);
   			return;
   		}
   		return;
   	}
   */
}


void rollEye(unsigned char servo, unsigned char hour)
{
  //  Serial.print("Moving to ");
  //  Serial.println(hour);

  //  moveServo(servo, eyeRollPos[servo][hour]);
}


void moveServo(unsigned char servo, unsigned char position)
{
  switch (servo)
  {
  case SERVO1:
    servoRightLR.write(position);
    break; 
  case SERVO2:
    servoRightUD.write(position);
    break; 
  case SERVO3:
    myservo3.write(position);
    break; 
  case SERVO4:
    servoLeftLR.write(position);
    break; 
  case SERVO5:
    servoLeftUD.write(position);
    break; 
  case SERVO6:
    myservo6.write(position);
    break; 
  }
}


// I2C callback for received data
void receiveData(int byteCount)
{
 
  while(Wire.available())
  {
    // read I2C value
    I2Ccommand = Wire.read();
//    Serial.print("data received: ");
//    Serial.println(I2Ccommand);

    // toggle LED to indicate traffic
    if (state == 0)
    {
        digitalWrite(13, HIGH); // set the LED on
        state = 1;
    }
    else
    {
        digitalWrite(13, LOW); // set the LED off
        state = 0;
    }
  }
}

 
// I2C callback for sending data
// This is called from the I2C master!
void sendData()
{
    // which command for Arduino?
    if (I2Ccommand == GETSPEECHSWITCH)
    {
      Wire.write(speechState);
      return;
    }
    
    // which command for Arduino?
    if (I2Ccommand == GETMUSICSWITCH)
    {
      Wire.write(musicState);
      return;
    }
    
    // unkown command received
    Wire.write(0);
}

