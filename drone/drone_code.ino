#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <Dabble.h>
#include <Wire.h>
#include <MPU6050_light.h>

// --- Motor Pin Definitions (User Specified) ---
const int M1_PIN = 21; // Front-Left
const int M2_PIN = 20; // Front-Right
const int M3_PIN = 10; // Back-Right
const int M4_PIN = 5;  // Back-Left

// PWM Settings
const int freq = 500;
const int res = 8;

MPU6050 mpu(Wire);

// --- PID Gains (Adjust these for stability) ---
float pid_p_gain = 1.35;
float pid_i_gain = 0.04;
float pid_d_gain = 16.0;

// --- Control Variables ---
float roll_input = 0;   // Controlled by Bluetooth Left/Right
float pitch_input = 0;  // Controlled by Bluetooth Triangle/Cross
int throttle = 0;       // Controlled by Bluetooth Up/Down

void setup() {
  Serial.begin(115200);
  
  // ESP32-C3 Specified I2C (SDA=7, SCL=6)
  Wire.begin(7, 6); 
  
  // Bluetooth Setup
  Dabble.begin("ESP32-C3-Drone"); 

  // Motor PWM Setup (New ESP32 API)
  ledcAttach(M1_PIN, freq, res);
  ledcAttach(M2_PIN, freq, res);
  ledcAttach(M3_PIN, freq, res);
  ledcAttach(M4_PIN, freq, res);

  // MPU6050 Initialization
  byte status = mpu.begin();
  if(status != 0) {
    while(1) { Serial.println("MPU6050 Error!"); delay(1000); }
  }
  
  Serial.println("Calibrating... Keep Drone level.");
  delay(2000);
  mpu.calcOffsets(); // အရေးကြီးသည်: Calibration လုပ်ချိန် ငြိမ်နေပါစေ
  Serial.println("Ready to Fly!");
}

void loop() {
  Dabble.processInput();
  mpu.update();

  // 1. --- Remote Control Inputs ---
  // Throttle Control (အနိမ့်အမြင့်)
  if (GamePad.isUpPressed()) {
    if(throttle < 230) throttle += 1; // ဖြည်းဖြည်းချင်းတက်ရန်
  }
  if (GamePad.isDownPressed()) {
    if(throttle > 0) throttle -= 1;
  }

  // Roll Control (ဘေးတိုက်စောင်းရန် - Left/Right)
  if (GamePad.isLeftPressed()) roll_input = -15; 
  else if (GamePad.isRightPressed()) roll_input = 15;
  else roll_input = 0;

  // Pitch Control (ရှေ့တိုးနောက်ဆုတ် - Triangle/Cross)
  if (GamePad.isTrianglePressed()) pitch_input = -15; 
  else if (GamePad.isCrossPressed()) pitch_input = 15;
  else pitch_input = 0;

  // Emergency Stop (Select ခလုတ်နှိပ်လျှင် မော်တာအားလုံးရပ်)
  if (GamePad.isSelectPressed()) throttle = 0;

  // 2. --- Stabilization PID Logic ---
  float current_roll = mpu.getAngleX();
  float current_pitch = mpu.getAngleY();

  float out_roll = pid_p_gain * (current_roll - roll_input);
  float out_pitch = pid_p_gain * (current_pitch - pitch_input);

  // 3. --- Motor Mixing ---
  if (throttle < 15) { // Throttle နည်းနေလျှင် မော်တာရပ်ထားရန်
    stopMotors();
  } else {
    // M1, M2 ဘက်ကို Front ထားသော Mixing logic
    int m1 = constrain(throttle - out_pitch + out_roll, 0, 255);
    int m2 = constrain(throttle - out_pitch - out_roll, 0, 255);
    int m3 = constrain(throttle + out_pitch - out_roll, 0, 255);
    int m4 = constrain(throttle + out_pitch + out_roll, 0, 255);

    ledcWrite(M1_PIN, m1);
    ledcWrite(M2_PIN, m2);
    ledcWrite(M3_PIN, m3);
    ledcWrite(M4_PIN, m4);
  }
  
  delay(10); // 100Hz Loop speed
}

void stopMotors() {
  ledcWrite(M1_PIN, 0);
  ledcWrite(M2_PIN, 0);
  ledcWrite(M3_PIN, 0);
  ledcWrite(M4_PIN, 0);
}
