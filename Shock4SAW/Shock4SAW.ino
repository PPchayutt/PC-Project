#include <Arduino.h>
#include <U8g2lib.h>  // สำหรับจอ
#include <Wire.h>     // สำหรับ Hardware I2C
#include <dw_font.h>  // สำหรับภาษาไทย

// ---------- PINS ----------
const uint8_t BUTTON_A_PIN = 12;  // ปุ่ม A (True)
const uint8_t BUTTON_B_PIN = 13;  // ปุ่ม B (False)

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
extern dw_font_info_t font_th_sarabunpsk_regular20;  // เลือกขนาดฟอนต์ 20
dw_font_t myfont_large;
const uint8_t *eng_font = u8g2_font_helvB12_tr;  // ฟอนต์อังกฤษสำหรับจอเล็ก

// ---------- QUIZ DATA ----------
struct Question {
  const char *question_line1;  // บรรทัดที่ 1
  const char *question_line2;  // บรรทัดที่ 2
  bool answer_is_true;         // คำตอบ
};

// คำถาม 20 ข้อ
Question questions[] = {
  { "void setup()", "ทำงานแค่ 1 รอบ?", true },
  { "void loop()", "ทำงานวนซ้ำตลอด?", true },
  { "pinMode()", "ใช้อ่านค่าไฟ?", false },
  { "digitalWrite()", "ใช้เขียนค่า 0/1?", true },
  { "delay(1000)", "คือรอ 1 วินาที?", true },
  { "สาย USB", "ใช้จ่ายไฟได้?", true },
  { "สถานะ HIGH", "คือ 0 โวลต์?", false },
  { "สถานะ LOW", "คือ 0 โวลต์?", true },
  { "// (ทับสองครั้ง)", "คือคอมเมนต์?", true },
  { "analogRead()", "คืนค่า 0-1023?", true },
  { "digitalRead()", "คืนค่า 0-1023?", false },
  { "ขา LED (ยาว)", "คือขั้วบวก (A)?", true },
  { "ขา LED (สั้น)", "คือขั้วบวก (A)?", false },
  { "GND (กราวด์)", "คือขั้วลบ?", true },
  { "PWM ใช้สร้าง", "สัญญาณอนาล็อก?", true },
  { "บอร์ด Uno R4", "ใช้ไฟ 5V?", true },
  { "Serial.begin()", "ใช้เริ่มสื่อสาร?", true },
  { "ตัวต้านทาน", "มีขั้วไหม?", false },
  { "Buzzer", "ใช้แสดงข้อความ?", false },
  { "I2C ใช้สาย", "สัญญาณ 4 เส้น?", false }
};

const int TOTAL_Q = sizeof(questions) / sizeof(questions[0]);  // นับ 20 ข้อให้เอง

int orderIdx[TOTAL_Q];           // Array เก็บ index ที่สุ่มแล้ว
int curOrderIndex = 0;           // Index ปัจจุบันใน orderIdx
int current_question_index = 0;  // Index ของคำถามปัจจุบัน (จาก questions[])

// ตัวแปรเก็บคะแนน
int score_correct = 0;
int score_wrong = 0;

// ---------- TIMING ----------
const unsigned long DEBOUNCE_MS = 50;
const unsigned long SHORT_BEEP_MS = 150;
const unsigned long FREQ_RIGHT = 1500;
const unsigned long FREQ_WRONG = 300;

unsigned long lastChangeMs = 0;
bool buzzerOn = false;
unsigned long beepEndMs = 0;

// ---------- STATE ----------
// สถานะ SHOW_SUMMARY
enum State {
  WAIT_ANSWER,
  WRONG_LOCK,
  RIGHT_FEEDBACK,
  SHOW_SUMMARY  // สถานะสำหรับสรุปคะแนน
};
State state = WAIT_ANSWER;

// --- dw_font Callback Functions ---
void draw_pixel_large_u8g2(int16_t x, int16_t y) {
  u8g2_large.drawPixel(x, y);
}
void clear_pixel_large_u8g2(int16_t x, int16_t y) {
  // ไม่ต้องทำอะไร
}

// ---------- HELPERS (ฟังก์ชันช่วย) ----------
void rgbOff() {
  analogWrite(RGB_R_PIN, 255);
  analogWrite(RGB_G_PIN, 255);
  analogWrite(RGB_B_PIN, 255);
}
void rgbSet(int r, int g, int b) {  // Common Anode: 0=ON, 255=OFF
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
  static int stable = 0;
  if (raw != lastRaw) {
    lastChangeMs = millis();
    lastRaw = raw;
  }
  if (millis() - lastChangeMs > DEBOUNCE_MS) {
    if (stable != raw) {
      int changed = raw & (~stable);
      stable = raw;
      if (changed & 0x01) return 1;  // A pressed
      if (changed & 0x02) return 2;  // B pressed
    }
  }
  return 0;
}

// --- Question Handling ---
void shuffleQuestions() {
  // รีเซ็ตคะแนนทุกครั้งที่สุ่มใหม่
  score_correct = 0;
  score_wrong = 0;

  for (int i = 0; i < TOTAL_Q; i++) orderIdx[i] = i;
  for (int i = 0; i < TOTAL_Q; i++) {
    int j = random(i, TOTAL_Q);
    int t = orderIdx[i];
    orderIdx[i] = orderIdx[j];
    orderIdx[j] = t;
  }
  curOrderIndex = 0;
  current_question_index = orderIdx[curOrderIndex];
}

void nextQuestion() {
  curOrderIndex++;
  // ตรวจสอบว่าครบทุกข้อหรือยัง
  if (curOrderIndex >= TOTAL_Q) {
    // --- ครบทุกข้อแล้ว ---
    state = SHOW_SUMMARY;  // เปลี่ยนสถานะเป็นโชว์สรุป
    curOrderIndex = 0;     // รีเซ็ต index ไว้รอ
  } else {
    // --- ยังไม่ครบ ---
    current_question_index = orderIdx[curOrderIndex];  // ไปคำถามถัดไป
  }
  Serial.print("Next Question Index: ");
  Serial.println(current_question_index);
}

// --- Drawing Function ---
void drawScreens() {
  if (state == SHOW_SUMMARY) {
    // ----- แสดงหน้าจอสรุปคะแนน -----

    // จอใหญ่: แสดงสรุป
    u8g2_large.clearBuffer();

    // 1. พิมพ์ "สรุปคะแนน"
    dw_font_goto(&myfont_large, 0, 25);
    dw_font_print(&myfont_large, (char *)"สรุปคะแนน");

    // 2. สร้าง Buffer (กล่องข้อความชั่วคราว)
    char buffer_correct[30];  // "ถูก : XX ข้อ"
    char buffer_wrong[30];    // "ผิด : XX ข้อ"

    // 3. Format ข้อความลงใน Buffer
    snprintf(buffer_correct, 30, "ถูก : %d ข้อ", score_correct);
    snprintf(buffer_wrong, 30, "ผิด : %d ข้อ", score_wrong);

    // 4. พิมพ์ Buffer ที่มีภาษาไทยและตัวเลข ด้วย dw_font
    dw_font_goto(&myfont_large, 0, 45);  // (ปรับ Y ให้อยู่ใต้บรรทัดสรุป)
    dw_font_print(&myfont_large, buffer_correct);

    dw_font_goto(&myfont_large, 0, 60);  // (ปรับ Y บรรทัดต่อมา)
    dw_font_print(&myfont_large, buffer_wrong);

    u8g2_large.sendBuffer();

    // จอเล็ก: บอกให้เริ่มใหม่
    u8g2_small.clearBuffer();
    u8g2_small.setFont(eng_font);
    u8g2_small.setCursor(10, 25);
    u8g2_small.print("Press any");
    u8g2_small.setCursor(10, 50);
    u8g2_small.print("button...");
    u8g2_small.sendBuffer();

  } else {
    // ----- แสดงคำถามตามปกติ -----
    const Question &q = questions[current_question_index];

    // จอใหญ่: คำถาม (dw_font ภาษาไทย)
    u8g2_large.clearBuffer();
    const int FONT_HEIGHT = 20;
    const int LINE_SPACING = 5;
    int y_line1 = FONT_HEIGHT + 5;
    int y_line2 = (FONT_HEIGHT * 2) + LINE_SPACING + 5;
    dw_font_goto(&myfont_large, 0, y_line1);
    dw_font_print(&myfont_large, (char *)q.question_line1);
    dw_font_goto(&myfont_large, 0, y_line2);
    dw_font_print(&myfont_large, (char *)q.question_line2);
    u8g2_large.sendBuffer();

    // จอเล็ก: ตัวเลือก + สถานะ (U8g2 อังกฤษ)
    u8g2_small.clearBuffer();
    u8g2_small.setFont(eng_font);
    u8g2_small.setCursor(10, 25);
    u8g2_small.print("A: True");
    u8g2_small.setCursor(10, 55);
    u8g2_small.print("B: False");
    if (state == WRONG_LOCK) {
      u8g2_small.setCursor(0, 10);
      u8g2_small.setFont(u8g2_font_6x10_tf);
      u8g2_small.print("WRONG! :(");
    } else if (state == RIGHT_FEEDBACK) {
      u8g2_small.setCursor(0, 10);
      u8g2_small.setFont(u8g2_font_6x10_tf);
      u8g2_small.print("CORRECT! :)");
    }
    u8g2_small.sendBuffer();
  }
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
    nextQuestion();  // ไปข้อต่อไป (หรือไปหน้าสรุป)
    drawScreens();   // แสดงคำถามใหม่ (หรือหน้าสรุป)
  }

  int button_pressed = readButtonsDebounced();

  // อนุญาตให้กดปุ่มได้ตอนอยู่หน้าสรุป
  if (button_pressed == 0 && state != SHOW_SUMMARY) return;

  bool user_chose_A = (button_pressed == 1);
  bool correct_answer = (state == WAIT_ANSWER || state == WRONG_LOCK) ? questions[current_question_index].answer_is_true : false;

  switch (state) {
    case WAIT_ANSWER:
      {
        if (button_pressed == 0) return;  // ถ้าไม่กดปุ่ม ก็ไม่ต้องทำอะไร

        if ((user_chose_A && correct_answer) || (!user_chose_A && !correct_answer)) {
          // ตอบถูก
          Serial.println("Correct!");
          score_correct++;  // นับคะแนน
          digitalWrite(TRIGGER_PIN, LOW);
          rgbSet(0, 255, 0);
          buzzerShortRight();
          state = RIGHT_FEEDBACK;
          drawScreens();
        } else {
          // ตอบผิด
          Serial.println("Wrong!");
          score_wrong++;  // นับคะแนน
          rgbSet(255, 0, 0);
          buzzerLongWrong();
          digitalWrite(TRIGGER_PIN, HIGH);
          state = WRONG_LOCK;
          drawScreens();
        }
        break;
      }

    case WRONG_LOCK:
      {
        if (button_pressed == 0) return;  // ถ้าไม่กดปุ่ม ก็ไม่ต้องทำอะไร

        if ((user_chose_A && correct_answer) || (!user_chose_A && !correct_answer)) {
          // กดถูกแล้ว
          Serial.println("Correct!");
          buzzerStop();
          digitalWrite(TRIGGER_PIN, LOW);
          rgbSet(0, 255, 0);
          buzzerShortRight();
          state = RIGHT_FEEDBACK;
          drawScreens();
        }
        // ถ้ากดผิดซ้ำ ก็ไม่ต้องทำอะไร
        break;
      }

    case RIGHT_FEEDBACK:
      // ไม่ต้องทำอะไร รอให้ feedback จบ
      break;

    // Case สำหรับหน้าสรุป
    case SHOW_SUMMARY:
      {
        // รอการกดปุ่ม A หรือ B เพื่อเริ่มใหม่
        if (button_pressed == 1 || button_pressed == 2) {
          Serial.println("Restarting quiz...");
          shuffleQuestions();   // สุ่มใหม่ และ รีเซ็ตคะแนน
          state = WAIT_ANSWER;  // กลับไปสถานะรอคำตอบ
          drawScreens();        // วาดคำถามแรก
        }
        break;
      }
  }
}
