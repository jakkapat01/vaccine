# Light ADC completion interrupt

แสง PA1 อ่านด้วย ADC1 ผ่าน HAL_ADC_Start_IT และ ADC_IRQHandler → Sensors_ADC_IRQHandler → HAL_ADC_IRQHandler → HAL_ADC_ConvCpltCallback

ISR อ่านข้อมูล ทิ้งตัวอย่างแรก เฉลี่ยอีก 8 ตัวอย่าง แปลงเป็นเปอร์เซ็นต์แบบจำนวนเต็ม ตรวจช่วง MIN/MAX และ hysteresis แล้วตั้ง flag LIGHT ALARM ไม่มีการวาด OLED ส่ง UART หรือคำนวณ float ใน ISR

Main loop รับ flag แล้วแสดง LIGHT ALARM ส่ง Console และบันทึก Log กด D3 รับทราบตามเดิม ระบบจะพร้อมแจ้งรอบใหม่เมื่อแสงกลับเข้าเขตปกติที่เผื่อ hysteresis

อุณหภูมิ PA0 ยังคงใช้ Analog Watchdog ตัวเดียวของ ADC1 การอ่านแสงเปิด EOC/OVR IRQ ชั่วคราวและหยุดด้วย HAL_ADC_Stop_IT ก่อนกลับไปอ่านอุณหภูมิและเปิด background conversion เดิม จึงไม่ทิ้ง EOC IRQ ไว้ในช่วงอ่านอุณหภูมิต่อเนื่อง

## จังหวะการอ่าน

ยังอ่านตามรอบ 500 ms และเมื่อเปลี่ยนช่วงตั้งค่า ไม่ใช่ IRQ ทันทีทุกครั้งที่แสงเปลี่ยน Main รอ flag จาก ISR แบบมี timeout 5 ms ต่อ conversion เช่นเดียวกับขอบเขต timeout ของการอ่านเดิม แต่ไม่ได้ใช้ HAL_ADC_PollForConversion สำหรับแสงอีกแล้ว Interrupt อื่นยังทำงานได้ระหว่างรอ

ขณะ Main ติดงาน OLED ที่เป็น blocking รอบเริ่มอ่านแสงถัดไปอาจช้าได้ การเปลี่ยนนี้ไม่ได้ทำให้การเก็บข้อมูลทั้งหมดเป็น asynchronous และไม่ได้เพิ่ม DMA

## ตรวจสอบบนบอร์ด

1. Build/แฟลชโปรเจกต์ vaccine แล้วเชื่อม Serial 115200 8N1
2. ส่ง IRQ: LIGHT_EOC ต้องเพิ่มตามการอ่าน โดยปกติ 9 ครั้งต่อรอบ (1 discard + 8 average)
3. เข้า Admin Settings → Light Range ตั้งช่วง เช่น 20–80% โดยเลือกช่วงให้ค่าแสงปัจจุบันอยู่ภายในก่อน
4. ปิดหรือส่องเซนเซอร์ให้ค่าออกนอกช่วง ต้องแสดง LIGHT ALARM และ Console แสดง ALARM LIGHT OUT OF RANGE ADC_EOC IRQ
5. LIGHT_ALARM_COUNT เพิ่ม 1 ต่อเหตุการณ์ และไม่เพิ่มซ้ำระหว่างแสงยังผิดช่วง กด D3 รับทราบแล้วกลับหน้าเดิม
6. ให้ค่าแสงกลับเข้า 22–78% สำหรับช่วง 20–80% แล้วนำออกนอกช่วงอีกครั้ง ต้องเตือนได้อีก
7. ทดสอบอุณหภูมิผิดช่วงควบคู่กัน ADC_AWD ยังทำงานได้ และ TEMP ALARM มีลำดับการแสดงก่อน LIGHT ALARM ตามเดิม

ช่วงแสงเริ่มต้น 0–100% ครอบคลุมค่าที่แสดงได้ทั้งหมด จึงต้องตั้งช่วงแคบลงเพื่อทดสอบ Alarm ห้ามตีความ LIGHT_EOC ที่เพิ่มขึ้นว่าเกิด Alarm ทุกครั้ง: ค่านี้นับการแปลง ADC เสร็จ ส่วน LIGHT_ALARM_COUNT นับการออกนอกช่วง

## ผลตรวจซอฟต์แวร์

- ARM emulator: 58 checks ของ Light EOC/temperature AWD/timeout/error/recovery ผ่าน
- Regression ของ sensor faults, ranges, nurse flow, auto opening, TIM2 และ LED: 89 checks ผ่าน
- Build 42 source files ผ่าน และตรวจ -Wall -Wextra -Wconversion -Wfloat-equal -Wuninitialized ไม่มี diagnostics
- ยังไม่ได้แฟลชหรือทดสอบกับฮาร์ดแวร์จริงในการแก้ครั้งนี้

รายงาน CODING_GUIDELINE_REFACTOR.md เป็นผลตรวจของ refactor ก่อนหน้า ส่วนการเปลี่ยนพฤติกรรมการอ่านแสงและการตรวจครั้งนี้บันทึกในไฟล์นี้
