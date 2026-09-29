# ตรวจ Sensor Fault

แยกความผิดปกติของการอ่านเซนเซอร์ออกจากอุณหภูมิ/แสงที่อยู่นอกช่วงปกติ

## สิ่งที่ตรวจ

- ADC initialization/configuration ของ watchdog ไม่สำเร็จ: ADC INIT ERROR และลอง initialize ใหม่ทุก 2 วินาที
- อ่าน/เริ่ม conversion อุณหภูมิไม่สำเร็จ: TEMP READ FAIL
- ADC อุณหภูมิ <=5 หรือ >=4090: TEMP ADC RAIL
- แปลงค่า NTC ไม่ได้ หรือเกินขอบเขต -40 ถึง 125°C: TEMP INVALID
- อ่านแสงไม่สำเร็จ: LIGHT READ FAIL
- ADC แสง <=1 หรือ >=4094 ต่อเนื่อง 3 รอบ: LIGHT ADC RAIL
- ข้อมูลเซนเซอร์เก่าอย่างน้อย 2 วินาที หรือยังไม่มี sample: DATA STALE

ADC rail เป็นข้อสงสัยเกี่ยวกับสัญญาณ ไม่ใช่การยืนยันว่าเซนเซอร์เสียแน่นอน โดยเฉพาะเซนเซอร์แสงอาจอิ่มตัวที่มืด/สว่างสุดได้จริง สายที่หลุดแล้วลอยอยู่กลางช่วง ADC อาจตรวจไม่ได้ และค่าที่นิ่งอยู่เฉย ๆ ไม่ถือว่าเสียโดยอัตโนมัติ

## พฤติกรรม

- แสดงหน้า SENSOR FAULT พร้อมสาเหตุ และบันทึก Console/LOG เมื่อ fault เปลี่ยน ไม่ส่งข้อความรัวทุก main loop
- D3 รับทราบแล้วกลับหน้าที่ค้าง แต่ยังมี FAULT บนหัวจอ
- USE / RETURN ปฏิเสธการเปิดเมื่อเซนเซอร์ใดเสียหรือข้อมูลเก่า แม้กดรับทราบแล้ว
- ถ้าฝาเปิดอยู่ ยังจบงานและปิดฝาได้ ไม่สั่ง Servo ปิดเอง
- Admin ยังเข้า REPLACE VIAL ได้ตามเงื่อนไขฮาร์ดแวร์ เพื่อเข้าถึงกล่อง ส่วน Nurse ไม่มีสิทธิ์นี้
- Fault จากการอ่าน/ค่าผิดปกติต้องอ่านได้ปกติ 3 รอบจึงล้าง (อ่านทุก 500 ms); ข้อมูล stale กลับมาปกติเมื่อมี sample สดและไม่มี fault อื่น
- แสดง SENSOR RECOVERED เมื่อ fault หาย Health ที่เสียไปไม่ฟื้น และ GAP ยังคงบอกว่าประวัติไม่ครบ
- การตรวจ fault ทำใน main loop ไม่ใช่ IRQ ใหม่ ถ้า main loop ค้างทั้งระบบ การแจ้งเตือนจะรอให้ main loop กลับมาทำงาน ยังไม่มี hardware watchdog reset
- Alarm อุณหภูมิจาก ADC interrupt และ Timer/LED เตือนฝายังทำงาน โดยหน้า fault มีลำดับแสดงผลก่อน alarm ค่าผิดช่วง

## Console

STATUS เพิ่ม SENSOR_FAULT เป็น bitmask: 0 ปกติ, 1 ADC, 2 temp read, 4 temp rail, 8 temp conversion, 16 light read, 32 light rail, 64 stale หลายเหตุพร้อมกันจะบวกค่าบิต เช่น 20 = temp rail + light read

## ตรวจสอบ

Build 0 errors / 0 warnings; ARM tests ผ่าน 46 checks (fault classification/recovery/stale/access/close 26 และ ADC mock regression 20)
ยังไม่ได้ Flash หรือทดสอบการถอดสายจริงบนบอร์ด หลังติดตั้ง F5 → Clean/Build → ลงบอร์ดใหม่
