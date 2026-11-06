# SƠ ĐỒ KẾT NỐI THIẾT BỊ (LCD 4-BIT)

## 1. Nguồn cấp

```
Nguồn tổ ong 12 V ─┬──> LM2596 (chỉnh ra 5 V) ──> Đường 5 V ─┬─> Blue Pill (chân 5V)
                   │                                          ├─> LCD (VDD, đèn nền)
                   │                                          ├─> E18-D80NK, module LM393
                   │                                          ├─> Servo SG90
                   │                                          ├─> L298N (chân 5V logic, nếu cần)
                   │                                          └─> AMS1117-3.3 ──> ESP-01 (3,3 V)
                   └──> L298N (cấp nguồn động cơ, xem mục 3 điểm 1)

Tất cả GND nối chung: nguồn 12 V, LM2596, Blue Pill, L298N, servo, LCD, cảm biến, AMS1117, ESP-01, CH340.
```

- Blue Pill cấp 5 V qua chân `5V`, mạch ổn áp trên bo tạo 3,3 V cho STM32.
- ESP-01 cần nguồn 3,3 V riêng từ AMS1117, không lấy từ chân 3,3 V của Blue Pill (dòng đỉnh của ESP-01 có thể lên vài trăm mA).
- Nên đặt một tụ khoảng 470 µF trên đường 5 V gần servo để tránh sụt áp khi servo chạy làm reset MCU.

## 2. Bảng kết nối chi tiết

### 2.1. Cảm biến

| Thiết bị | Chân thiết bị | Nối tới | Ghi chú |
|---|---|---|---|
| E18-D80NK (`CB_Thap`) | Nâu (VCC) | 5 V | |
| | Xanh dương (GND) | GND | |
| | Đen (OUT) | PB10 | Ngõ ra NPN, LOW khi có vật. PB10 có Pull-up nội |
| Module LM393 (`CB_Cao`) | VCC | 5 V | |
| | GND | GND | |
| | OUT | PB11 | LOW khi có vật. Chỉnh biến trở để đặt khoảng cách |

### 2.2. Nút nhấn (6 nút, một chân nối GPIO, chân còn lại nối GND)

| Nút | Chân MCU |
|---|---|
| START | PB12 |
| STOP | PB13 |
| MODE | PB14 |
| RESET | PB15 |
| CONVEYOR | PA12 |
| SERVO | PA11 |

Không cần điện trở ngoài vì đã bật Pull-up nội. Nhấn là nối xuống GND (LOW).

### 2.3. LED và còi

| Thiết bị | Chân MCU | Cách nối |
|---|---|---|
| LED xanh (`LED_RUN`) | PA7 | PA7 → điện trở 330 Ω → anode, cathode → GND |
| LED đỏ (`LED_FAULT`) | PB0 | PB0 → điện trở 330 Ω → anode, cathode → GND |
| Còi SFM-20B (`BUZZER`) | PB1 | PB1 → chân (+) của còi, chân (-) → GND |

Mức HIGH là bật. Còi nối trực tiếp PB1 (đo thực tế dưới 8 mA ở 3,3 V). Nếu còi yếu hoặc PB1 nóng thì thêm transistor NPN và cấp 5 V cho còi.

### 2.4. LCD 16x2, chế độ 4-bit

| Chân LCD | Tên | Nối tới |
|---|---|---|
| 1 | VSS | GND |
| 2 | VDD | 5 V |
| 3 | VO | Giữa biến trở 10 kΩ (hai đầu biến trở nối 5 V và GND), dùng chỉnh độ tương phản |
| 4 | RS | PA1 (`LCD_RS`) |
| 5 | RW | **GND** (chỉ ghi, không đọc) |
| 6 | E | PA2 (`LCD_E`) |
| 7 đến 10 | D0 đến D3 | Bỏ trống |
| 11 | D4 | PA3 (`LCD_D4`) |
| 12 | D5 | PA4 (`LCD_D5`) |
| 13 | D6 | PA5 (`LCD_D6`) |
| 14 | D7 | PA6 (`LCD_D7`) |
| 15 | A (đèn nền +) | 5 V qua điện trở khoảng 220 Ω |
| 16 | K (đèn nền -) | GND |

Lưu ý:
- Bo HW-61 (PCF8574T) đang hàn sẵn vào 16 chân LCD sẽ che các chân này. Để chạy 4-bit thật, bạn cần tháo bo HW-61 hoặc dùng LCD không có bo I2C.
- Vì RW nối GND, LCD không bao giờ đẩy tín hiệu 5 V về MCU.
- Chân dữ liệu của MCU chỉ cho ra mức 3,3 V trong khi LCD chạy 5 V. Thường vẫn đọc được, nếu LCD hiện sai ký tự thì kiểm tra mức logic này trước khi nghi ngờ code.

### 2.5. Băng tải (L298N và động cơ F130)

| Chân L298N | Nối tới | Ghi chú |
|---|---|---|
| IN1 | PB5 (`IN1_L298`) | |
| IN2 | PB4 (`IN2_L298`) | |
| ENA | PB6 (`PWM_CONVEYOR`, TIM4_CH1) | Tháo jumper ENA trên module, nếu để jumper thì ENA luôn mức cao và PWM không có tác dụng |
| OUT1, OUT2 | Hai đầu động cơ F130 | Đảo hai dây nếu băng tải chạy ngược |
| +12V (cấp động cơ) | Xem mục 3 điểm 1 | |
| GND | GND chung | |
| 5V | Xem mục 3 điểm 2 | |

### 2.6. Servo SG90

| Dây servo | Nối tới |
|---|---|
| Đỏ (VCC) | 5 V (từ LM2596, không lấy từ chân 5V của Blue Pill) |
| Nâu/đen (GND) | GND chung |
| Cam/vàng (tín hiệu) | PA0 (`PWM_SERVO`, TIM2_CH1) |

### 2.7. Nạp chương trình và gỡ lỗi (ST-Link V2)

| ST-Link | Blue Pill |
|---|---|
| SWDIO | PA13 |
| SWCLK | PA14 |
| GND | GND |
| 3.3V | 3,3 V (có thể bỏ nếu Blue Pill đang được cấp nguồn riêng) |

### 2.8. UART1 (PA9, PA10): kiểm thử và IoT

| Thiết bị | Chân thiết bị | Nối tới |
|---|---|---|
| CH340 (kiểm thử, xem log) | RXD | PA9 (`USART1_TX`) |
| | TXD | PA10 (`USART1_RX`) |
| | GND | GND |
| ESP-01 (IoT, giai đoạn sau) | RX | PA9 |
| | TX | PA10 |
| | VCC, CH_PD (EN) | 3,3 V từ AMS1117 |
| | GND | GND chung |
| | GPIO0 | Để hở khi chạy bình thường (nối GND khi nạp firmware cho ESP) |

Không nối cùng lúc CH340 và ESP-01 vào PA9/PA10 vì hai thiết bị cùng dùng USART1. CH340 dùng ở giai đoạn kiểm thử, ESP-01 dùng sau này. Không nối chân 5 V của CH340 vào đường 5 V của mạch nếu Blue Pill đã được cấp nguồn riêng.

## 3. Các điểm cần kiểm tra trên mạch thật

1. **Điện áp động cơ F130:** nhiều loại F130 chỉ chịu khoảng 3 đến 6 V. Nếu F130 của bạn là loại này thì không cấp 12 V thẳng vào L298N, mà cấp từ một LM2596 khác chỉnh khoảng 7 V (L298N sụt khoảng 2 V). Bạn xem nhãn động cơ để chọn.
2. **Chân 5V của L298N:** khi cấp động cơ trên 7 V và để jumper 5V-EN, mạch ổn áp trên module tạo ra 5 V cho logic. Nếu cấp động cơ thấp hơn thì phải cấp 5 V từ ngoài vào chân 5V của module.
3. **Chân PB4 (`IN2_L298`):** chỉ dùng được khi Debug chọn Serial Wire (đã đúng trong `.ioc`).
4. **Mức ngõ ra cảm biến:** E18-D80NK và module LM393 chạy 5 V, ngõ ra có thể kéo lên 5 V khi không có vật. PB10 và PB11 chịu được 5 V (chân FT) nên nối trực tiếp được, nhưng bạn nên đo mức khi không có vật trước khi cắm.

## 4. LCD I2C (chưa dùng): ghi chú cách kết nối

Nếu chuyển sang bo I2C HW-61 (PCF8574T) gắn sẵn sau LCD, dùng **I2C1 trên PB8 (SCL) và PB9 (SDA)**:

| Chân HW-61 | Nối tới |
|---|---|
| VCC | 5 V |
| GND | GND |
| SDA | PB9 (`I2C1_SDA`) |
| SCL | PB8 (`I2C1_SCL`) |

Khi dùng LCD I2C:
- Các chân PA1 đến PA6 (`LCD_RS`, `LCD_E`, `LCD_D4` đến `LCD_D7`) được giải phóng, không cần nối gì.
- Bo HW-61 đã có sẵn điện trở kéo lên cho SDA, SCL (thường 4,7 kΩ lên VCC = 5 V). PB8 và PB9 chịu được 5 V nên không cần mạch chuyển mức, nhưng bạn nên kiểm tra bằng đồng hồ.
- Địa chỉ I2C: bo HW-61 thật mặc định `0x27` (khi A0, A1, A2 để hở), còn trong Proteus PCF8574 mặc định `0x20` (như trong `lcd_i2c.h` của bạn). Khi chạy thật cần đổi địa chỉ cho đúng.
- Độ tương phản chỉnh bằng biến trở trên bo, đèn nền bật/tắt bằng jumper trên bo.
- Trong CubeMX chọn PB8, PB9 cho I2C1 (CubeMX tự bật remap), và firmware `lcd.c` chỉ cần thay phần ghi mức thấp.
