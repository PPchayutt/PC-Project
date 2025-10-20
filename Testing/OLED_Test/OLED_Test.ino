/*
 * โค้ดทดสอบจอ OLED 2 จอ (0.96" และ 1.3") พร้อมกัน
 * โดยใช้ Software I2C แยก Pin กัน
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// --- ตั้งค่าจอที่ 1 (จอเล็ก 0.96 นิ้ว) ---
// ต่อ SCL ที่ขา 5, ต่อ SDA ที่ขา 6
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2_small(U8G2_R0, /* scl=*/ 5, /* sda=*/ 6, /* reset=*/ U8X8_PIN_NONE);

// --- ตั้งค่าจอที่ 2 (จอใหญ่ 1.3 นิ้ว) ---
// ต่อ SCL ที่ขา 7, ต่อ SDA ที่ขา 8
U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2_large(U8G2_R0, /* scl=*/ 7, /* sda=*/ 8, /* reset=*/ U8X8_PIN_NONE);


void setup(void) {
  // เริ่มการทำงานของจอทั้งสอง
  u8g2_small.begin();
  u8g2_large.begin();
}

void loop(void) {
  // --- 1. วาดบนจอเล็ก ---
  u8g2_small.clearBuffer(); // ล้างจอเล็ก
  u8g2_small.setFont(u8g2_font_helvB10_t_thai); // เลือกฟอนต์ไทย
  u8g2_small.setCursor(0, 20); // ตั้งตำแหน่ง
  u8g2_small.print("จอเล็ก 0.96 นิ้ว");
  u8g2_small.sendBuffer(); // ส่งภาพไปแสดงที่จอเล็ก

  // --- 2. วาดบนจอใหญ่ ---
  u8g2_large.clearBuffer(); // ล้างจอใหญ่
  u8g2_large.setFont(u8g2_font_helvB10_t_thai); // เลือกฟอนต์ไทย
  u8g2_large.setCursor(0, 20); // ตั้งตำแหน่ง
  u8g2_large.print("จอใหญ่ 1.3 นิ้ว");
  u8g2_large.sendBuffer(); // ส่งภาพไปแสดงที่จอใหญ่

  // หน่วงเวลา 2 วินาที ก่อนจะวาดใหม่ในรอบถัดไป
  delay(2000);
}