/*
 * โค้ดโปรเจกต์ (เวอร์ชัน U8g2 + dw_font)
 * จอใหญ่: Hardware I2C (A4, A5) + dw_font ภาษาไทย
 * จอเล็ก: Software I2C (D5, D6) + U8g2 ภาษาอังกฤษ
 */

#include <Arduino.h>
#include <U8g2lib.h>  // ยังคงต้องใช้ U8g2 สำหรับจอเล็ก และเป็น "พื้น" ให้ dw_font วาดทับ
#include <Wire.h>
#include "dw_font.h"  // Include dw_font library ที่อยู่ในโฟลเดอร์เดียวกัน

// --- ตั้งค่าจอ U8g2 (เหมือนเดิม) ---
// จอเล็ก (Software I2C, SSD1306)
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2_small(U8G2_R0, /* scl=*/5, /* sda=*/6, /* reset=*/U8X8_PIN_NONE);
// จอใหญ่ (Hardware I2C, SH1106, Address 0x78)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2_large(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* address=*/0x78);

// --- ตั้งค่า dw_font ---
extern dw_font_info_t font_th_sarabunpsk_regular40;  // ฟอนต์ไทยขนาด 40
dw_font_t myfont_large;                              // สร้างตัวแปร dw_font สำหรับจอใหญ่

// ฟอนต์อังกฤษสำหรับจอเล็ก (จาก U8g2)
const uint8_t* eng_font = u8g2_font_helvB12_tr;

// --- ฟังก์ชัน Callback สำหรับ dw_font (วาดลงบน Buffer ของ u8g2_large) ---
void draw_pixel_large_u8g2(int16_t x, int16_t y) {
  u8g2_large.drawPixel(x, y);  // ใช้คำสั่งวาดของ U8g2
}
void clear_pixel_large_u8g2(int16_t x, int16_t y) {
  // ไม่จำเป็นต้องทำอะไร เพราะเราจะ clearBuffer() ของ U8g2 ก่อนวาดเสมอ
}

void setup(void) {
  Wire.begin();  // เริ่ม Hardware I2C (สำหรับจอใหญ่)
  u8g2_small.begin();
  u8g2_large.begin();

  // --- เริ่มการทำงาน dw_font (เฉพาะจอใหญ่) ---
  // ใช้ขนาดจอ 128x64 และส่งฟังก์ชัน Callback ที่วาดลง U8g2
  dw_font_init(&myfont_large, 128, 64, draw_pixel_large_u8g2, clear_pixel_large_u8g2);
  dw_font_setfont(&myfont_large, &font_th_sarabunpsk_regular40);  // ตั้งค่าฟอนต์ไทย
}

void loop(void) {
  // --- 1. วาดบนจอใหญ่ (แสดงคำถามภาษาไทย ด้วย dw_font) ---
  u8g2_large.clearBuffer();                    // **สำคัญ:** ล้าง Buffer ของ U8g2 ก่อนเสมอ
  dw_font_goto(&myfont_large, 0, 40);          // ตั้งตำแหน่ง dw_font (ปรับ Y ตามต้องการ)
  dw_font_print(&myfont_large, "วงจรเปิดคือ?");  // พิมพ์ภาษาไทยด้วย dw_font
  u8g2_large.sendBuffer();                     // **สำคัญ:** ส่ง Buffer ของ U8g2 ไปแสดงผล

  // --- 2. วาดบนจอเล็ก (แสดงตัวเลือกภาษาอังกฤษ ด้วย U8g2) ---
  u8g2_small.clearBuffer();
  u8g2_small.setFont(eng_font);  // เลือกฟอนต์อังกฤษ
  u8g2_small.setCursor(10, 25);
  u8g2_small.print("A: True");
  u8g2_small.setCursor(10, 55);
  u8g2_small.print("B: False");
  u8g2_small.sendBuffer();

  delay(5000);
}