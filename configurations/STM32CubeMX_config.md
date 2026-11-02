# Cấu hình CubeMX

Tài liệu này tóm tắt cấu hình STM32CubeMX của dự án để người đọc (và công cụ AI) hiểu nhanh mà không cần đọc toàn bộ file `.ioc`.

Nguồn dữ liệu: `test_feature.ioc`, `gpio.c`, `tim.c`, `usart.c`, `stm32f1xx_it.c`. Các mục ghi "(bổ sung)" chưa có thông tin trong các file này.

## 1. Thông tin chung

| Mục | Giá trị |
|---|---|
| MCU | STM32F103C8T6, LQFP48 |
| Phiên bản công cụ | STM32CubeMX 6.18.0, STM32Cube FW_F1 V1.8.7 |
| Tên dự án trong `.ioc` | `test_feature` |
| Toolchain | MDK-ARM V5.32 (Keil) |
| Vị trí `main.c` | `Core/Src` |
| Mỗi ngoại vi một cặp `.c/.h` | Có (`gpio.c`, `tim.c`, `usart.c`) |
| Giữ code người dùng khi tạo lại | Có (chỉ viết trong các vùng `USER CODE`) |
| Stack / Heap | `0x400` / `0x200` |
| Debug (SYS) | Serial Wire: PA13 = SWDIO, PA14 = SWCLK |
| Time base của HAL | SysTick |

Ghi chú: ở chế độ Serial Wire, các chân PB3, PB4 và PA15 được giải phóng khỏi JTAG. Dự án dùng PB4 làm `IN2_L298`, vì vậy phải giữ Debug = Serial Wire.

## 2. Clock

| Mục | Giá trị |
|---|---|
| Nguồn clock hệ thống | HSI 8 MHz, không dùng PLL (suy ra từ `.ioc`; xác nhận trong `SystemClock_Config()`) |
| SYSCLK / HCLK | 8 MHz |
| APB1 / APB2 | 8 MHz / 8 MHz (hệ số chia bằng 1) |
| Clock cấp cho timer | 8 MHz |
| Proteus | Thuộc tính Clock Frequency của chip phải đặt 8 MHz |

## 3. Timer

Công thức: `f = f_timer / ((Prescaler + 1) × (Period + 1))`, với `f_timer` = 8 MHz.

| Timer | Vai trò | Prescaler | Period | Kết quả | Ngắt |
|---|---|---|---|---|---|
| TIM1 | Tick 1 ms cho `Timer_GetTick()` | 0 | 7999 | 1000 Hz, chu kỳ 1 ms | Update, `TIM1_UP_IRQn`, ưu tiên 0 |
| TIM2_CH1 (PA0, `PWM_SERVO`) | PWM điều khiển servo | 79 | 1999 | 50 Hz, chu kỳ 20 ms; 1 đếm = 10 µs (xung 1 ms = 100 đếm, 2 ms = 200 đếm) | Không |
| TIM4_CH1 (PB6, `PWM_CONVEYOR`) | PWM tốc độ băng tải (chân EN của L298) | 0 | 199 | 40 kHz; 200 mức duty (0,5% mỗi mức) | Không |

Cấu hình chung cho cả ba timer: đếm lên, nguồn clock nội, `ClockDivision = DIV1`, `AutoReloadPreload` tắt. Kênh PWM của TIM2 và TIM4 dùng chế độ PWM1, cực tính cao, xung ban đầu bằng 0 (`Pulse = 0`).

Các việc CubeMX không tự làm, phải có trong code người dùng:

- `HAL_TIM_Base_Start_IT(&htim1);` để TIM1 chạy và sinh ngắt.
- `HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);` và `HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);` để xuất PWM.
- Định nghĩa `HAL_TIM_PeriodElapsedCallback()` (hiện nằm trong `timer.c`) và lọc theo `htim->Instance == TIM1`.

Giá trị `Pulse` ứng với hai vị trí servo (`Non-gate`, `Gate`) và tốc độ băng tải chưa được chốt.

## 4. NVIC

| Ngắt | Ưu tiên (preemption/sub) | Ghi chú |
|---|---|---|
| `TIM1_UP_IRQn` | 0 / 0 | Cập nhật bộ đếm mili giây `g_msTicks` |
| `SysTick_IRQn` | 15 / 0 | Cấp tick cho `HAL_Delay()` và `HAL_GetTick()` |
| Các ngắt lỗi hệ thống | 0 / 0 | Mặc định của CubeMX |

Priority group: `NVIC_PRIORITYGROUP_4` (4 bit preemption, không có sub-priority).

Lưu ý:

- Có hai nguồn tick 1 ms độc lập: SysTick (cho HAL) và TIM1 (cho `Timer_GetTick()`).
- Không gọi `HAL_Delay()` trong trình phục vụ ngắt TIM1: SysTick có ưu tiên thấp hơn nên không thể ngắt TIM1, và `HAL_Delay()` sẽ bị treo.

## 5. USART1 (PA9 = TX, PA10 = RX)

| Mục | Giá trị |
|---|---|
| Chế độ | Asynchronous, TX/RX |
| Tốc độ | 115200 baud, 8 bit, không parity, 1 stop bit |
| Điều khiển luồng | Không |
| Oversampling | 16 |
| Ngắt / DMA | Không dùng (truyền nhận theo kiểu chặn) |
| Chân | PA9: Alternate Function Push-pull; PA10: Input, không kéo |

Hiện được dùng để kiểm thử qua Virtual Terminal trong Proteus. Giao tiếp với ESP8266 (IoT) chưa triển khai.

## 6. GPIO

Chi tiết từng chân xem `pinout.md`. Cấu hình chung (từ `gpio.c`):

| Nhóm | Chân | Cấu hình |
|---|---|---|
| Đầu ra số | `LCD_RS`, `LCD_E`, `LCD_D4` đến `LCD_D7`, `LED_RUN`, `LED_FAULT`, `BUZZER`, `IN1_L298`, `IN2_L298` | Output Push-pull, tốc độ thấp, không kéo, mức ban đầu LOW |
| Đầu vào nút nhấn | `START`, `STOP`, `MODE`, `RESET` (PB12 đến PB15), `CONVEYOR` (PA12), `SERVO` (PA11) | Input, Pull-up (nút nối xuống GND, tích cực mức thấp) |
| Đầu vào cảm biến | `CB_Thap` (PB10), `CB_Cao` (PB11) | Input, Pull-up (nội) |
| PWM | `PWM_SERVO` (PA0), `PWM_CONVEYOR` (PB6) | Alternate Function Push-pull, tốc độ thấp |
| Chân kiểm thử | PC13 | Output Push-pull, chỉ dùng để thử, không thuộc yêu cầu hệ thống |

Mức ban đầu LOW cho mọi đầu ra đảm bảo khi cấp điện: băng tải không quay (`IN1_L298` = `IN2_L298` = 0), LED và còi tắt.

## 7. Thứ tự khởi tạo (theo `.ioc`)

`SystemClock_Config()` → `MX_GPIO_Init()` → `MX_TIM1_Init()` → `MX_USART1_UART_Init()` → `MX_TIM2_Init()` → `MX_TIM4_Init()`

## 8. Keil (bổ sung)

- Tùy chọn tối ưu hóa: (bổ sung)
- MicroLIB: (bổ sung)
- Create HEX File: (bổ sung; cần bật để nạp vào Proteus)
- Macro định nghĩa: `USE_HAL_DRIVER`, `STM32F103xB` (xác nhận trong `.uvprojx`)
