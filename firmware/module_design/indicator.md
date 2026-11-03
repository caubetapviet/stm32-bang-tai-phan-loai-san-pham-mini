# MODULE INDICATOR

## 1. Overview

Module `indicator` điều khiển các thiết bị báo hiệu của hệ thống:

- `LED_RUN` (PA7): báo hệ thống đang chạy.
- `LED_FAULT` (PB0): báo lỗi.
- `BUZZER` (PB1): còi cảnh báo lỗi (SFM-20B, còi chủ động).

Cả ba thiết bị đều bật khi chân GPIO ở mức HIGH.

Module `indicator` không biết trạng thái hệ thống. `main` đọc `status` rồi ra lệnh bật/tắt, giống `conveyor` và `servo`.

## 2. Responsibilities

- Khởi tạo ba thiết bị ở trạng thái tắt.
- Bật/tắt `LED_RUN`.
- Bật/tắt đồng thời `LED_FAULT` và `BUZZER`.

Module không chịu trách nhiệm:

- Quyết định bật/tắt theo `STATE_*`, `ERROR_*`.
- Đảm bảo `LED_RUN` và `LED_FAULT` không sáng cùng lúc (do `main` quyết định).
- Tạo kiểu kêu ngắt quãng cho còi, nhấp nháy LED.
- Đọc nút nhấn, đọc cảm biến, tính thời gian.

## 3. Input / Output

### 3.1. Input

| Input | Mô tả |
|---|---|
| `Indicator_SetRun(on)` | `1U`: bật `LED_RUN`; `0U`: tắt |
| `Indicator_SetFault(on)` | `1U`: bật `LED_FAULT` và `BUZZER`; `0U`: tắt cả hai |

### 3.2. Output

| Output | Mô tả |
|---|---|
| `LED_RUN` (PA7) | HIGH: sáng |
| `LED_FAULT` (PB0) | HIGH: sáng |
| `BUZZER` (PB1) | HIGH: kêu liên tục |

## 4. API Design

### 4.1. Danh sách API

```c
void Indicator_Init(void);
void Indicator_SetRun(uint8_t on);
void Indicator_SetFault(uint8_t on);
```

### 4.2. Chức năng từng API

| API | Chức năng |
|---|---|
| `Indicator_Init()` | Ghi mức tắt cho `LED_RUN`, `LED_FAULT`, `BUZZER`. Gọi một lần khi khởi động, sau `MX_GPIO_Init()`. |
| `Indicator_SetRun()` | `on != 0U`: bật `LED_RUN`; `on == 0U`: tắt. |
| `Indicator_SetFault()` | `on != 0U`: bật `LED_FAULT` và `BUZZER` cùng lúc; `on == 0U`: tắt cả hai. |

### 4.3. Ví dụ sử dụng trong `main`

```c
Indicator_Init();

/* Nút START */
if (Status_Start())
{
    Indicator_SetRun(1U);
}

/* Nút STOP */
if (Status_Stop())
{
    Indicator_SetRun(0U);
}

/* Khi vừa vào STATE_ERROR */
if (Status_Update(conveyorRunning, s.cb_thap_active, s.cb_cao_active, classified))
{
    Indicator_SetRun(0U);
    Indicator_SetFault(1U);
}

/* Nút RESET ở STATE_ERROR */
else if (Status_ClearError())
{
    Indicator_SetFault(0U);
}
```

## 5. Internal Design

### 5.1. Quy ước hoạt động

- `LED_RUN` sáng liên tục suốt `STATE_RUNNING` (cả `MODE_AUTO` và `MODE_MANUAL`, kể cả khi băng tải đang tắt ở `MODE_MANUAL`). Tắt ở `STATE_STOP` và `STATE_ERROR`.
- `LED_FAULT` sáng liên tục suốt `STATE_ERROR`.
- `BUZZER` kêu liên tục suốt `STATE_ERROR`. Không có kiểu kêu ngắt quãng.
- `LED_FAULT` và `BUZZER` luôn được bật/tắt cùng nhau trong một hàm, theo yêu cầu hệ thống (mục 4.2 và 6.2): khi lỗi, đèn và còi cùng hoạt động.
- Hàm an toàn khi gọi lặp lại.
- Module không tự loại trừ `LED_RUN` và `LED_FAULT`. Khi vào `STATE_ERROR`, `main` gọi `Indicator_SetRun(0U)` trước, rồi `Indicator_SetFault(1U)`.
- Nếu sau này cần tắt còi riêng (ví dụ nút mute) thì thêm hàm `Indicator_SetBuzzer()`, không ảnh hưởng code cũ.

### 5.2. Mức tích cực

Mức bật là HIGH (`GPIO_PIN_SET`), đúng với mạch thực tế (GPIO xuất HIGH thì LED và còi hoạt động). Mức bật/tắt được định nghĩa bằng macro trong `indicator.c`:

```c
#define INDICATOR_ON   GPIO_PIN_SET
#define INDICATOR_OFF  GPIO_PIN_RESET
```

Nếu phần cứng đổi sang tích cực mức thấp thì chỉ cần đổi hai macro này, không đổi API.

### 5.3. Khởi tạo

`MX_GPIO_Init()` đã đặt ba chân ở mức LOW, nên khi cấp điện đèn và còi không hoạt động. `Indicator_Init()` ghi lại mức tắt một lần để chắc chắn.

## 6. Dependencies

| Thành phần | Quan hệ | Mô tả |
|---|---|---|
| `main` | Gọi API | Bật/tắt `LED_RUN`, `LED_FAULT` và `BUZZER` theo trạng thái hệ thống |
| `main.h` | Phụ thuộc | Nhãn chân `LED_RUN_*`, `LED_FAULT_*`, `BUZZER_*` |
| STM32 HAL | Phụ thuộc | `HAL_GPIO_WritePin()` |
| `status`, `timer`, `sensor`, `conveyor`, `servo`, `data` | Không phụ thuộc | `main` đọc trạng thái rồi ra lệnh |

## 7. Testing

### Test 1: Khởi tạo

- Gọi `Indicator_Init()`.
- Kiểm tra `LED_RUN`, `LED_FAULT` tắt, còi im.

### Test 2: LED_RUN

- Gọi `Indicator_SetRun(1U)`, kiểm tra `LED_RUN` sáng.
- Gọi `Indicator_SetRun(0U)`, kiểm tra `LED_RUN` tắt.

### Test 3: Fault

- Gọi `Indicator_SetFault(1U)`, kiểm tra `LED_FAULT` sáng và còi kêu liên tục.
- Gọi `Indicator_SetFault(0U)`, kiểm tra cả hai tắt.

### Test 4: Độc lập giữa hai nhóm

- Gọi `Indicator_SetRun(1U)` rồi `Indicator_SetFault(1U)`.
- Kiểm tra cả hai cùng hoạt động (module không tự loại trừ).
- Gọi `Indicator_SetRun(0U)`, kiểm tra `LED_FAULT` và còi vẫn hoạt động.

### Test 5: Gọi lặp lại

- Gọi `Indicator_SetRun(1U)` hai lần, `Indicator_SetFault(0U)` hai lần.
- Kiểm tra không có hành vi bất thường.

### Test 6: Luồng lỗi trong `main`

- Tạo lỗi, kiểm tra `LED_RUN` tắt, `LED_FAULT` và còi hoạt động.
- Nhấn `BTN_RESET`, kiểm tra `LED_FAULT` và còi tắt.

### Test 7: Kiểm tra dòng tiêu thụ thực tế của còi

- Đo dòng của còi khi cấp từ chân PB1 (3,3 V).
- Kiểm tra dòng thực tế nằm trong giới hạn an toàn của chân GPIO, còi kêu đủ to và ổn định.
- Nếu dòng quá cao, còi yếu hoặc chân PB1 nóng: thêm transistor NPN và cấp 5 V cho còi.

## Các thông số và quyết định chưa chốt

| Nội dung | Trạng thái |
|---|---|
| Dòng tiêu thụ thực tế của còi | Cần kiểm tra. Còi SFM-20B bản 5 V của dự án, đo bằng tay dưới 8 mA ở 3,3 V, chưa có datasheet xác nhận. Các trang bán hàng chỉ ghi số liệu của bản 12 V (10 đến 12 mA). Đang nối trực tiếp PB1. Nếu còi yếu hoặc chân nóng, thêm transistor NPN và cấp 5 V cho còi, không đổi code |
| Kiểu kêu của còi | Liên tục. Kêu ngắt quãng cần thêm `Indicator_Update()` và `Timer_GetTick()`, chưa làm |
| Tắt còi riêng (mute) | Không làm ở bản hiện tại |
| Mức tích cực | HIGH là bật, đổi bằng macro trong `indicator.c` nếu phần cứng thay đổi |
