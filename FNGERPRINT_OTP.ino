#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <Adafruit_Fingerprint.h>
#include <Keypad.h>
#include <Servo.h>

#define redPin 10
#define greenPin 13

#define enrollButton 12
#define deleteButton 11

const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {A0, A1, A2, A3};
byte colPins[COLS] = {4, 5, 6, 7};
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial sim800l(8,9); // RX, TX for GSM
SoftwareSerial fingerPrint(3, 2); // RX, TX for fingerprint

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerPrint);
Servo myServo;

int otp;
String otpstring = "";

void setup() {
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);

  lcd.init();
  lcd.backlight();

  sim800l.begin(4800);
  Serial.begin(115200);

  finger.begin(57600);

  // Configure buttons as INPUT_PULLUP
  pinMode(enrollButton, INPUT_PULLUP);
  pinMode(deleteButton, INPUT_PULLUP);
}

void loop() {
  lcd.setCursor(0, 0);
  lcd.print("FINGERPRINT +OTP");
  lcd.setCursor(0, 1);
  lcd.print("DOOR LOCK SYSTEM");

  if (getFingerprintIDez() >= 0) {
    otp = random(2000, 9999);
    otpstring = String(otp);
    SendSMS();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" OTP Sent to");
    lcd.setCursor(0, 1);
    lcd.print(" Your Mobile");
    delay(100);
    Serial.println(otpstring);
    delay(100);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter OTP :");
    getotp();
  } else {
    checkButtons();
  }
}

int getFingerprintIDez() {
  uint8_t p = finger.getImage();

  if (p != FINGERPRINT_OK) {
    return -1;
  }

  p = finger.image2Tz();

  if (p != FINGERPRINT_OK) {
    return -1;
  }

  p = finger.fingerFastSearch();

  if (p != FINGERPRINT_OK) {
    return -1;
  }

  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  return finger.fingerID;
}

void getotp() {
  String enteredOTP = "";
  while (enteredOTP.length() < 4) {
    char key = customKeypad.getKey();
    if (key) {
      lcd.setCursor(0, 1);
      enteredOTP += key;
      lcd.print(enteredOTP);
    }
  }

  Serial.print("Entered OTP is ");
  Serial.println(enteredOTP);

  if (otpstring == enteredOTP) {
    lcd.setCursor(0, 0);
    lcd.print("Access Granted");
    lcd.setCursor(0, 1);
    lcd.print("Door Opening");
    digitalWrite(redPin, LOW);
    digitalWrite(greenPin, HIGH);
    delay(5000);
    digitalWrite(greenPin, LOW);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Access Failed");
    lcd.setCursor(0, 1);
    lcd.print("Try Again !!!");
    digitalWrite(redPin, HIGH);
    delay(3000);
  }
}

void SendSMS() {
  Serial.println("Sending SMS...");
  sim800l.print("AT+CMGF=1\r");
  delay(100);
  sim800l.print("AT+CSMP=17,167,0,0\r");
  delay(500);
  sim800l.print("AT+CMGS=\"+254726010655\"\r");
  delay(500);
  sim800l.print("Your OTP is " + otpstring + " Just Type OTP And Unlock The Door");
  delay(500);
  sim800l.print((char)26);
  delay(500);
  sim800l.println();
  Serial.println("Text Sent.");
  delay(500);
}

void checkButtons() {
  if (digitalRead(enrollButton) == LOW) {
    lcd.clear();
    lcd.print("Enrolling Finger");
    delay(1000);
    enrollFingerprint();
  } else if (digitalRead(deleteButton) == LOW) {
    lcd.clear();
    lcd.print("Deleting Finger");
    delay(1000);
    deleteFingerprint();
  }
}

void enrollFingerprint() {
  int id = -1;
  while (id == -1) {
    lcd.clear();
    lcd.print("Enter ID (0-9):");
    char key = customKeypad.getKey();
    if (key >= '0' && key <= '9') {
      id = key - '0';
    }
  }

  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("No finger detected");
    delay(2000);
    return;
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Error converting");
    delay(2000);
    return;
  }

  lcd.clear();
  lcd.print("Remove finger");
  delay(2000);

  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }

  lcd.clear();
  lcd.print("Place finger again");
  delay(2000);

  p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("No finger detected");
    delay(2000);
    return;
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Error converting");
    delay(2000);
    return;
  }

  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Error creating model");
    delay(2000);
    return;
  }

  p = finger.storeModel(id);
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Error storing model");
    delay(2000);
    return;
  }

  lcd.clear();
  lcd.print("Enrollment success");
  delay(2000);
}

void deleteFingerprint() {
  lcd.clear();
  lcd.print("Enter ID to delete");
  int idToDelete = -1;
  while (idToDelete == -1) {
    char key = customKeypad.getKey();
    if (key >= '0' && key <= '9') {
      idToDelete = key - '0';
    }
  }

  uint8_t p = finger.deleteModel(idToDelete);
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Fingerprint deleted");
    delay(2000);
  } else {
    lcd.clear();
    lcd.print("Failed to delete");
    delay(2000);
  }
}
