# Vaccine Box — STM32F411RE

โครงงานกล่องวัคซีนจำลอง NUCLEO-F411RE + Training Shield เวอร์ชัน 29 กันยายน 2026

อ่านอุณหภูมิและแสงจริง แสดง Health V1–V3 บน OLED ควบคุม Servo และแจ้งเตือนด้วย ADC/TIM2 Interrupt เป็นแบบจำลองเพื่อการเรียน Health ไม่ใช่ผลตรวจคุณภาพวัคซีนทางการแพทย์

## Build

STM32CubeIDE 1.19.0 → File → Import → Existing Projects into Workspace → เลือกโฟลเดอร์นี้ → project `vaccine` → Build Debug

รวม HAL และ CMSIS headers พร้อม relative include paths แล้ว ไม่มี `.ioc` และไม่รวมไฟล์ Debug/Release หรือ launch configuration เฉพาะเครื่อง

## ใช้งาน

หน้า HOME แสดงอุณหภูมิ แสง และแถบ Health กด D2 เข้าหน้า PIN (ตอนเริ่มระบบให้ปิดฝาตามข้อความก่อน)

| ปุ่ม | หน้าที่ |
|---|---|
| D2 | ขอเปิด / ยืนยันเลขหรือเมนู |
| D3 กดสั้น | ลบ / ย้อนกลับ / รับทราบ Alarm |
| D4 / D5 | เลื่อนซ้าย–ขวา / ลด–เพิ่มค่า |

- Nurse PIN เริ่มต้น `1234`: เข้าเลือกหลอดสำหรับ USE / RETURN โดยตรง
- Admin PIN เริ่มต้น `2468`: TRANSPORT, USE / RETURN, REPLACE VIAL, ADMIN SETTINGS
- Transport ต้องใช้ Admin ทั้งเข้าและออก พยาบาลไม่สามารถข้ามด้วยปุ่มย้อนกลับหรือ Logout
- Use / Return สมมติว่าใช้แล้วคืนหลอดเดิม Health ไม่รีเซ็ตและไม่มี OUT
- Admin Replace Vial ยืนยันใส่หลอดใหม่ แล้ว Health เริ่ม 100% หลังยืนยันปิด
- ระบบตรวจ Risk <70, เซนเซอร์พร้อม และสถานะหลอด ก่อนเปิดใช้
- ส่งคำสั่ง Servo เปิดสำเร็จแล้วเริ่มจับเวลาทันที ไม่ต้องยืนยัน OPENED อีก
- D2 จบงาน → ปิดฝาและ D2 ยืนยัน → Servo ปิดและกลับ HOME
- ยังไม่มีเซนเซอร์ตำแหน่งฝาหรืออุปกรณ์ระบุตัวหลอด จึงยืนยันการเคลื่อนที่/ตัวตนหลอดจริงไม่ได้

## Admin Settings บน OLED

เมนูแสดงครั้งละ 3 รายการ เลื่อนต่อเพื่อดูรายการท้าย

- OPEN TIME: 10–300 วินาที
- NURSE PIN / ADMIN PIN: กรอกใหม่ 4 หลักสองครั้งให้ตรงกัน และสองบทบาทต้องไม่ใช้รหัสเดียวกัน
- TEMP RANGE: MIN–MAX ระหว่าง -40 ถึง 125°C เริ่มต้น 2–8°C
- LIGHT RANGE: MIN–MAX ระหว่าง 0 ถึง 100% เริ่มต้น 0–100% ไม่ใช่ค่า lux

MIN ต้องต่ำกว่า MAX การแก้มีผลเมื่อกด SAVE ช่วงอุณหภูมิใช้กับ Analog Watchdog และสูตร Risk/Health ส่วนช่วงแสงใช้แจ้งเตือน การตั้งค่าไม่ฟื้น Health เดิม

## Alarm และ Sensor Fault

- ADC Analog Watchdog: อุณหภูมิภายในผิดช่วง ตั้งธงใน ISR แล้วแสดง TEMP ALARM ใน main loop มี hysteresis ก่อนเตือนรอบใหม่
- Light Alarm: ตรวจจากค่าที่อ่านใน main loop ไม่ใช่ ADC IRQ แยกสำหรับแสง
- TIM2: Timer interrupt ทุกวินาทีระหว่างเปิด เมื่อหมดเวลาแสดง TIMEOUT และ LED 4 ดวงกระพริบ ไม่ปิดฝาเอง
- Sensor Fault: ADC/init/read error, ค่าติดขอบ ADC, ค่าอุณหภูมิแปลงไม่ได้ และข้อมูลเก่าอย่างน้อย 2 วินาที
- D3 รับทราบ Fault ได้แต่ยังเปิด USE / RETURN ไม่ได้จนกลับมาปกติ ฝาที่เปิดอยู่ยังปิดได้
- ADC rail เป็นสัญญาณน่าสงสัย ไม่ยืนยันสายขาดทุกกรณี โดยเฉพาะแสงอิ่มตัวอาจเป็นสภาพจริง

## Peripheral / Pins

| อุปกรณ์ | ขา / Peripheral |
|---|---|
| อุณหภูมิ / แสง | PA0 / PA1, ADC1 |
| OLED | PB8 SCL / PB9 SDA, I2C1 |
| Servo | PB0, TIM3 CH3 PWM 50 Hz |
| D2 / D3 / D4 / D5 | PA10 / PB3 / PB5 / PB4, EXTI |
| LED 4 ดวง | PA5 / PA6 / PA7 / PB6, active high |
| Serial | PA2 TX / PA3 RX, USART2 ผ่าน ST-LINK VCP |
| จับเวลาเปิด | TIM2 interrupt |
| ฐานเวลา | SysTick 1 ms |

## Console

Web Serial Terminal, 115200 8N1 พร้อม newline เลือก COM ของ ST-LINK ตามเครื่อง

```text
STATUS
IRQ
CONFIG
LOG 0
LOG 1
LOG 2
LOG 3
SET OPEN_SECONDS 30
SET PIN NURSE 1357
SET PIN ADMIN 8642
SET OUTSIDE_TEMP 35
SET VIAL 1 SENSITIVITY 1.5
LOGOUT
```

CONFIG, LOG และ SET ต้องล็อกอิน Admin ที่บอร์ดและอยู่หน้า MODE/TRANSPORT ขณะล็อก ส่วน STATUS/IRQ อ่านได้ทั่วไป LOG เก็บ 16 รายการล่าสุด เวลาเป็น ms ตั้งแต่เริ่มเครื่อง ไม่เก็บค่า PIN ในประวัติ

ค่าตั้ง PIN Health และประวัติอยู่ RAM ทั้งหมด รีเซ็ตแล้วกลับค่าเริ่มต้น ยังไม่มี Flash persistence, RTC, RFID, สวิตช์ฝาจริง หรือเว็บ Dashboard เฉพาะโครงงาน

## เอกสาร

- [Sensor Fault](docs/sensor-fault-guide.md)
- [ตั้งช่วงเซนเซอร์](docs/sensor-ranges-guide.md)
- [Servo เปิดอัตโนมัติและ LED](docs/auto-open-led-guide.md)
- [Admin Settings](docs/oled-admin-settings-guide.md)

เฟิร์มแวร์ชุด Sensor Fault ผ่าน ARM emulator / HAL mocks 46 checks; การทดสอบจำลองไม่แทนการทดสอบสายเซนเซอร์และกลไกจริงบนบอร์ด

## Third-party licenses

HAL v1.8.3 และ CMSIS Device F4 v2.6.10 จาก STMicroelectronics; CMSIS Core จาก Library เดิม เก็บ copyright notices และ license ใน Inc/HAL, Src/HAL และ Drivers/CMSIS ไว้ ยังไม่ได้กำหนด license สำหรับโค้ดแอปของโครงงาน
