# MODULE DESIGN: DATA

## 1. Overview

Module `data` quản lý bộ đếm sản phẩm loại A, loại B. Bộ đếm tồn tại ở hai nơi:

- **RAM:** bản chính, thay đổi liên tục khi vận hành, dùng để hiển thị LCD và gửi IoT. Mất khi tắt nguồn.
- **ROM (Flash nội của STM32F103C8T6):** bản sao, chỉ được ghi vào những thời điểm xác định (xem mục 2.2). Còn nguyên khi tắt nguồn và dùng để khôi phục bộ đếm lúc khởi động.

Module `data` không nhận diện sản phẩm, không quyết định trạng thái hệ thống, không xử lý nút nhấn. Việc kiểm tra trạng thái hệ thống trước khi gọi `Data_Reset()` thuộc trách nhiệm của `main`.

### 1.1. Các khái niệm dùng trong tài liệu

| Tên | Nằm ở | Ý nghĩa |
|---|---|---|
| `count_a` | RAM | Số sản phẩm loại A (bản chính) |
| `count_b` | RAM | Số sản phẩm loại B (bản chính) |
| `count_total` | RAM | Tổng `count_a + count_b`, lưu thành biến riêng trong `data` và luôn được cập nhật cùng `count_a`, `count_b` |
| Bản sao ROM | ROM | Giá trị `count_a`, `count_b` tại lần ghi ROM gần nhất |
| `save_total` | RAM | Giá trị `count_total` tại lần RAM và ROM được đồng bộ gần nhất, tức là lần nạp từ ROM (khởi động, reset) hoặc lần lưu vào ROM. Hiệu `count_total - save_total` là số sản phẩm mới chưa được lưu vào ROM. |
| `count_threshold` | Hằng số | Số sản phẩm mới tối thiểu (`count_total - save_total`) để được phép lưu ROM |
| `cyclic_check` | Hằng số | Chu kỳ kiểm tra lưu ROM (ms) |

## 2. Responsibilities

### 2.1. Bộ đếm trong RAM

- `count_a`, `count_b` và `count_total` là bản chính của bộ đếm. LCD và IoT đều lấy giá trị từ RAM.
- RAM được ghi trong ba trường hợp:
  1. **Khởi động:** `Data_Init()` nạp `count_a`, `count_b` từ ROM và tính `count_total`.
  2. **Vận hành:** `main` gọi `Data_IncrementA()` hoặc `Data_IncrementB()` mỗi khi `sensor` báo sản phẩm A hoặc B. Việc quyết định sản phẩm có hợp lệ hay không thuộc trách nhiệm của `main`, module `data` chỉ tăng khi được gọi.
  3. **Reset:** `Data_Reset()` ghi 0 vào ROM rồi nạp lại RAM từ ROM.
- `count_total` luôn bằng `count_a + count_b`, được cập nhật ở mọi nơi `count_a` hoặc `count_b` thay đổi.

### 2.2. Bản sao trong ROM

- ROM chỉ lưu `count_a` và `count_b`, không lưu `count_total`.
- ROM không được ghi sau mỗi lần tăng bộ đếm. ROM được ghi trong hai trường hợp:
  1. **Lưu định kỳ:** `main` gọi `Data_Update()` mỗi vòng lặp, module tự quyết định có ghi hay không. Chỉ ghi khi thỏa **cả ba** điều kiện:
     - Hệ thống đang `STATE_RUNNING` (`main` truyền `isRunning = 1`).
     - Đã đủ chu kỳ `cyclic_check` kể từ lần kiểm tra trước.
     - `count_total - save_total >= count_threshold`.
  2. **Reset:** `main` gọi `Data_Reset()` ở `STATE_STOP`, module ghi 0 vào ROM.
- Chính sách lưu dùng chung cho `MODE_AUTO` và `MODE_MANUAL`.
- Do chỉ lưu theo ngưỡng và chu kỳ, ROM luôn chậm hơn hoặc bằng RAM.

### 2.3. Luồng dữ liệu giữa RAM và ROM

| Thời điểm | RAM (`count_a`, `count_b`, `count_total`) | ROM | `save_total` |
|---|---|---|---|
| Khởi động (`Data_Init`) | Nạp từ ROM (Flash trống thì bằng 0) | Không đổi | `= count_total` |
| Có sản phẩm A hoặc B (`main` gọi `Data_IncrementA/B`) | Tăng 1 | Không đổi | Không đổi |
| Lưu định kỳ (`Data_Update`, đủ điều kiện) | Không đổi | Ghi bằng giá trị RAM | `= count_total` |
| Reset (`Data_Reset`) | Nạp lại từ ROM sau khi ROM bị ghi 0 | Ghi 0 | `= count_total` |
| Tắt nguồn | Mất | Giữ nguyên | Mất |

### 2.4. Điều kiện `main` gọi `Data_Reset()`

- Chỉ ở `STATE_STOP`.
- Ở `STATE_ERROR`, nút RESET chỉ xóa lỗi, không xóa bộ đếm.
- Ở `STATE_RUNNING`, nút RESET không có tác dụng.

## 3. Input / Output

### 3.1. Input

| Input | Mô tả |
|---|---|
| `Data_IncrementA()` / `Data_IncrementB()` | Lệnh tăng bộ đếm từ `main` |
| `isRunning` (tham số `Data_Update()`) | `1` khi hệ thống ở `STATE_RUNNING`, `0` trong các trường hợp còn lại |
| `Data_Reset()` | Lệnh xóa bộ đếm từ `main` |
| `Timer_GetTick()` / `Timer_IsElapsed()` | Lấy thời gian từ module `timer` để tính chu kỳ `cyclic_check` |
| Dữ liệu ROM | Hai giá trị `count_a`, `count_b` đọc từ Flash khi khởi động và sau reset |

### 3.2. Output

| Output | Kiểu | Mô tả |
|---|---|---|
| `Data_GetCountA()` | `uint32_t` | Số sản phẩm loại A |
| `Data_GetCountB()` | `uint32_t` | Số sản phẩm loại B |
| `Data_GetCountTotal()` | `uint32_t` | Trả về `count_total` |

## 4. API Design

```c
void Data_Init(void);
void Data_IncrementA(void);
void Data_IncrementB(void);
uint32_t Data_GetCountA(void);
uint32_t Data_GetCountB(void);
uint32_t Data_GetCountTotal(void);
void Data_Update(uint8_t isRunning);
void Data_Reset(void);
```

| API | Chức năng |
|---|---|
| `Data_Init()` | Nạp dữ liệu từ ROM, khởi tạo `count_total`, `save_total` và các biến nội bộ. Gọi một lần khi khởi động, sau `Timer_Init()`. |
| `Data_IncrementA()` | `count_a++`, `count_total++`. Gọi khi `main` nhận `SENSOR_EVENT_PRODUCT_A`. |
| `Data_IncrementB()` | `count_b++`, `count_total++`. Gọi khi `main` nhận `SENSOR_EVENT_PRODUCT_B`. |
| `Data_GetCountA()` | Trả về `count_a`. |
| `Data_GetCountB()` | Trả về `count_b`. |
| `Data_GetCountTotal()` | Trả về `count_total`. |
| `Data_Update()` | Kiểm tra điều kiện lưu ROM và lưu nếu đủ điều kiện. `main` gọi mỗi vòng lặp, truyền `1` nếu hệ thống đang `STATE_RUNNING`, ngược lại truyền `0`. |
| `Data_Reset()` | Ghi 0 vào ROM rồi nạp lại từ ROM. `main` chỉ gọi khi hệ thống ở `STATE_STOP`. |

Ví dụ sử dụng trong `main`:

```c
Timer_Init();
Data_Init();

/* Trong vòng lặp chính */
SensorEvent_t ev = Sensor_Update();

if (ev == SENSOR_EVENT_PRODUCT_A)
{
    Data_IncrementA();
}
else if (ev == SENSOR_EVENT_PRODUCT_B)
{
    Data_IncrementB();
}

if ((btn == BUTTON_EVENT_RESET) && (state == STATE_STOP))
{
    Data_Reset();
}

Data_Update((state == STATE_RUNNING) ? 1U : 0U);

/* Cập nhật LCD, IoT */
total = Data_GetCountTotal();
a     = Data_GetCountA();
b     = Data_GetCountB();
```

## 5. Internal Design

### 5.1. Biến nội bộ

| Biến | Kiểu | Chức năng |
|---|---|---|
| `count_a` | `uint32_t` | Bộ đếm sản phẩm A trong RAM |
| `count_b` | `uint32_t` | Bộ đếm sản phẩm B trong RAM |
| `count_total` | `uint32_t` | Tổng `count_a + count_b` trong RAM |
| `save_total` | `uint32_t` | Giá trị `count_total` tại lần RAM và ROM được đồng bộ gần nhất (nạp từ ROM hoặc lưu vào ROM) |
| `last_check_tick` | `uint32_t` | Mốc thời gian bắt đầu chu kỳ `cyclic_check` hiện tại |
| `was_running` | `uint8_t` | Giá trị `isRunning` của lần gọi `Data_Update()` trước |

Ràng buộc luôn đúng:

- `count_total == count_a + count_b`. Chỉ ba nơi được thay đổi `count_a`, `count_b`: `Data_IncrementA()`, `Data_IncrementB()` và `LoadFromRom()`; cả ba đều phải cập nhật `count_total`.
- `save_total <= count_total`, vì bộ đếm chỉ tăng và `save_total` chỉ được gán bằng `count_total`. Phép trừ không dấu `count_total - save_total` vì thế không bị tràn.

### 5.2. Tham số cấu hình

| Tham số | Ý nghĩa | Giá trị tạm |
|---|---|---|
| `count_threshold` | Số sản phẩm mới tối thiểu để lưu ROM | 3 |
| `cyclic_check` | Chu kỳ kiểm tra lưu (ms) | 10000 |

Hai giá trị trên chỉ là giá trị tạm, sẽ chỉnh khi thử nghiệm. Được khai báo là hằng số `static const uint32_t` trong `data.c`.

### 5.3. Hàm nội bộ truy cập Flash

Chỉ hai hàm `static` dưới đây làm việc trực tiếp với Flash. Phần còn lại của module không phụ thuộc phần cứng.

**`LoadFromRom()`**

1. Đọc hai giá trị 32 bit từ địa chỉ lưu trữ.
2. Nếu một trong hai bằng `0xFFFFFFFF` (Flash trống), gán cả hai bằng 0.
3. Gán `count_a`, `count_b`.
4. Gán `count_total = count_a + count_b`.
5. Gán `save_total = count_total`.

**`SaveToRom(a, b)`**

1. Mở khóa Flash (`HAL_FLASH_Unlock`).
2. Xóa trang chứa dữ liệu (`HAL_FLASHEx_Erase`).
3. Ghi `a` và `b` (`HAL_FLASH_Program`, ghi từng từ 32 bit).
4. Khóa Flash (`HAL_FLASH_Lock`).

Hàm không trả về kết quả vì chính sách hiện tại không dùng kết quả lưu (xem mục 5.4 và 5.5).

**Vùng lưu trữ:** trang cuối của Flash, ví dụ `0x0800FC00` với Flash 64 KB. Cần kiểm tra file `.map` của Keil để chắc chương trình không chiếm trang này.

### 5.4. Chính sách lưu ROM trong `Data_Update()`

Logic áp dụng mỗi lần gọi `Data_Update(isRunning)`:

1. Nếu `isRunning = 0`:
   - Đặt `was_running = 0`.
   - Thoát, không lưu và giữ nguyên `save_total`.
2. Nếu `isRunning = 1` và `was_running = 0` (vừa vào `STATE_RUNNING`):
   - `last_check_tick = Timer_GetTick()`.
   - `was_running = 1`.
   - Thoát, không kiểm tra ngưỡng ngay.
3. Nếu `Timer_IsElapsed(last_check_tick, cyclic_check)` đúng:
   - `last_check_tick = Timer_GetTick()`.
   - Nếu `count_total - save_total >= count_threshold`:
     - Gọi `SaveToRom(count_a, count_b)`.
     - `save_total = count_total`.
4. Ngược lại, không làm gì.

Điều kiện lưu cần đủ cả `cyclic_check` và `count_threshold`.

Lưu ý:

- Sau mỗi lần gọi `SaveToRom()`, `save_total` luôn được cập nhật, kể cả khi ghi Flash lỗi. Nếu lỗi, module không thử lại ngay mà chờ đủ `count_threshold` sản phẩm mới và `cyclic_check` tiếp theo.
- Không lưu thêm khi chuyển sang `STATE_STOP` hoặc `STATE_ERROR`.
- Chính sách không phân biệt AUTO và MANUAL. Ở MANUAL khi băng tải tắt, không có sản phẩm mới nên không đạt `count_threshold`, module tự nhiên không lưu.

### 5.5. Khởi tạo và Reset

**`Data_Init()`:**

1. Gọi `LoadFromRom()`.
2. `last_check_tick = 0`, `was_running = 0`.

**`Data_Reset()`:**

1. Gọi `SaveToRom(0, 0)`.
2. Gọi `LoadFromRom()`.

Vì `LoadFromRom()` luôn đặt lại `count_total` và `save_total` theo giá trị vừa nạp:

- Nếu ghi ROM thành công: `count_a = count_b = 0`, `count_total = 0` và `save_total = 0`.
- Nếu ghi ROM thất bại: RAM và `save_total` quay về giá trị đang có trong ROM. LCD vẫn hiển thị số cũ, người vận hành có thể nhấn RESET lại.

### 5.6. Giới hạn đã chấp nhận

- Khi mất nguồn, các lượt đếm chưa lưu (nhỏ hơn `count_threshold`, hoặc chưa đủ chu kỳ `cyclic_check`) sẽ bị mất.
- Không kiểm tra tính hợp lệ của dữ liệu ROM ngoài trường hợp Flash trống. Nếu mất nguồn đúng lúc đang ghi Flash, dữ liệu có thể sai. Xác suất thấp vì mỗi lần ghi chỉ kéo dài vài ms và lưu thưa.
- Khi `Data_Reset()` ghi ROM thất bại, các lượt đếm chưa lưu trong RAM bị mất do RAM được nạp lại từ ROM.
- Không xử lý tràn `uint32_t` của bộ đếm.
- Khi xóa hoặc ghi Flash, CPU dừng khoảng vài ms đến vài chục ms, có thể làm trễ vài tick của TIM1. Vì vậy `cyclic_check` được chọn đủ lớn để việc lưu diễn ra thưa, đồng thời hạn chế số lần xóa Flash.

## 6. Dependencies

| Thành phần | Quan hệ | Mô tả |
|---|---|---|
| `main` | Gọi API | Tăng bộ đếm, gọi `Data_Update()`, gọi `Data_Reset()`, lấy giá trị bộ đếm |
| `timer` | Phụ thuộc | `Timer_GetTick()`, `Timer_IsElapsed()` để tính chu kỳ `cyclic_check` |
| STM32 HAL Flash | Phụ thuộc phần cứng | Đọc, xóa, ghi Flash nội |
| `sensor` | Gián tiếp | Phát sinh sự kiện để `main` quyết định tăng bộ đếm |
| `LCD`, `IoT` | Không phụ thuộc | `main` lấy giá trị từ `data` rồi truyền cho `LCD` và `IoT` |

Module `data` không gọi trực tiếp `sensor`, `LCD`, `IoT` hoặc `status`.

## 7. Testing

### Test 1: Khởi tạo với dữ liệu ROM

- Ghi sẵn A và B vào ROM, gọi `Data_Init()`.
- Kiểm tra `count_a`, `count_b` đúng giá trị đã ghi, `count_total` bằng tổng và `save_total` bằng `count_total`.

### Test 2: Khởi tạo khi Flash trống

- Xóa trang Flash lưu dữ liệu, gọi `Data_Init()`.
- Kiểm tra `count_a = count_b = 0`, `count_total = 0` và `save_total = 0`.

### Test 3: Tăng bộ đếm A

- Gọi `Data_IncrementA()` nhiều lần.
- Kiểm tra `count_a` và `count_total` tăng đúng, `count_b` không đổi.

### Test 4: Tăng bộ đếm B

- Gọi `Data_IncrementB()` nhiều lần.
- Kiểm tra `count_b` và `count_total` tăng đúng, `count_a` không đổi.

### Test 5: Tính tổng

- Gọi xen kẽ `Data_IncrementA()` và `Data_IncrementB()`.
- Kiểm tra `Data_GetCountTotal()` luôn bằng `Data_GetCountA() + Data_GetCountB()`.

### Test 6: Chưa đủ chu kỳ cyclic_check

- Gọi `Data_Update(1)` liên tục khi đã đạt `count_threshold` nhưng chưa đủ `cyclic_check`.
- Kiểm tra không lưu ROM, không lưu ngay lúc vừa vào `STATE_RUNNING`.

### Test 7: Đủ cyclic_check nhưng chưa đủ count_threshold

- Tăng bộ đếm ít hơn `count_threshold`, chờ đủ `cyclic_check`.
- Kiểm tra không lưu ROM và `save_total` không đổi.

### Test 8: Đủ cả count_threshold và cyclic_check

- Tăng bộ đếm từ `count_threshold` trở lên, chờ đủ `cyclic_check`.
- Kiểm tra ROM được lưu đúng `count_a`, `count_b` và `save_total` được cập nhật bằng `count_total`.

### Test 9: Không lưu khi không chạy

- Gọi `Data_Update(0)` khi đã đạt `count_threshold` và quá `cyclic_check`.
- Kiểm tra không lưu ROM, `save_total` không đổi.
- Gọi lại `Data_Update(1)` và kiểm tra chu kỳ `cyclic_check` được tính lại từ thời điểm vào lại.

### Test 10: Reset

- Đặt A và B khác 0, gọi `Data_Reset()`.
- Kiểm tra RAM, ROM, `count_total` và `save_total` đều bằng 0. Sau khi khởi động lại, dữ liệu vẫn là 0.

### Test 11: Mất nguồn

- Tăng bộ đếm, chờ lưu ROM, tắt rồi cấp nguồn lại.
- Kiểm tra giá trị khôi phục bằng giá trị đã lưu lần gần nhất.

## Các thông số và quyết định chưa chốt

| Nội dung | Trạng thái |
|---|---|
| Giá trị `count_threshold` | Tạm: 3, chỉnh khi thử nghiệm |
| Giá trị `cyclic_check` | Tạm: 10000 ms, chỉnh khi thử nghiệm |
| Địa chỉ trang Flash lưu dữ liệu | Dự kiến trang cuối, cần kiểm tra `.map` |
| Mô phỏng ghi Flash trên Proteus | Cần thử; nếu không được, thay tạm hai hàm Flash bằng mảng RAM để test logic |
| Lưu thêm khi vào `STATE_STOP` / `STATE_ERROR` | Không làm ở bản hiện tại |
