#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

// 定義鍵盤的行數和列數
const uint8_t ROWS = 4;
const uint8_t COLS = 4;

// 定義鍵盤上的按鍵
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

// 定義行和列連接到 Arduino 的引腳
uint8_t rowPins[ROWS] = { 23, 19, 18, 5 };   // 選擇可用的 GPIO
uint8_t colPins[COLS] = { 25, 26, 27, 14 };  // 選擇可用的 GPIO

// LCD 顯示器的 I2C 地址和尺寸
#define I2C_ADDR    0x27
#define LCD_COLUMNS 16
#define LCD_LINES   2
LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);

// 初始化鍵盤
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// 定義密碼變量
char correctPassword[5] = "0000"; // 正確密碼
char enteredPassword[5]; // 用戶輸入的密碼
char newPassword[5]; // 新密碼
int passwordIndex = 0; // 當前輸入的密碼索引
bool locked = true; // 鎖定狀態
bool changingPassword = false; // 是否在更改密碼狀態
unsigned long messageClearTime = 0; // 訊息清除時間

// 定義 LED 引腳
#define RED_LED_PIN 13
#define GREEN_LED_PIN 12

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Locked"); // 初始化顯示鎖定狀態
  memset(enteredPassword, 0, sizeof(enteredPassword)); // 初始化密碼緩存
  memset(newPassword, 0, sizeof(newPassword)); // 初始化新密碼緩存

  // 初始化 LED 引腳
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, HIGH); // 鎖定狀態亮紅燈
  digitalWrite(GREEN_LED_PIN, LOW); // 解鎖狀態滅綠燈
}

void loop() {
  char key = keypad.getKey(); // 獲取鍵盤按鍵
  unsigned long currentTime = millis(); // 獲取當前時間

  // 檢查是否需要清除提示信息並返回到 "Locked" 或 "Unlocked"
  if (messageClearTime > 0 && currentTime >= messageClearTime) {
    lcd.clear();
    lcd.setCursor(0, 0);
    if (locked) {
      lcd.print("Locked");
    } else {
      lcd.print("Unlocked");
    }
    messageClearTime = 0; // 重置訊息清除時間
  }

  if (key != NO_KEY) {
    Serial.println(key); // 打印按鍵值到串口監視器

    if (locked) {
      // 處理密碼解鎖
      if (key == '#') {
        enteredPassword[passwordIndex] = '\0'; // 結束當前輸入的密碼

        if (strcmp(enteredPassword, correctPassword) == 0) {
          locked = false;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Unlocked"); // 顯示解鎖
          digitalWrite(RED_LED_PIN, LOW); // 鎖定狀態滅紅燈
          digitalWrite(GREEN_LED_PIN, HIGH); // 解鎖狀態亮綠燈
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Wrong Password"); // 顯示錯誤密碼
          messageClearTime = currentTime + 1000; // 顯示 1 秒後清除
        }
        passwordIndex = 0; // 重置密碼輸入緩存
        memset(enteredPassword, 0, sizeof(enteredPassword)); // 清空緩存
      } else if (passwordIndex < 4 && isdigit(key)) {
        enteredPassword[passwordIndex++] = key; // 添加數字到輸入的密碼
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Locked"); // 顯示鎖定狀態
        lcd.setCursor(0, 1);
        lcd.print(enteredPassword); // 顯示當前輸入的密碼
      }
    } else {
      // 解鎖狀態下
      if (changingPassword) {
        // 處理密碼更改
        if (key == '#') {
          newPassword[passwordIndex] = '\0'; // 結束當前輸入的密碼
          strcpy(correctPassword, newPassword); // 設置新密碼
          changingPassword = false;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Password Changed"); // 顯示密碼已更改
          messageClearTime = currentTime + 1000; // 顯示 1 秒後清除

          // 密碼更改後兩顆LED閃爍5次
          for (int i = 0; i < 5; i++) {
            digitalWrite(RED_LED_PIN, HIGH);
            digitalWrite(GREEN_LED_PIN, HIGH);
            delay(500);
            digitalWrite(RED_LED_PIN, LOW);
            digitalWrite(GREEN_LED_PIN, LOW);
            delay(500);
          }

          // 密碼更改後進入鎖定狀態
          locked = true;
          digitalWrite(RED_LED_PIN, HIGH); // 鎖定狀態亮紅燈
          digitalWrite(GREEN_LED_PIN, LOW); // 解鎖狀態滅綠燈
          passwordIndex = 0;
          memset(newPassword, 0, sizeof(newPassword)); // 清空新密碼緩存
        } else if (key == '*') {
          // 退出並重新進入更改密碼流程
          passwordIndex = 0;
          memset(newPassword, 0, sizeof(newPassword)); // 清空新密碼緩存
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("New Password:"); // 提示輸入新密碼
          lcd.setCursor(0, 1);
          changingPassword = true; // 重新設置為更改密碼模式
        } else if (passwordIndex < 4 && isdigit(key)) {
          newPassword[passwordIndex++] = key; // 添加數字到新密碼
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("New Password:"); // 顯示新密碼提示
          lcd.setCursor(0, 1);
          lcd.print(newPassword); // 顯示當前輸入的新密碼
        }
      } else {
        // 處理其他操作
        if (key == '*') {
          // 鎖回去
          locked = true;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Locked"); // 顯示鎖定狀態
          digitalWrite(RED_LED_PIN, HIGH); // 鎖定狀態亮紅燈
          digitalWrite(GREEN_LED_PIN, LOW); // 解鎖狀態滅綠燈
        } else if (key == 'A' || key == 'B' || key == 'C' || key == 'D') {
          // 收集按鍵輸入
          enteredPassword[passwordIndex++] = key;
          if (passwordIndex == 4 && 
              enteredPassword[0] == 'A' && 
              enteredPassword[1] == 'B' && 
              enteredPassword[2] == 'C' && 
              enteredPassword[3] == 'D') {
            // 開始更改密碼流程
            changingPassword = true;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("New Password:"); // 顯示新密碼提示
            lcd.setCursor(0, 1);
            passwordIndex = 0; // 重置密碼輸入緩存
            memset(enteredPassword, 0, sizeof(enteredPassword)); // 清空舊密碼緩存
            memset(newPassword, 0, sizeof(newPassword)); // 清空新密碼緩存
          }
        } else if (passwordIndex < 4 && isdigit(key)) {
          enteredPassword[passwordIndex++] = key; // 添加數字到輸入的密碼
        }
      }
    }
  }
}
