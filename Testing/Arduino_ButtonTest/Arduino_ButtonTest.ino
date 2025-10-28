/*
 * โค้ดทดสอบปุ่มกด 2 ปุ่ม (A และ B)
 * แสดงผลเฉพาะตอนสถานะปุ่มเปลี่ยน
 */

// --- Pin Definitions ---
const int BUTTON_A_PIN = 12; // ปุ่ม A (True)
const int BUTTON_B_PIN = 13; // ปุ่ม B (False)

// --- Variables to store the previous button state ---
int lastButtonA_state = HIGH; // สถานะก่อนหน้าของปุ่ม A (เริ่มต้นคือ ไม่กด)
int lastButtonB_state = HIGH; // สถานะก่อนหน้าของปุ่ม B

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Button Test Ready! (Prints on change)");

  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
}

void loop() {
  // อ่านสถานะปัจจุบันของปุ่ม
  int currentButtonA_state = digitalRead(BUTTON_A_PIN);
  int currentButtonB_state = digitalRead(BUTTON_B_PIN);

  // --- ตรวจสอบปุ่ม A ---
  // ถ้าสถานะปัจจุบันไม่เหมือนกับสถานะก่อนหน้า
  if (currentButtonA_state != lastButtonA_state) {
    Serial.print("Button A: ");
    if (currentButtonA_state == LOW) {
      Serial.println("PRESSED");
    } else {
      Serial.println("Released");
    }
    // อัปเดตสถานะก่อนหน้าให้เป็นสถานะปัจจุบัน
    lastButtonA_state = currentButtonA_state;
    delay(50); // Debounce delay
  }

  // --- ตรวจสอบปุ่ม B ---
  // ถ้าสถานะปัจจุบันไม่เหมือนกับสถานะก่อนหน้า
  if (currentButtonB_state != lastButtonB_state) {
    Serial.print("Button B: ");
    if (currentButtonB_state == LOW) {
      Serial.println("PRESSED");
    } else {
      Serial.println("Released");
    }
    // อัปเดตสถานะก่อนหน้าให้เป็นสถานะปัจจุบัน
    lastButtonB_state = currentButtonB_state;
    delay(50); // Debounce delay
  }
}