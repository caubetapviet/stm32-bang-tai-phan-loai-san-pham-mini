# MODULE STATUS

## 1. Overview

Module `status` quản lý trạng thái hệ thống, chế độ hoạt động và mã lỗi:

- Trạng thái: `STATE_STOP`, `STATE_RUNNING`, `STATE_ERROR`.
- Chế độ: `MODE_AUTO`, `MODE_MANUAL`.
- Mã lỗi: `ERROR_NONE`, `ERROR_TIMEOUT`, `ERROR_CONTINUOUSLY_ACTIVE`.

Module kiểm tra yêu cầu đổi trạng thái hoặc chế độ có hợp lệ hay không, và phát hiện hai lỗi `ERROR_TIMEOUT`, `ERROR_CONTINUOUSLY_ACTIVE`.

Module `status` không điều khiển thiết bị. Khi trạng thái thay đổi, `main` đọc kết quả rồi ra lệnh cho `conveyor`, `servo`, `sensor`, `indicator`, `LCD`. Trạng thái bật/tắt băng tải và vị trí servo ở `MODE_MANUAL` thuộc các module thiết bị, không thuộc `status`.

Module `status` không phụ thuộc `buttons`, `sensor`, `conveyor`. `main` chuyển sự kiện nút thành lời gọi API của `status` và truyền dữ liệu cảm biến, băng tải vào `Status_Update()` qua tham số.

## 2. Responsibilities

- Giữ `g_state`, `g_mode`, `g_error_code` là biến nội bộ (`static`) trong `status.c`. Chỉ các hàm của module được thay đổi chúng.
- Chấp nhận hoặc từ chối yêu cầu đổi trạng thái/chế độ theo bảng chuyển trạng thái (mục 5.1) và trả kết quả cho `main`.
- Phát hiện `ERROR_TIMEOUT` và `ERROR_CONTINUOUSLY_ACTIVE`, chuyển hệ thống sang `STATE_ERROR` và ghi mã lỗi.
- Cung cấp trạng thái, chế độ, mã lỗi hiện tại cho các module khác thông qua `main`.

Module không chịu trách nhiệm:

- Đọc nút nhấn, đọc cảm biến.
- Điều khiển băng tải, servo, LED, còi, LCD.
- Xóa bộ đếm (`BTN_RESET` ở `STATE_STOP` do `main` gọi `Data_Reset()`).
- Quyết định băng tải đang chạy hay không (do `main` truyền vào).
- Lưu chế độ qua lần tắt nguồn. Khi cấp điện luôn vào `STATE_STOP` và `MODE_AUTO`.

## 3. Input / Output

### 3.1. Input

| Input | Mô tả |
|---|---|
| `Status_Start()`, `Status_Stop()`, `Status_ToggleMode()`, `Status_ClearError()` | Yêu cầu đổi trạng thái/chế độ từ `main` (do thao tác nút nhấn) |
| `conveyorRunning` (tham số `Status_Update()`) | `1U` nếu băng tải đang chạy, `0U` nếu dừng |
| `cbThapActive`, `cbCaoActive` (tham số `Status_Update()`) | `1U` nếu cảm biến đang ở mức LOW (phát hiện vật), `0U` nếu HIGH |
| `classified` (tham số `Status_Update()`) | `1U` nếu vòng lặp này có lượt phân loại thành công (`SENSOR_EVENT_PRODUCT_A` hoặc `SENSOR_EVENT_PRODUCT_B`), ngược lại `0U` |
| `Timer_GetTick()` / `Timer_IsElapsed()` | Lấy thời gian từ module `timer` để tính timeout và T2 |

### 3.2. Output

| Output | Kiểu | Mô tả |
|---|---|---|
| `Status_GetState()` | `SystemState_t` | Trạng thái hiện tại |
| `Status_GetMode()` | `SystemMode_t` | Chế độ hiện tại |
| `Status_GetError()` | `ErrorCode_t` | Mã lỗi hiện tại (`ERROR_NONE` khi không có lỗi) |
| Kết quả `Status_Start/Stop/ToggleMode/ClearError()` | `uint8_t` | `1U`: yêu cầu hợp lệ và đã đổi; `0U`: bị bỏ qua |
| Kết quả `Status_Update()` | `uint8_t` | `1U` đúng một lần khi vừa phát sinh lỗi; ngược lại `0U` |

## 4. API Design

### 4.1. Kiểu dữ liệu

```c
typedef enum
{
    STATE_STOP = 0,
    STATE_RUNNING,
    STATE_ERROR
} SystemState_t;

typedef enum
{
    MODE_AUTO = 0,
    MODE_MANUAL
} SystemMode_t;

typedef enum
{
    ERROR_NONE = 0,
    ERROR_TIMEOUT,
    ERROR_CONTINUOUSLY_ACTIVE
} ErrorCode_t;
```

### 4.2. Danh sách API

```c
void Status_Init(void);

SystemState_t Status_GetState(void);
SystemMode_t  Status_GetMode(void);
ErrorCode_t   Status_GetError(void);

uint8_t Status_Start(void);
uint8_t Status_Stop(void);
uint8_t Status_ToggleMode(void);
uint8_t Status_ClearError(void);

uint8_t Status_Update(uint8_t conveyorRunning,
                      uint8_t cbThapActive,
                      uint8_t cbCaoActive,
                      uint8_t classified);
```

### 4.3. Chức năng từng API

| API | Chức năng |
|---|---|
| `Status_Init()` | Đặt `STATE_STOP`, `MODE_AUTO`, `ERROR_NONE` và xóa các mốc thời gian. Gọi một lần khi khởi động, sau `Timer_Init()`. |
| `Status_GetState()` | Trả về trạng thái hiện tại. Chỉ đọc, không thay đổi trạng thái. |
| `Status_GetMode()` | Trả về chế độ hiện tại. |
| `Status_GetError()` | Trả về mã lỗi hiện tại. |
| `Status_Start()` | Nếu đang `STATE_STOP`: chuyển sang `STATE_RUNNING`, xóa các mốc thời gian, trả `1U`. Ngược lại trả `0U`. |
| `Status_Stop()` | Nếu đang `STATE_RUNNING`: chuyển sang `STATE_STOP`, xóa các mốc thời gian, trả `1U`. Ngược lại trả `0U`. |
| `Status_ToggleMode()` | Nếu đang `STATE_STOP`: đổi giữa `MODE_AUTO` và `MODE_MANUAL`, trả `1U`. Ngược lại trả `0U`. |
| `Status_ClearError()` | Nếu đang `STATE_ERROR`: chuyển sang `STATE_STOP`, đặt `ERROR_NONE`, trả `1U`. Ngược lại trả `0U`. |
| `Status_Update()` | Phát hiện lỗi (mục 5.3). `main` gọi mỗi vòng lặp. Trả `1U` đúng một lần khi vừa chuyển từ `STATE_RUNNING` sang `STATE_ERROR`, ngược lại `0U`. |

### 4.4. Ví dụ sử dụng trong `main`

```c
Timer_Init();
Status_Init();

/* Trong vòng lặp chính */
ButtonEvent_t btn = Buttons_Update();

switch (btn)
{
    case BUTTON_EVENT_START:
        if (Status_Start())
        {
            /* Sensor_Start(); AUTO: băng tải chạy ngay;
               MANUAL: băng tải dừng, servo Non-gate */
        }
        break;

    case BUTTON_EVENT_STOP:
        if (Status_Stop())
        {
            /* Dừng băng tải, servo Non-gate, Sensor_Stop() */
        }
        break;

    case BUTTON_EVENT_MODE:
        Status_ToggleMode();
        break;

    case BUTTON_EVENT_RESET:
        if (Status_GetState() == STATE_STOP)
        {
            Data_Reset();
        }
        else if (Status_ClearError())
        {
            /* Tắt LED lỗi và còi */
        }
        break;

    default:
        break;
}

/* Phát hiện lỗi */
SensorEvent_t ev = Sensor_Update();
uint8_t classified = ((ev == SENSOR_EVENT_PRODUCT_A) ||
                      (ev == SENSOR_EVENT_PRODUCT_B)) ? 1U : 0U;

SensorState_t s;
Sensor_GetState(&s);

if (Status_Update(conveyorRunning, s.cb_thap_active, s.cb_cao_active, classified))
{
    /* Vừa vào STATE_ERROR: dừng băng tải, servo Non-gate, Sensor_Stop(),
       bật LED lỗi và còi, hủy lượt phân loại đang xử lý */
}

Data_Update((Status_GetState() == STATE_RUNNING) ? 1U : 0U);
```

Lưu ý: `classified` chỉ đúng trong một vòng lặp, nên `main` phải truyền mỗi vòng (truyền `0U` khi không có sự kiện).

## 5. Internal Design

### 5.1. Bảng chuyển trạng thái

| Hiện tại | Sự kiện | Kết quả |
|---|---|---|
| `STATE_STOP` | `Status_Start()` | `STATE_RUNNING` |
| `STATE_STOP` | `Status_ToggleMode()` | Đổi `MODE_AUTO` / `MODE_MANUAL` |
| `STATE_RUNNING` | `Status_Stop()` | `STATE_STOP` |
| `STATE_RUNNING` | `Status_Update()` phát hiện lỗi | `STATE_ERROR`, ghi mã lỗi |
| `STATE_ERROR` | `Status_ClearError()` | `STATE_STOP`, `ERROR_NONE` |

Các trường hợp còn lại: không đổi, hàm trả `0U`. Không cho phép chuyển trực tiếp từ `STATE_ERROR` sang `STATE_RUNNING`. Chế độ chỉ đổi được ở `STATE_STOP`.

### 5.2. Biến nội bộ

| Biến | Kiểu | Chức năng |
|---|---|---|
| `g_state` | `SystemState_t` | Trạng thái hiện tại |
| `g_mode` | `SystemMode_t` | Chế độ hiện tại |
| `g_error_code` | `ErrorCode_t` | Mã lỗi hiện tại |
| `was_conveyor_running` | `uint8_t` | Giá trị `conveyorRunning` của lần gọi `Status_Update()` trước, để phát hiện lúc băng tải vừa chạy |
| `timeout_start_tick` | `uint32_t` | Mốc bắt đầu tính `timeout_limit` |
| `sensor_active_tracking` | `uint8_t` | `1U` nếu đang tính thời gian cảm biến kích hoạt liên tục |
| `sensor_active_start_tick` | `uint32_t` | Mốc bắt đầu tính `sensor_active_limit` |

### 5.3. Tham số cấu hình

| Tham số | Ý nghĩa | Giá trị tạm |
|---|---|---|
| `timeout_limit` | Thời gian tối đa không có lượt phân loại thành công (ms) | 30000 |
| `sensor_active_limit` (T2) | Thời gian tối đa một cảm biến ở LOW liên tục (ms) | 2000 (chưa chốt) |

Ràng buộc của T2: T2 phải lớn hơn T1 của module `sensor` (hiện 50 ms) và lớn hơn thời gian một sản phẩm đi qua cảm biến, để sản phẩm bình thường không gây lỗi. Hai tham số được khai báo là hằng số `static const uint32_t` trong `status.c` và sẽ chỉnh khi thử nghiệm.

### 5.4. Logic `Status_Update()`

Chỉ có tác dụng khi `g_state == STATE_RUNNING`. Nếu trạng thái khác: đặt `was_conveyor_running = 0`, `sensor_active_tracking = 0`, trả `0U`.

Khi `STATE_RUNNING`, thực hiện theo thứ tự:

1. Nếu `conveyorRunning = 0`:
   - Đặt `was_conveyor_running = 0`, `sensor_active_tracking = 0`.
   - Trả `0U`. Băng tải dừng thì không kiểm tra cảm biến, dù cảm biến ở mức nào.
2. Nếu `conveyorRunning = 1` và `was_conveyor_running = 0` (băng tải vừa chạy):
   - `timeout_start_tick = Timer_GetTick()`.
   - `was_conveyor_running = 1`.
3. Nếu `classified = 1`: `timeout_start_tick = Timer_GetTick()`. Bước này đặt **trước** khi kiểm tra timeout, để lượt phân loại thành công đúng lúc hết thời gian không bị báo lỗi.
4. Xử lý thời gian cảm biến kích hoạt liên tục:
   - Nếu `cbThapActive` hoặc `cbCaoActive` bằng `1U`:
     - Nếu `sensor_active_tracking = 0`: đặt `sensor_active_tracking = 1`, `sensor_active_start_tick = Timer_GetTick()`.
   - Nếu cả hai bằng `0U`: đặt `sensor_active_tracking = 0`.
5. Kiểm tra lỗi, theo thứ tự ưu tiên:
   - Nếu `sensor_active_tracking = 1` và `Timer_IsElapsed(sensor_active_start_tick, sensor_active_limit)`: lỗi `ERROR_CONTINUOUSLY_ACTIVE`.
   - Ngược lại, nếu `Timer_IsElapsed(timeout_start_tick, timeout_limit)`: lỗi `ERROR_TIMEOUT`.
6. Khi có lỗi: đặt `g_state = STATE_ERROR`, ghi `g_error_code`, xóa các mốc thời gian (`was_conveyor_running = 0`, `sensor_active_tracking = 0`), trả `1U`. Nếu không có lỗi, trả `0U`.

Hai lỗi cùng thỏa trong một lần gọi: ưu tiên `ERROR_CONTINUOUSLY_ACTIVE` vì chỉ ra sự cố cụ thể hơn.

### 5.5. Ý nghĩa của `ERROR_TIMEOUT`

`ERROR_TIMEOUT` xảy ra khi không có lượt phân loại thành công trong `timeout_limit`. Điều kiện này bao gồm:

- Không có sản phẩm nào đi qua.
- Sản phẩm đi qua nhưng không phân loại được (vật chạy quá nhanh, tổ hợp cảm biến không hợp lệ).
- Cảm biến hỏng nên không phân loại được.

Vì vậy bộ đếm timeout chỉ được đặt lại bởi `classified = 1`, không dựa vào mức cảm biến.

Theo từng chế độ:

- `MODE_AUTO`: băng tải chạy ngay khi `STATE_RUNNING` nên thời gian được tính ngay khi vào `STATE_RUNNING` (lần gọi `Status_Update()` đầu tiên có `conveyorRunning = 1`).
- `MODE_MANUAL`: chỉ tính khi băng tải đang chạy. Băng tải dừng thì mốc bị hủy, chạy lại thì tính lại từ 0.

Thời gian T2 của `ERROR_CONTINUOUSLY_ACTIVE` cũng tính theo quy tắc băng tải chạy như trên.

### 5.6. Khởi tạo

`Status_Init()`:

1. `g_state = STATE_STOP`, `g_mode = MODE_AUTO`, `g_error_code = ERROR_NONE`.
2. `was_conveyor_running = 0`, `sensor_active_tracking = 0`.

## 6. Dependencies

| Thành phần | Quan hệ | Mô tả |
|---|---|---|
| `main` | Gọi API | Gọi API đổi trạng thái theo nút nhấn, gọi `Status_Update()` mỗi vòng lặp, đọc trạng thái |
| `timer` | Phụ thuộc | `Timer_GetTick()`, `Timer_IsElapsed()` để tính timeout và T2 |
| `buttons` | Không phụ thuộc | `main` chuyển sự kiện nút thành lời gọi API của `status` |
| `sensor` | Không phụ thuộc | `main` truyền mức cảm biến và cờ `classified` qua tham số |
| `conveyor` | Không phụ thuộc | `main` truyền `conveyorRunning` qua tham số |
| `indicator`, `LCD`, `servo`, `data` | Không phụ thuộc | `main` đọc trạng thái từ `status` rồi ra lệnh cho các module này |

## 7. Testing

### Test 1: Khởi tạo

- Gọi `Status_Init()`.
- Kiểm tra `STATE_STOP`, `MODE_AUTO`, `ERROR_NONE`.

### Test 2: Start hợp lệ và không hợp lệ

- Ở `STATE_STOP`, gọi `Status_Start()`: trả `1U`, trạng thái thành `STATE_RUNNING`.
- Ở `STATE_RUNNING` và `STATE_ERROR`, gọi `Status_Start()`: trả `0U`, trạng thái không đổi.

### Test 3: Stop

- Ở `STATE_RUNNING`, gọi `Status_Stop()`: trả `1U`, trạng thái thành `STATE_STOP`.
- Ở `STATE_STOP` và `STATE_ERROR`: trả `0U`, trạng thái không đổi.

### Test 4: Đổi chế độ

- Ở `STATE_STOP`, gọi `Status_ToggleMode()`: trả `1U`, chế độ đổi giữa AUTO và MANUAL.
- Ở `STATE_RUNNING` và `STATE_ERROR`: trả `0U`, chế độ không đổi.

### Test 5: Xóa lỗi

- Ở `STATE_ERROR`, gọi `Status_ClearError()`: trả `1U`, về `STATE_STOP`, `ERROR_NONE`.
- Ở `STATE_STOP` và `STATE_RUNNING`: trả `0U`.
- Kiểm tra không thể đi trực tiếp từ `STATE_ERROR` sang `STATE_RUNNING`.

### Test 6: ERROR_TIMEOUT ở AUTO

- Vào `STATE_RUNNING`, `MODE_AUTO`, `conveyorRunning = 1`, không có `classified`.
- Sau `timeout_limit`: `Status_Update()` trả `1U`, trạng thái `STATE_ERROR`, mã `ERROR_TIMEOUT`.
- Các lần gọi sau trả `0U`.

### Test 7: Lượt phân loại đặt lại timeout

- Truyền `classified = 1` trước khi hết `timeout_limit`.
- Kiểm tra không báo lỗi tại mốc cũ, và lỗi chỉ xảy ra sau đủ `timeout_limit` kể từ lượt phân loại.
- Kiểm tra `classified = 1` đúng lúc hết thời gian không bị báo lỗi.

### Test 8: Timeout ở MANUAL

- `conveyorRunning = 0`: không báo lỗi dù quá `timeout_limit`.
- Cho `conveyorRunning = 1`: thời gian tính lại từ 0.
- Cho `conveyorRunning = 0` rồi 1 lại: thời gian tính lại từ 0.

### Test 9: ERROR_CONTINUOUSLY_ACTIVE

- `conveyorRunning = 1`, một cảm biến ở mức LOW liên tục quá `sensor_active_limit`.
- Kiểm tra báo `ERROR_CONTINUOUSLY_ACTIVE`.
- Trường hợp cảm biến lên HIGH trước khi hết `sensor_active_limit`: không báo lỗi, thời gian tính lại từ lần LOW sau.

### Test 10: Băng tải dừng thì bỏ qua cảm biến

- `conveyorRunning = 0`, cảm biến ở LOW liên tục quá `sensor_active_limit`.
- Kiểm tra không báo lỗi.

### Test 11: Hai lỗi cùng lúc

- Tạo tình huống cả hai điều kiện lỗi cùng thỏa trong một lần gọi.
- Kiểm tra mã lỗi là `ERROR_CONTINUOUSLY_ACTIVE`.

### Test 12: Không phát hiện lỗi ngoài STATE_RUNNING

- Ở `STATE_STOP` và `STATE_ERROR`, gọi `Status_Update()` với cảm biến LOW kéo dài và quá thời gian.
- Kiểm tra trả `0U` và trạng thái không đổi.

### Test 13: Start lại sau Stop hoặc ClearError

- Dừng hoặc xóa lỗi, rồi `Status_Start()`.
- Kiểm tra các mốc thời gian được tính lại từ đầu, không bị lỗi ngay do mốc cũ.

## Các thông số và quyết định chưa chốt

| Nội dung | Trạng thái |
|---|---|
| Giá trị `timeout_limit` | 30000 ms (theo `system_requirements.md`) |
| Giá trị `sensor_active_limit` (T2) | Tạm 2000 ms, chỉnh khi thử nghiệm; phải lớn hơn T1 và thời gian một sản phẩm đi qua |
| Xử lý hành động khi vào `STATE_ERROR` (dừng băng tải, servo, LED, còi, hủy lượt phân loại) | Thuộc `main`, không thuộc `status` |
| Hai lỗi trùng nhau | Ưu tiên `ERROR_CONTINUOUSLY_ACTIVE` |
