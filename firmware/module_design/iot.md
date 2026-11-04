# MODULE IOT

## 1. Overview

Module `IoT` gửi thông tin giám sát của hệ thống ra USART1 (PA9 = TX) dưới dạng một dòng văn bản, định kỳ 3 giây một lần.

- Phía nhận là module ESP8266 ESP-01 chạy firmware riêng (kết nối Wi-Fi, đẩy dữ liệu lên giao diện IoT). STM32 không điều khiển ESP bằng lệnh AT và không xử lý Wi-Fi.
- Hiện tại USART1 được dùng để kiểm thử bằng Virtual Terminal trong Proteus. Việc nối ESP-01 thật chưa triển khai; khi nối, API của module không đổi.
- Module chỉ gửi, không nhận lệnh. IoT chỉ phục vụ giám sát, việc điều khiển và quản lý bộ đếm vẫn do STM32 đảm nhiệm.

Module `IoT` không đọc `status` hay `data`. `main` truyền toàn bộ thông tin cần gửi qua tham số của `IoT_Update()`. Module chỉ dùng các kiểu dữ liệu của `status.h` (`SystemState_t`, `SystemMode_t`, `ErrorCode_t`), không gọi hàm nào của `status`. Cách tổ chức giống module `LCD`.

## 2. Responsibilities

- Khởi tạo mốc thời gian gửi.
- Mỗi 3 giây dựng một dòng văn bản từ các giá trị nhận được và gửi qua USART1.

Module không chịu trách nhiệm:

- Đọc trạng thái, chế độ, mã lỗi, bộ đếm (do `main` truyền vào).
- Cấu hình USART1 (do CubeMX, `MX_USART1_UART_Init()`).
- Giao tiếp Wi-Fi, lệnh AT, firmware của ESP-01.
- Nhận lệnh từ ESP-01 (chân RX của USART1 chưa dùng).
- Gửi ngay khi có thay đổi trạng thái hoặc lỗi (chỉ gửi định kỳ).

## 3. Input / Output

### 3.1. Input

| Input | Mô tả |
|---|---|
| `state` (tham số `IoT_Update()`) | Trạng thái hệ thống hiện tại |
| `mode` | Chế độ hiện tại (`MODE_AUTO` hoặc `MODE_MANUAL`) |
| `error` | Mã lỗi hiện tại |
| `total`, `countA`, `countB` | Giá trị bộ đếm từ RAM (module `data`) |
| `Timer_GetTick()` / `Timer_IsElapsed()` | Lấy thời gian từ module `timer` để tính chu kỳ gửi |

### 3.2. Output

| Output | Mô tả |
|---|---|
| `USART1_TX` (PA9) | Một dòng văn bản kết thúc bằng `\r\n`, 115200 baud, 8N1 |

## 4. API Design

### 4.1. Danh sách API

```c
void IoT_Init(void);
void IoT_Update(SystemState_t state,
                SystemMode_t  mode,
                ErrorCode_t   error,
                uint32_t      total,
                uint32_t      countA,
                uint32_t      countB);
```

### 4.2. Chức năng từng API

| API | Chức năng |
|---|---|
| `IoT_Init()` | Đặt `last_send_tick = Timer_GetTick()`. Gọi một lần khi khởi động, sau `MX_USART1_UART_Init()` và `Timer_Init()`. Lần gửi đầu tiên diễn ra sau 3 giây. |
| `IoT_Update()` | `main` gọi mỗi vòng lặp. Nếu đã đủ `IOT_SEND_PERIOD_MS` kể từ lần gửi trước: dựng dòng văn bản, gửi bằng `HAL_UART_Transmit()`, ghi lại mốc thời gian. Ngược lại không làm gì. |

Hàm tạo dòng văn bản và hàm đổi số sang chuỗi là hàm `static` trong `iot.c`, `main` không gọi trực tiếp.

### 4.3. Ví dụ sử dụng trong `main`

```c
Timer_Init();
Data_Init();
Status_Init();
IoT_Init();

/* Mỗi vòng lặp */
IoT_Update(Status_GetState(),
           Status_GetMode(),
           Status_GetError(),
           Data_GetCountTotal(),
           Data_GetCountA(),
           Data_GetCountB());
```

`main` không cần biết đã đến lúc gửi chưa. Gọi `IoT_Update()` mỗi vòng lặp là đủ.

## 5. Internal Design

### 5.1. Định dạng dòng gửi

Văn bản dạng `KEY=VALUE`, các trường cách nhau bằng dấu `;`, kết thúc bằng `\r\n`:

```
STATE=RUNNING;MODE=AUTO;ERR=NONE;T=120;A=70;B=50
```

| Trường | Giá trị |
|---|---|
| `STATE` | `STOP`, `RUNNING`, `ERROR` |
| `MODE` | `AUTO`, `MANUAL` |
| `ERR` | `NONE`, `TIMEOUT`, `CONT_ACTIVE` |
| `T` | Tổng số sản phẩm (`count_total`) |
| `A` | Số sản phẩm loại A |
| `B` | Số sản phẩm loại B |

- Luôn gửi đủ sáu trường, thứ tự cố định.
- Số được gửi ở dạng thập phân đầy đủ (không giới hạn 5 chữ số như LCD).
- Mã lỗi dùng cùng tên với LCD (`TIMEOUT`, `CONT_ACTIVE`).
- Đổi số sang chuỗi bằng hàm tự viết (vài dòng), không dùng `sprintf`, để không phụ thuộc cấu hình MicroLIB của Keil.

### 5.2. Biến nội bộ

| Biến | Kiểu | Chức năng |
|---|---|---|
| `last_send_tick` | `uint32_t` | Mốc thời gian của lần gửi gần nhất (hoặc lúc `IoT_Init()`) |

Không lưu giá trị đã gửi lần trước vì không so sánh thay đổi.

### 5.3. Logic `IoT_Update()`

1. Nếu `Timer_IsElapsed(last_send_tick, IOT_SEND_PERIOD_MS)` sai: thoát.
2. `last_send_tick = Timer_GetTick()`.
3. Dựng dòng văn bản vào bộ đệm `char buf[IOT_BUF_SIZE]`.
4. `HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, IOT_TX_TIMEOUT_MS)`.

Kết quả của `HAL_UART_Transmit()` không được dùng. Nếu gửi lỗi hoặc hết thời gian thì bỏ qua, lần gửi sau sẽ là 3 giây tiếp theo.

### 5.4. Kiểu gửi

- Gửi kiểu chặn: `HAL_UART_Transmit()` chờ gửi xong mới trả về. Không dùng ngắt hoặc DMA nên không phải sửa `.ioc`.
- Thời gian chặn: 115200 baud, 10 bit mỗi byte (khoảng 87 µs). Dòng điển hình khoảng 50 ký tự mất khoảng 4,3 ms; dòng dài nhất (ba số 10 chữ số, `MANUAL`, `CONT_ACTIVE`) khoảng 82 ký tự mất khoảng 7 ms.
- Trong thời gian chặn, `main` không chạy được (nút, cảm biến, `Servo_Update()`). Với chu kỳ gửi 3 giây thì chấp nhận được: nhỏ hơn nhiều so với T1 của `sensor` (50 ms) và `T_delay`, `T_hold` của servo.
- Ngắt TIM1 vẫn chạy trong lúc chặn nên `Timer_GetTick()` không bị lệch.

### 5.5. Tham số cấu hình

| Tham số | Ý nghĩa | Giá trị tạm |
|---|---|---|
| `IOT_SEND_PERIOD_MS` | Chu kỳ gửi (ms) | 3000 |
| `IOT_TX_TIMEOUT_MS` | Thời gian chờ tối đa của `HAL_UART_Transmit()` (ms) | 20 |
| `IOT_BUF_SIZE` | Kích thước bộ đệm dòng gửi (byte) | 96 |

Khai báo bằng `#define` hoặc `static const` trong `iot.c`.

### 5.6. Quy ước

- Khi hệ thống vào `STATE_ERROR` hoặc đổi trạng thái, phía IoT có thể nhận thông tin trễ tối đa 3 giây. Chấp nhận được vì IoT chỉ phục vụ giám sát. Nếu sau này cần báo ngay, thêm điều kiện gửi khi `state` hoặc `error` thay đổi (cần lưu giá trị lần gửi trước), không đổi API.
- Module gửi ở mọi trạng thái (`STATE_STOP`, `STATE_RUNNING`, `STATE_ERROR`).
- Hàm không tự kiểm tra USART1 đã khởi tạo. Phải gọi sau `MX_USART1_UART_Init()`.

## 6. Dependencies

| Thành phần | Quan hệ | Mô tả |
|---|---|---|
| `main` | Gọi API | Gọi `IoT_Init()` một lần, gọi `IoT_Update()` mỗi vòng lặp và truyền dữ liệu |
| `status.h` | Phụ thuộc kiểu dữ liệu | `SystemState_t`, `SystemMode_t`, `ErrorCode_t`; không gọi hàm của `status` |
| `timer` | Phụ thuộc | `Timer_GetTick()`, `Timer_IsElapsed()` để tính chu kỳ gửi |
| `usart.h` | Phụ thuộc | Dùng `huart1` |
| STM32 HAL | Phụ thuộc | `HAL_UART_Transmit()` |
| `data`, `sensor`, `conveyor`, `servo`, `indicator`, `LCD` | Không phụ thuộc | `main` lấy dữ liệu rồi truyền vào `IoT_Update()` |

## 7. Testing

### Test 1: Khởi tạo

- Gọi `IoT_Init()`.
- Kiểm tra chưa gửi gì ngay sau khởi tạo.

### Test 2: Gửi định kỳ

- Gọi `IoT_Update()` liên tục mỗi vòng lặp.
- Kiểm tra trên Virtual Terminal: một dòng xuất hiện mỗi 3 giây, lần đầu sau 3 giây kể từ `IoT_Init()`.

### Test 3: Định dạng dòng

- Kiểm tra dòng đúng dạng `STATE=...;MODE=...;ERR=...;T=...;A=...;B=...` kết thúc `\r\n`, đủ sáu trường theo thứ tự.

### Test 4: Các giá trị trạng thái, chế độ, lỗi

- Truyền lần lượt `STATE_STOP`, `STATE_RUNNING`, `STATE_ERROR`; `MODE_AUTO`, `MODE_MANUAL`; `ERROR_NONE`, `ERROR_TIMEOUT`, `ERROR_CONTINUOUSLY_ACTIVE`.
- Kiểm tra từng giá trị được in đúng tên (`STOP`, `RUNNING`, `ERROR`, `AUTO`, `MANUAL`, `NONE`, `TIMEOUT`, `CONT_ACTIVE`).

### Test 5: Bộ đếm

- Truyền bộ đếm bằng 0, giá trị một chữ số, nhiều chữ số và giá trị lớn (ví dụ 4294967295).
- Kiểm tra số in đúng, không bị cắt hoặc thừa ký tự, dòng không tràn bộ đệm.

### Test 6: Không chặn quá lâu

- Đo thời gian một lần gửi (ví dụ bật chân kiểm thử PC13 quanh `HAL_UART_Transmit()`).
- Kiểm tra khoảng 4 đến 7 ms, `Servo_Update()`, nút nhấn và cảm biến vẫn hoạt động bình thường.

### Test 7: Chu kỳ không lệch tick

- Chạy lâu, kiểm tra chu kỳ gửi vẫn khoảng 3 giây (TIM1 vẫn đếm đúng trong lúc gửi).

## Các thông số và quyết định chưa chốt

| Nội dung | Trạng thái |
|---|---|
| Firmware ESP-01 | Viết riêng, nạp bằng USB-to-TTL (CH340). Chưa thiết kế |
| Nối ESP-01 thật | Chưa làm. ESP-01 cần nguồn 3,3 V riêng (AMS1117-3.3), chung GND với STM32 |
| Định dạng dòng gửi | Văn bản `KEY=VALUE;...`. Có thể đổi sang JSON sau nếu phía ESP cần, chỉ sửa trong `iot.c` |
| Chu kỳ gửi | 3000 ms, chỉnh khi thử |
| Gửi khi có thay đổi (state, error) | Không làm ở bản hiện tại |
| Nhận lệnh từ ESP-01 | Không làm. Chân PA10 (RX) chưa dùng |
| Kiểu gửi | Chặn (`HAL_UART_Transmit()`). Chỉ đổi sang ngắt hoặc DMA nếu đo thấy ảnh hưởng |
| Tên mã lỗi | `TIMEOUT`, `CONT_ACTIVE`, thống nhất với `LCD` |
| Kiểm thử hiện tại | Virtual Terminal trong Proteus trên USART1 |
