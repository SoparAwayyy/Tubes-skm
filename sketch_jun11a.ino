#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Servo.h>
#include <Keypad.h>

#define RST_PIN A3
#define SS_PIN 10
#define SERVO_PIN A0
const byte ROWS = 4;
const byte COLS = 4;

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myServo;

char customKeymap[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};
Keypad keypad = Keypad(makeKeymap(customKeymap), rowPins, colPins, ROWS, COLS);

char password[] = "1234";
char enteredPassword[5];
int passwordIndex = 0;
int attemptsLeft = 3;
bool passwordMode = false;
bool systemActive = false;

byte authorizedUID[][4] = {
  {0x68, 0x31, 0x4A, 0x53},
};

const int authorizedUIDCount = sizeof(authorizedUID)/sizeof(authorizedUID[0]);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  SPI.begin();
  mfrc522.PCD_Init();
  delay(4);
  mfrc522.PCD_DumpVersionToSerial();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Tekan * untuk memulai");

  myServo.attach(SERVO_PIN);
  myServo.write(0);
}

void loop() {
  char key = keypad.getKey();

  if (!systemActive) {
    if (key == '*') {
      activateSystem();
    }
  } else {
    readRFID();
    if (passwordMode) {
      handlePasswordInput();
    }
  }
}

void activateSystem() {
  systemActive = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Aktif");
  lcd.setCursor(0, 1);
  lcd.print("tempelkan kartu");
}

void readRFID() {
  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
      mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
      if (isAuthorized(mfrc522.uid.uidByte)) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Kartu terbaca");
        lcd.setCursor(0, 1);
        lcd.print("Password: ");

        passwordMode = true;
        passwordIndex = 0;
        memset(enteredPassword, 0, sizeof(enteredPassword));
      } else {
        handleRFIDError();
      }
    }
  }
}

bool isAuthorized(byte* uid) {
  for (int i = 0; i < authorizedUIDCount; i++) {
    bool match = true;
    for (int j = 0; j < mfrc522.uid.size; j++) {
      if (uid[j] != authorizedUID[i][j]) {
        match = false;
        break;
      }
    }
    if (match) {
      return true;
    }
  }
  return false;
}

void handleRFIDError() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gagal membaca kartu");
  lcd.setCursor(0, 1);
  lcd.print("Coba lagi...");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tekan * untuk memulai");
  resetSystem();
}

void handlePasswordInput() {
  char key = keypad.getKey();
  if (key) {
    if (key == '#') {
      enteredPassword[passwordIndex] = '\0';
      checkPassword();
    } else {
      enteredPassword[passwordIndex] = key;
      passwordIndex++;
      lcd.print('*');
    }
  }
}

void checkPassword() {
  lcd.clear();
  lcd.setCursor(0, 0);

  if (strcmp(enteredPassword, password) == 0) {
    lcd.print("Silahkan Masuk");
    Serial.println("\nPassword Correct! Access Granted.");
    delay(2000);

    unlockDoor();

    resetSystem();
  } else {
    lcd.print("Password Salah");
    Serial.println("\nIncorrect Password! Access Denied.");
    attemptsLeft--;

    if (attemptsLeft > 0) {
      lcd.setCursor(0, 1);
      lcd.print("Kesempatan: " + String(attemptsLeft));
      delay(2000);
      resetPasswordInput();
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Sabar ya");

      for (int countdown = 10; countdown > 0; countdown--) {
        lcd.setCursor(0, 1);
        lcd.print("Sabar ya - " + String(countdown) + " detik ");
        delay(1000);
      }

      lcd.clear();
      lcd.print("Tekan * untuk memulai");
      systemActive = false;
      passwordMode = false;
      passwordIndex = 0;
      attemptsLeft = 3;
      memset(enteredPassword, 0, sizeof(enteredPassword));
    }
  }
}

void unlockDoor() {
  delay(1000);
  Serial.println("Menggerakkan Servo ke posisi terbuka (90)");
  myServo.write(90);
  delay(2000);

  Serial.println("Menggerakkan Servo kembali ke posisi tertutup (0)");
  myServo.write(0);

  delay(1000);
}

void resetSystem() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tekan * untuk memulai");
  systemActive = false;
  passwordMode = false;
  passwordIndex = 0;
  attemptsLeft = 3;
  memset(enteredPassword, 0, sizeof(enteredPassword));
}

void resetPasswordInput() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Password: ");
  passwordIndex = 0;
  memset(enteredPassword, 0, sizeof(enteredPassword));
}
