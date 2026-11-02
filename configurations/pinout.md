# Danh sách chân STM32F103C8T6
![Pinout_STM32](../images/pinout.png)
| Module | Tên tín hiệu | Chân MCU | Chức năng | Cấu hình |
|---|---|---|---|---|
| Cảm biến | CB_Thap | PB10 | Phát hiện sản phẩm A và B | GPIO Input, Floating (pull-up bên ngoài) |
| | CB_Cao | PB11 | Phát hiện sản phẩm A | GPIO Input, Floating (pull-up bên ngoài) |
| Nút nhấn | START | PB12 | Khởi động hệ thống (`BTN_START`) | GPIO Input, Pull-up |
| | STOP | PB13 | Dừng hệ thống (`BTN_STOP`) | GPIO Input, Pull-up |
| | MODE | PB14 | Chuyển đổi AUTO/MANUAL, chỉ ở `STATE_STOP` (`BTN_MODE`) | GPIO Input, Pull-up |
| | RESET | PB15 | Xóa bộ đếm (`STATE_STOP`) hoặc xóa lỗi (`STATE_ERROR`) (`BTN_RESET`) | GPIO Input, Pull-up |
| | CONVEYOR | PA12 | Bật/tắt băng tải ở `MODE_MANUAL` (`BTN_CONVEYOR`) | GPIO Input, Pull-up |
| | SERVO | PA11 | Chuyển vị trí servo `Non-gate`/`Gate` ở `MODE_MANUAL` (`BTN_SERVO`) | GPIO Input, Pull-up |
| Cảnh báo | LED_RUN | PA7 | Báo trạng thái chạy | GPIO Output Push-pull |
| | LED_FAULT | PB0 | Báo trạng thái lỗi | GPIO Output Push-pull |
| | BUZZER | PB1 | Phát âm thanh cảnh báo lỗi | GPIO Output Push-pull |
| LCD 16x2 | LCD_RS | PA1 | Chọn thanh ghi lệnh/dữ liệu | GPIO Output Push-pull |
| | LCD_E | PA2 | Tạo xung Enable | GPIO Output Push-pull |
| | LCD_D4 | PA3 | Truyền bit dữ liệu D4 | GPIO Output Push-pull |
| | LCD_D5 | PA4 | Truyền bit dữ liệu D5 | GPIO Output Push-pull |
| | LCD_D6 | PA5 | Truyền bit dữ liệu D6 | GPIO Output Push-pull |
| | LCD_D7 | PA6 | Truyền bit dữ liệu D7 | GPIO Output Push-pull |
| Băng tải | PWM_CONVEYOR | PB6 | Xuất PWM điều chỉnh tốc độ động cơ qua chân EN của mạch công suất (TIM4_CH1) | Alternate Function Push-pull, PWM |
| | IN1_L298 | PB5 | Điều khiển đầu vào thứ nhất của mạch công suất | GPIO Output Push-pull |
| | IN2_L298 | PB4 | Điều khiển đầu vào thứ hai của mạch công suất | GPIO Output Push-pull |
| Phân loại | PWM_SERVO | PA0 | Xuất PWM điều khiển servo (TIM2_CH1) | Alternate Function Push-pull, PWM |
| IoT | USART1_TX | PA9 | Gửi dữ liệu từ MCU đến mô-đun Wi-Fi | Alternate Function Push-pull, USART |
| | USART1_RX | PA10 | Nhận dữ liệu từ mô-đun Wi-Fi | USART Input |
| Nạp/Gỡ lỗi | SYS_JTMS-SWDIO | PA13 | Truyền dữ liệu giao tiếp gỡ lỗi/nạp chương trình | SYS, SWD |
| | SYS_JTCK-SWCLK | PA14 | Xung nhịp giao tiếp gỡ lỗi/nạp chương trình | SYS, SWD |

> Ghi chú: PC13 (GPIO_Output) chỉ dùng để kiểm thử, không thuộc yêu cầu hệ thống.
