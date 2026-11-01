# Timer Module

## 1. Overview

Module `timer` cung cấp bộ đếm thời gian hệ thống theo đơn vị millisecond (ms), sử dụng ngắt định kỳ từ TIM1.

Các module khác sử dụng bộ đếm này để kiểm tra thời gian đã trôi qua mà không cần tạo vòng lặp chờ hoặc delay blocking.

**Lưu ý:** TIM1 phải được cấu hình trong STM32CubeMX và khởi động ngắt trước khi sử dụng bộ đếm. Module `timer` không tự cấu hình hoặc khởi động TIM1.

## 2. Responsibilities

- Quản lý biến đếm thời gian hệ thống `g_msTicks`.
- Cung cấp giá trị tick hiện tại.
- Kiểm tra một khoảng thời gian đã trôi qua hay chưa.
- Tăng bộ đếm mỗi khi TIM1 phát sinh ngắt cập nhật.

Module không trực tiếp quản lý các khoảng thời gian cụ thể của hệ thống. Các module sử dụng timer tự lưu thời điểm bắt đầu và xác định thời gian cần kiểm tra.

## 3. Input / Output

| Loại | Thành phần | Mô tả |
|---|---|---|
| Input | Ngắt TIM1 | Tạo tick thời gian định kỳ |
| Input | `startTime` | Thời điểm bắt đầu tính thời gian |
| Input | `duration` | Khoảng thời gian cần kiểm tra (ms) |
| Output | `g_msTicks` | Giá trị tick hiện tại thông qua API |
| Output | Kết quả kiểm tra | `1U`: đã đủ thời gian; `0U`: chưa đủ |

**Đơn vị thời gian:** millisecond (ms), với điều kiện mỗi ngắt TIM1 tương ứng chính xác 1 ms.

## 4. API Design

### `Timer_Init()`

```c
void Timer_Init(void);
```

- Đặt bộ đếm thời gian hệ thống về `0`.
- Không cấu hình TIM1.
- Không khởi động ngắt TIM1.

### `Timer_GetTick()`

```c
uint32_t Timer_GetTick(void);
```

- Trả về giá trị hiện tại của bộ đếm `g_msTicks`.
- Giá trị trả về có kiểu `uint32_t`.

### `Timer_IsElapsed()`

```c
uint8_t Timer_IsElapsed(uint32_t startTime, uint32_t duration);
```

- Kiểm tra thời gian đã trôi qua kể từ `startTime` có đạt `duration` hay chưa.
- Trả về `1U` nếu đã đủ thời gian.
- Trả về `0U` nếu chưa đủ thời gian.
- Phép trừ không dấu giúp xử lý trường hợp bộ đếm tràn `uint32_t`.

Ví dụ sử dụng:

```c
uint32_t startTime = Timer_GetTick();

if (Timer_IsElapsed(startTime, 1000U))
{
    // Đã trôi qua ít nhất 1000 ms
}
```

## 5. Internal Design

### Biến nội bộ

```c
static volatile uint32_t g_msTicks = 0U;
```

- `static`: chỉ được truy cập bên trong `timer.c`.
- `volatile`: thông báo cho compiler rằng giá trị có thể thay đổi ngoài luồng thực thi thông thường, ở đây là trong ngắt TIM1.
- `uint32_t`: bộ đếm có thể chạy từ `0` đến `4294967295`, sau đó quay về `0`.

### Cơ chế hoạt động

1. CubeMX cấu hình TIM1 với chu kỳ ngắt 1 ms.
2. Chương trình khởi động ngắt TIM1.
3. `Timer_Init()` đặt `g_msTicks` về `0`.
4. Mỗi khi TIM1 phát sinh ngắt, `HAL_TIM_PeriodElapsedCallback()` được gọi.
5. Nếu ngắt đến từ TIM1, `g_msTicks` tăng thêm 1.
6. Các module khác gọi `Timer_GetTick()` hoặc `Timer_IsElapsed()` để kiểm tra thời gian.

### Xử lý tràn bộ đếm

`Timer_IsElapsed()` sử dụng phép trừ số nguyên không dấu:

```c
(uint32_t)(currentTime - startTime) >= duration
```

Cách tính này vẫn hoạt động khi bộ đếm tràn từ giá trị lớn nhất về `0`, miễn là khoảng thời gian cần đo nhỏ hơn một chu kỳ tràn của bộ đếm.

## 6. Dependencies

| Thành phần | Vai trò |
|---|---|
| TIM1 | Tạo ngắt định kỳ |
| STM32 HAL | Cung cấp callback xử lý ngắt |
| CubeMX | Cấu hình thông số TIM1 |

Các module khác có thể phụ thuộc vào API của `timer` để kiểm tra thời gian. Module `timer` không phụ thuộc vào các module chức năng khác.

## 7. Testing

- Kiểm tra `Timer_Init()` đặt bộ đếm về `0`.
- Kiểm tra `Timer_GetTick()` tăng theo thời gian.
- Kiểm tra `Timer_IsElapsed()` với các trường hợp:
  - Thời gian chưa đủ.
  - Thời gian vừa đủ.
  - Thời gian đã vượt quá khoảng yêu cầu.
- Kiểm tra xử lý tràn bộ đếm bằng cách thử với `startTime` gần giá trị lớn nhất của `uint32_t`.

**Điều kiện cần xác nhận:** Chu kỳ ngắt thực tế của TIM1 phải là 1 ms. Nếu cấu hình TIM1 không đúng chu kỳ này, giá trị tick sẽ không còn tương ứng với đơn vị millisecond như thiết kế.
