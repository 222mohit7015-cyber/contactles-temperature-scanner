#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_MLX90614.h>
#include "DFRobotDFPlayerMini.h"
#include <HardwareSerial.h>
#define trigPin 5
#define echoPin 18

LiquidCrystal_I2C lcd(0x27,16,2);
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

HardwareSerial mySerial(2);
DFRobotDFPlayerMini player;

long duration;
float distance;
float tempOffset = 1.6;

unsigned long lastVoiceTime = 0;
int lastState = -1;

String startText = "   Contactless Body Temperature Scanner   ";

// Scroll Text
void scrollText(String text)
{
  for(int i=0;i<text.length()-15;i++)
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(text.substring(i,i+16));
    delay(200);
  }
}


// Distance
float getDistance()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  return distance;
}

// Stable Temp
float getStableTemperature()
{
  float readings[10];

  for(int i=0;i<10;i++)
  {
    readings[i] = mlx.readObjectTempC();
    delay(70);
  }

  float minVal = readings[0];
  float maxVal = readings[0];
  float sum = 0;

  for(int i=0;i<10;i++)
  {
    if(readings[i] < minVal) minVal = readings[i];
    if(readings[i] > maxVal) maxVal = readings[i];
    sum += readings[i];
  }

  sum = sum - minVal - maxVal;
  float avg = sum / 8;

  return avg + tempOffset;
}

// Animation
void scanAnimation()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Scanning Temp");

  for(int i=0;i<=16;i++)
  {
    lcd.setCursor(0,1);
    for(int j=0;j<i;j++)
    {
      lcd.print(">");
    }
    delay(70);
    lcd.setCursor(0,1);
  }
}

// Voice control
void playVoice(int track, int state)
{
  if(state != lastState && millis() - lastVoiceTime > 2000)
  {
    player.play(track);
    lastVoiceTime = millis();
    lastState = state;
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  lcd.init();
  lcd.backlight();

  mlx.begin();

  // DFPlayer
  mySerial.begin(9600, SERIAL_8N1, 16, 17);
  if (!player.begin(mySerial))
  {
    Serial.println("DFPlayer Error");
  }
  player.volume(25);

  scrollText(startText);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("System Ready");
  lcd.setCursor(0,1);
  lcd.print("Place Forehead");
  delay(2000);
}

void loop()
{
  distance = getDistance();

  Serial.print("Distance: ");
  Serial.println(distance);

  // TOO FAR
  if(distance > 10)
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Move Closer");

    lcd.setCursor(0,1);
    lcd.print("Dist:");
    lcd.print(distance,0);
    lcd.print(" cm");

    playVoice(2, 1); // Move closer
    delay(400);
  }

  // CORRECT RANGE
  else if(distance >=5 && distance <=10)
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Hold Still");

    playVoice(3, 2); // Hold still

    scanAnimation();

    float temp = getStableTemperature();

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Temp:");
    lcd.print(temp,1);
    lcd.print("C");

    if(temp >= 37.5)
    {
      lcd.setCursor(0,1);
      lcd.print("FEVER!");

      playVoice(5, 4); // Fever
    }
    else
    {
      lcd.setCursor(0,1);
      lcd.print("Normal");

      playVoice(4, 3); // Normal
    }

    delay(3000);
  }

  // TOO CLOSE
  else
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Too Close");

    lcd.setCursor(0,1);
    lcd.print("Step Back");

    playVoice(1, 5); // Too close
    delay(400);
  }
}
