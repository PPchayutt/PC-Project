#include <Arduino.h>
#include <U8g2lib.h>  // สำหรับจอ
#include <Wire.h>     // สำหรับ Hardware I2C
#include <dw_font.h>  // สำหรับภาษาไทย

// ---------- PINS ----------
const uint8_t BUTTON_A_PIN = 12;
const uint8_t BUTTON_B_PIN = 13;

const uint8_t RGB_R_PIN = 9;
const uint8_t RGB_G_PIN = 10;
const uint8_t RGB_B_PIN = 11;

const uint8_t BUZZER_PIN = 4;
const bool PASSIVE_BUZZER = true;

const uint8_t TRIGGER_PIN = 2;  // ขาปล่อย 5V

// ---------- DISPLAY ----------
// จอเล็ก (Software I2C, SSD1306)
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2_small(U8G2_R0, /* scl=*/5, /* sda=*/6, /* reset=*/U8X8_PIN_NONE);
// จอใหญ่ (Hardware I2C, SH1106, Address 0x78)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2_large(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* address=*/0x78);

// ---------- dw_font Setup ----------
extern dw_font_info_t font_th_sarabunpsk_regular20;  // เลือกขนาดฟอนต์ 16-18-20
dw_font_t myfont_large;
const uint8_t *eng_font = u8g2_font_helvB12_tr;  // ฟอนต์อังกฤษสำหรับจอเล็ก

// ---------- QUIZ DATA ----------
struct Question {
  const char *question_text;
  bool answer_is_true;
};
Question questions[] = {
  { "วงจรเปิดมีกระแส = 0A", true },
  { "ตัวต้านทานต่อขนาน มูลค่าเท่าผลรวมค่าตัวต้านทาน", false },
  { "ไฟตรง DC มีความถี่ 0Hz", true },
  { "แหล่งแรงดันอุดมคติ มี R ภายในเป็นศูนย์", true },
  { "กฎของโอห์ม V=I/R", false },
  { "คาปาซิเตอร์ขัดขวางสัญญาณคงที่(DC)", true }

};
const int TOTAL_Q = sizeof(questions) / sizeof(questions[0]);

int orderIdx[TOTAL_Q];           // Array เก็บ index ที่สุ่มแล้ว
int curOrderIndex = 0;           // Index ปัจจุบันใน orderIdx
int current_question_index = 0;  // Index ของคำถามปัจจุบัน (จาก questions[])

// ---------- TIMING ----------
const unsigned long DEBOUNCE_MS = 50;  // เพิ่ม debounce time
const unsigned long SHORT_BEEP_MS = 150;
const unsigned long FREQ_RIGHT = 1500;
const unsigned long FREQ_WRONG = 300;

unsigned long lastChangeMs = 0;
bool buzzerOn = false;
unsigned long beepEndMs = 0;

// ---------- STATE ----------
enum State { WAIT_ANSWER,
             WRONG_LOCK,
             RIGHT_FEEDBACK };
State state = WAIT_ANSWER;

// --- dw_font Callback Functions ---
void draw_pixel_large_u8g2(int16_t x, int16_t y) {
  u8g2_large.drawPixel(x, y);
}
void clear_pixel_large_u8g2(int16_t x, int16_t y) {
}

// ---------- HELPERS ----------
void rgbOff() {
  analogWrite(RGB_R_PIN, 255);
  analogWrite(RGB_G_PIN, 255);
  analogWrite(RGB_B_PIN, 255);
}
void rgbSet(int r, int g, int b) {
  analogWrite(RGB_R_PIN, 255 - r);
  analogWrite(RGB_G_PIN, 255 - g);
  analogWrite(RGB_B_PIN, 255 - b);
}

void buzzerStop() {
  if (PASSIVE_BUZZER) noTone(BUZZER_PIN);
  else digitalWrite(BUZZER_PIN, LOW);
  buzzerOn = false;
}
void buzzerLongWrong() {
  if (PASSIVE_BUZZER) tone(BUZZER_PIN, FREQ_WRONG);
  else digitalWrite(BUZZER_PIN, HIGH);
  buzzerOn = true;
}
void buzzerShortRight() {
  if (PASSIVE_BUZZER) tone(BUZZER_PIN, FREQ_RIGHT);
  else digitalWrite(BUZZER_PIN, HIGH);
  buzzerOn = true;
  beepEndMs = millis() + SHORT_BEEP_MS;
}

// --- Button Reading ---
int readButtonsDebounced() {
  int raw = 0;
  if (digitalRead(BUTTON_A_PIN) == LOW) raw |= 0x01;
  if (digitalRead(BUTTON_B_PIN) == LOW) raw |= 0x02;

  static int lastRaw = 0;
  static int stable = 0;  // ใช้ static เพื่อให้จำค่าข้าม loop ได้

  if (raw != lastRaw) {
    lastChangeMs = millis();
    lastRaw = raw;
  }
  if (millis() - lastChangeMs > DEBOUNCE_MS) {
    if (stable != raw) {
      int changed = raw & (~stable);  // หาบิตที่เปลี่ยนจาก 0 เป็น 1 (กดลง)
      stable = raw;
      if (changed & 0x01) return 1;  // A pressed
      if (changed & 0x02) return 2;  // B pressed
    }
  }
  return 0;  // No new press detected
}

// --- Question Handling ---
void shuffleQuestions() {
  for (int i = 0; i < TOTAL_Q; i++) orderIdx[i] = i;
  for (int i = 0; i < TOTAL_Q; i++) {
    int j = random(i, TOTAL_Q);
    int t = orderIdx[i];
    orderIdx[i] = orderIdx[j];
    orderIdx[j] = t;
  }
  curOrderIndex = 0;
  current_question_index = orderIdx[curOrderIndex];  // ตั้งค่าคำถามแรก
}

void nextQuestion() {
  curOrderIndex++;
  if (curOrderIndex >= TOTAL_Q) {
    shuffleQuestions();  // ถ้าหมดแล้ว สุ่มใหม่
  } else {
    current_question_index = orderIdx[curOrderIndex];  // ไปคำถามถัดไปในลำดับที่สุ่มไว้
  }
  Serial.print("Next Question Index: ");
  Serial.println(current_question_index);
}

// --- Drawing Function ---
void drawScreens() {
  const Question &q = questions[current_question_index];

  // จอใหญ่: คำถาม (dw_font ภาษาไทย)
  u8g2_large.clearBuffer();
  dw_font_goto(&myfont_large, 0, 25);                     // ปรับ Y ตามขนาดฟอนต์
  dw_font_print(&myfont_large, (char *)q.question_text);  // ใช้ (char*) casting
  u8g2_large.sendBuffer();

  // จอเล็ก: ตัวเลือก + สถานะ (U8g2 อังกฤษ)
  u8g2_small.clearBuffer();
  u8g2_small.setFont(eng_font);
  u8g2_small.setCursor(10, 25);
  u8g2_small.print("A: True");
  u8g2_small.setCursor(10, 55);
  u8g2_small.print("B: False");

  if (state == WRONG_LOCK) {
    u8g2_small.setCursor(0, 10);  // แสดงข้อความตัวเล็กๆ ด้านบน
    u8g2_small.setFont(u8g2_font_6x10_tf);
    u8g2_small.print("WRONG! :(");
  } else if (state == RIGHT_FEEDBACK) {
    u8g2_small.setCursor(0, 10);
    u8g2_small.setFont(u8g2_font_6x10_tf);
    u8g2_small.print("CORRECT! :)");
  }
  u8g2_small.sendBuffer();
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Pins
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
  pinMode(RGB_R_PIN, OUTPUT);
  pinMode(RGB_G_PIN, OUTPUT);
  pinMode(RGB_B_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIGGER_PIN, OUTPUT);

  // Initial states
  rgbOff();
  buzzerStop();
  digitalWrite(TRIGGER_PIN, LOW);

  // Displays
  u8g2_small.begin();
  u8g2_large.begin();

  // dw_font
  dw_font_init(&myfont_large, 128, 64, draw_pixel_large_u8g2, clear_pixel_large_u8g2);
  dw_font_setfont(&myfont_large, &font_th_sarabunpsk_regular20);

  // Shuffle questions
  randomSeed(analogRead(A0));
  shuffleQuestions();  // สุ่มคำถามครั้งแรก

  Serial.println("Setup Complete. Starting Quiz!");
  drawScreens();  // แสดงคำถามแรก
}

// ---------- LOOP ----------
void loop() {
  // จัดการให้ beep สั้นดับเอง และไปข้อต่อไป
  if (buzzerOn && state == RIGHT_FEEDBACK && millis() >= beepEndMs) {
    buzzerStop();
    rgbOff();
    state = WAIT_ANSWER;
    nextQuestion();  // ไปข้อต่อไปหลังจาก feedback จบ
    drawScreens();   // แสดงคำถามใหม่
  }

  int button_pressed = readButtonsDebounced();  // 0:none, 1:A, 2:B
  if (button_pressed == 0) return;              // ถ้าไม่มีปุ่มกด ก็ไม่ต้องทำอะไรต่อ

  bool user_chose_A = (button_pressed == 1);
  bool correct_answer = questions[current_question_index].answer_is_true;

  switch (state) {
    case WAIT_ANSWER:
      {
        if ((user_chose_A && correct_answer) || (!user_chose_A && !correct_answer)) {
          // ตอบถูก
          Serial.println("Correct!");
          digitalWrite(TRIGGER_PIN, LOW);  // ไม่ส่ง Trigger
          rgbSet(0, 255, 0);               // เขียว
          buzzerShortRight();
          state = RIGHT_FEEDBACK;  // เปลี่ยนสถานะเพื่อรอ feedback จบ
          drawScreens();           // อัปเดตจอเล็ก (แสดงว่า Correct!)
        } else {
          // ตอบผิด
          Serial.println("Incorrect!");
          rgbSet(255, 0, 0);  // แดง
          buzzerLongWrong();
          digitalWrite(TRIGGER_PIN, HIGH);  // ส่ง Trigger
          state = WRONG_LOCK;               // ล็อกสถานะรอแก้ตัว
          drawScreens();                    // อัปเดตจอเล็ก (แสดงว่า Wrong!)
        }
        break;
      }  // end case WAIT_ANSWER

    case WRONG_LOCK:
      {
        // รอแก้ตัว ต้องกดคำตอบที่ถูกเท่านั้น
        if ((user_chose_A && correct_answer) || (!user_chose_A && !correct_answer)) {
          // กดถูกแล้ว
          Serial.println("Corrected!");
          buzzerStop();
          digitalWrite(TRIGGER_PIN, LOW);  // หยุด Trigger
          rgbSet(0, 255, 0);               // เขียว
          buzzerShortRight();
          state = RIGHT_FEEDBACK;  // เปลี่ยนสถานะเพื่อรอ feedback จบ
          drawScreens();           // อัปเดตจอเล็ก (แสดงว่า Correct!)
        }
        // ถ้ากดผิดซ้ำ ก็ไม่ต้องทำอะไร ปล่อยให้ไฟแดง+เสียงดังต่อไป
        break;
      }  // end case WRONG_LOCK

    case RIGHT_FEEDBACK:
      // กำลังแสดง feedback ตอบถูกอยู่ ไม่ต้องทำอะไร รอให้มันจบในเงื่อนไขแรกสุดของ loop()
      break;
  }  // end switch
}  // end loop()
