/*
 * โค้ดทดสอบจอ OLED 2 จอ (เวอร์ชัน Hybrid)
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// --- ตั้งค่าจอที่ 1 (จอเล็ก 0.96 นิ้ว) ---
// ต่อ SCL ที่ D5, SDA ที่ D6
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2_small(U8G2_R0, /* scl=*/5, /* sda=*/6, /* reset=*/U8X8_PIN_NONE);


// --- ตั้งค่าจอที่ 2 (จอใหญ่ 1.3 นิ้ว) ---
// ใช้ Hardware I2C (ขา A4, A5) และ Address 0x78
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2_large(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* address=*/0x78);


void setup(void) {
  Wire.begin();  // เริ่ม Hardware I2C (สำหรับจอใหญ่)

  u8g2_small.begin();  // เริ่มจอเล็ก (Software I2C)
  u8g2_large.begin();  // เริ่มจอใหญ่ (Hardware I2C)
}

void loop(void) {
  // --- 1. วาดบนจอเล็ก ---
  u8g2_small.clearBuffer();
  u8g2_small.setFont(u8g2_font_helvB10_tr);
  u8g2_small.setCursor(0, 20);
  u8g2_small.print("Small: 0.96\"");
  u8g2_small.sendBuffer();

  // --- 2. วาดบนจอใหญ่ ---
  u8g2_large.clearBuffer();
  u8g2_large.setFont(u8g2_font_helvB10_tr);
  u8g2_large.setCursor(0, 20);
  u8g2_large.print("Large: 1.3\"");
  u8g2_large.sendBuffer();

  delay(2000);
}