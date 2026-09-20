# Vaccine Box — STM32F411RE

โครงงานวิชา Microprocessor Systems and Applications: กล่องวัคซีนจำลองบน NUCLEO-F411RE และ Training Shield

อ่านอุณหภูมิและแสงภายในจากเซนเซอร์จริง จำลองอุณหภูมิภายนอกผ่าน Serial Console คำนวณ Risk ก่อนเปิด และแสดง Health ของวัคซีน 3 ช่องเป็นแถบพร้อมเปอร์เซ็นต์บน OLED

> เป็นแบบจำลองเพื่อการเรียน: Health/Risk ไม่ใช่ผลประเมินคุณภาพวัคซีนทางการแพทย์ สถานะและคะแนนเก็บใน RAM และรีเซ็ตเมื่อเริ่มระบบใหม่

## เปิดและ Build

1. ดาวน์โหลดหรือ clone repository นี้
2. เปิด STM32CubeIDE (ตรวจด้วยรุ่น 1.19.0)
3. ใช้ **File → Import → General → Existing Projects into Workspace** เลือกโฟลเดอร์นี้ แล้วเลือกโปรเจกต์ `vaccine`
4. เลือก **Project → Build Project** โดยใช้ configuration `Debug`
5. ต่อบอร์ดผ่าน ST-LINK แล้วสร้าง Run/Debug configuration สำหรับ STM32 เพื่อดาวน์โหลดโปรแกรม

รวม HAL และ CMSIS headers ไว้แล้ว ใช้ include path แบบ relative ไม่ต้องมีโฟลเดอร์ Library ของผู้พัฒนา ไม่มีไฟล์ `.ioc`; โปรเจกต์นี้แก้ไข source และ configuration โดยตรง

## การต่ออุปกรณ์

| อุปกรณ์ | ขา/Peripheral |
|---|---|
| อุณหภูมิภายใน | PA0 / ADC1_IN0 |
| แสงภายใน | PA1 / ADC1_IN1 |
| OLED | PB8 SCL, PB9 SDA / I2C1 |
| Servo สลัก | PB0 / TIM3_CH3 |
| Serial Console | PA2 TX, PA3 RX / USART2 ผ่าน ST-LINK VCP |
| D2 ยืนยัน | PA10 / EXTI15_10 |
| D3 ย้อนกลับหรือลบ | PB3 / EXTI3 |
| D4 ซ้ายหรือลดเลข | PB5 / EXTI9_5 |
| D5 ขวาหรือเพิ่มเลข | PB4 / EXTI4 |

ไม่มีเซนเซอร์ฝาจริง ใช้ D2 ยืนยันสถานะฝาจำลอง และ Servo ควบคุมสลัก ไม่ได้ยกฝา ปัจจุบันใช้ D4/D5 เลือกเมนูและเลขแทน potentiometer

## เริ่มใช้งาน

1. เปิดระบบ ปิดฝาแล้วกด D2 ยืนยัน
2. เลือกเลขด้วย D4/D5 กด D2 ใส่เลข กด D3 ลบ
3. PIN สำหรับการสาธิต: ผู้จ่ายยา `1234`, Admin `2468` (กำหนดใน `Inc/app_policy.h`)
4. เลือกโหมดด้วย D4/D5 แล้วกด D2

| โหมด | หน้าที่ |
|---|---|
| Transport | ล็อกและติดตามสภาพภายใน |
| Dispense | เลือกวัคซีน ตรวจสิทธิ์และ Risk ก่อนจ่าย |
| Service | Admin ดูแลรักษาหรือเติมวัคซีน |

Health ของวัคซีนที่ยังอยู่ในกล่องลดได้ทุกช่องตามสภาพร่วมกัน แม้กำลังเลือกช่องใดช่องหนึ่ง คะแนนที่ลดแล้วไม่ฟื้นเอง การเติมใหม่โดย Admin เริ่ม Health ใหม่

## Web Serial Console

ใช้เว็บ Serial Terminal ในเบราว์เซอร์ที่รองรับ Web Serial เลือกพอร์ต ST-LINK VCP (เลข COM ขึ้นกับเครื่อง) ตั้ง **115200, 8N1** และส่งคำสั่งพร้อม newline

```text
HELP
STATUS
IRQ
SET OUTSIDE_TEMP 35
SET VIAL 1 SENSITIVITY 1.5
LOGOUT
```

คำสั่ง `SET` ต้องล็อกอิน Admin ที่บอร์ด และอยู่หน้า MODE หรือ TRANSPORT ขณะล็อกกล่อง ไม่มีคำสั่ง SET HEALTH

## Interrupt

D2–D5 ใช้ EXTI ทั้งขาขึ้นและขาลง ISR บันทึกขอบสัญญาณ เวลา และคิวปุ่ม ส่วน main loop นำเหตุการณ์ไปประมวลผลตามหน้าปัจจุบัน กดอย่างน้อย 40 ms แล้วปล่อยจึงส่งเหตุการณ์ ไม่ทำงานซ้ำเมื่อกดค้าง

`IRQ` แสดงจำนวนขอบสัญญาณของแต่ละปุ่มและจำนวนเหตุการณ์ที่คิวเต็ม ไม่ใช่จำนวนครั้งที่กด UART RX และ SysTick ใช้ interrupt เช่นกัน

รายละเอียด: [Interrupt](docs/button-interrupt-guide.md), [ปุ่ม D4/D5](docs/D4-D5-navigation.md), [OLED](docs/compact-oled-guide.md)

## โครงสร้างโค้ด

| ไฟล์/โฟลเดอร์ | หน้าที่ |
|---|---|
| `Src/main.c` | เริ่มระบบและ peripheral |
| `Src/app_runtime.c`, `app_state.c` | main-loop งานแอปและ state machine |
| `Src/buttons.c`, `stm32f4xx_it.c` | คิวปุ่มและ interrupt handlers |
| `Src/sensors.c` | อ่านและแปลงค่าเซนเซอร์ |
| `Src/risk_model.c`, `vial_model.c` | แบบจำลอง Risk และ Health |
| `Src/ui_screen.c`, `oled_display.c` | หน้าจอและ OLED driver |
| `Src/uart_console.c`, `console_commands.c` | Serial Console |
| `Src/servo_lock.c`, `pin_auth.c` | สลักและ PIN |
| `Inc/HAL`, `Src/HAL`, `Drivers/CMSIS` | ไลบรารีที่รวมไว้ |

## Third-party software

HAL v1.8.3 และ CMSIS Device F4 v2.6.10 มาจาก STMicroelectronics; CMSIS Core headers มาจาก Library ที่ใช้งานในโปรเจกต์เดิม เก็บ copyright notices ในไฟล์ต้นฉบับไว้ พร้อม license ใน `Inc/HAL`, `Src/HAL`, `Drivers/CMSIS/Core` และ `Drivers/CMSIS/Device/ST/STM32F4xx`

ยังไม่ได้กำหนด license สำหรับโค้ดแอปของผู้ทำโครงงาน
