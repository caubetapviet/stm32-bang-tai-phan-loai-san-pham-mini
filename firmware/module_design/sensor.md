# MODULE SENSOR

## 1. Overview

Module `sensor` chịu trách nhiệm đọc trạng thái hai cảm biến, nhận diện sản phẩm A/B và phát sự kiện phân loại cho module `main`.

Module hoạt động độc lập với logic điều khiển hệ thống. Việc xử lý sản phẩm sau khi nhận diện thuộc trách nhiệm của `main`.

Hai cảm biến sử dụng:

- `CB_Thap`: phát hiện tất cả sản phẩm.
- `CB_Cao`: chỉ phát hiện sản phẩm loại A.

Cả hai cảm biến đều active-low.

## 2. Responsibilities

Module `sensor` có các nhiệm vụ:

1. Đọc trạng thái GPIO của hai cảm biến.
2. Chuyển đổi tín hiệu GPIO thành trạng thái cảm biến logic.
3. Nhận diện sản phẩm A/B sau khoảng thời gian xác nhận `T1`.
4. Phát sự kiện phân loại cho `main`.
5. Cung cấp trạng thái hiện tại của hai cảm biến.
6. Chỉ thực hiện nhận diện khi được kích hoạt bằng `Sensor_Start()`.
7. Hủy lượt nhận diện đang chờ khi nhận `Sensor_Stop()`.

Module không chịu trách nhiệm:

- Quyết định trạng thái hệ thống.
- Quyết định chế độ AUTO/MANUAL.
- Điều khiển conveyor hoặc servo.
- Cập nhật bộ đếm sản phẩm.
- Xử lý lỗi hệ thống.

## 3. Input / Output

### 3.1. Input

| Input             | Mô tả                  |
| ----------------- | ---------------------- |
| GPIO CB_Thap      | Tín hiệu cảm biến thấp |
| GPIO CB_Cao       | Tín hiệu cảm biến cao  |
| `Sensor_Start()`  | Cho phép nhận diện     |
| `Sensor_Stop()`   | Dừng nhận diện         |
| `Timer_GetTick()` | Lấy thời gian hệ thống |

### 3.2. Output

| Output                   | Mô tả                                |
| ------------------------ | ------------------------------------ |
| `SENSOR_EVENT_PRODUCT_A` | Nhận diện sản phẩm A                 |
| `SENSOR_EVENT_PRODUCT_B` | Nhận diện sản phẩm B                 |
| `SENSOR_EVENT_NONE`      | Không có sự kiện phân loại           |
| `SensorState_t`          | Trạng thái hiện tại của hai cảm biến |

### 3.3. Quy tắc nhận diện

| CB_Thap | CB_Cao | Kết quả         |
| ------- | ------ | --------------- |
| LOW     | LOW    | Sản phẩm A      |
| LOW     | HIGH   | Sản phẩm B      |
| HIGH    | LOW    | Không nhận diện |
| HIGH    | HIGH   | Không nhận diện |

## 4. API Design

### 4.1. Khai báo kiểu dữ liệu

```
typedef enum
{
    SENSOR_EVENT_NONE = 0,
    SENSOR_EVENT_PRODUCT_A,
    SENSOR_EVENT_PRODUCT_B
} SensorEvent_t;

typedef struct
{
    uint8_t cb_thap_active;
    uint8_t cb_cao_active;
} SensorState_t;
```

### 4.2. Danh sách API

```
void Sensor_Init(void);
void Sensor_Start(void);
void Sensor_Stop(void);

SensorEvent_t Sensor_Update(void);
void Sensor_GetState(SensorState_t *state);
```

### 4.3. Chức năng từng API

| API                 | Chức năng                                                                       |
| ------------------- | ------------------------------------------------------------------------------- |
| `Sensor_Init()`     | Khởi tạo biến nội bộ và trạng thái ban đầu                                      |
| `Sensor_Start()`    | Bắt đầu nhận diện, chuyển state machine sang `SENSOR_READY`                     |
| `Sensor_Stop()`     | Dừng nhận diện, hủy lượt đang chờ và chuyển state machine sang `SENSOR_STOPPED` |
| `Sensor_Update()`   | Đọc cảm biến, cập nhật state machine và trả về sự kiện phân loại                |
| `Sensor_GetState()` | Lấy trạng thái cảm biến gần nhất đã được cập nhật                               |

`Sensor_GetState()` không tự đọc GPIO và không thực hiện nhận diện. Module `main` cần gọi `Sensor_Update()` trước khi lấy trạng thái mới nhất.

## 5. Internal Design

### 5.1. State machine

Module sử dụng bốn trạng thái:

```
typedef enum
{
    SENSOR_STOPPED = 0,
    SENSOR_READY,
    SENSOR_WAIT_T1,
    SENSOR_WAIT_CLEAR
} SensorStateMachine_t;
```

| State               | Chức năng                                  |
| ------------------- | ------------------------------------------ |
| `SENSOR_STOPPED`    | Không thực hiện nhận diện                  |
| `SENSOR_READY`      | Sẵn sàng nhận sản phẩm mới                 |
| `SENSOR_WAIT_T1`    | Chờ hết thời gian T1 trước khi đọc kết quả |
| `SENSOR_WAIT_CLEAR` | Chờ cả hai cảm biến trở về HIGH            |

### 5.2. Thời gian xác nhận T1

- `T1 = 50ms` (giá trị tạm thời đã thống nhất).
- Sử dụng `Timer_GetTick()` để xác định thời gian.
- Không sử dụng `HAL_Delay()`.
- T1 sẽ được điều chỉnh sau khi kiểm thử thực tế.

### 5.3. Nguyên tắc hoạt động

- `Sensor_Init()` đưa module về `SENSOR_STOPPED`.
- `Sensor_Start()` xóa dữ liệu nhận diện đang chờ và chuyển sang `SENSOR_READY` ngay lập tức.
- Khi một trong hai cảm biến LOW tại `SENSOR_READY`, module ghi nhận thời điểm bắt đầu và chuyển sang `SENSOR_WAIT_T1`.
- Khi T1 kết thúc, module đọc trạng thái cả hai cảm biến và xác định A, B hoặc không nhận diện.
- Sau khi kiểm tra, bất kể kết quả là A, B hay không hợp lệ, module chuyển sang `SENSOR_WAIT_CLEAR`.
- Chỉ khi cả hai cảm biến HIGH, module mới quay về `SENSOR_READY`.

### 5.4. Xử lý Sensor_Stop()

Khi gọi `Sensor_Stop()`:

1. Chuyển state machine sang `SENSOR_STOPPED`.
2. Hủy lượt nhận diện đang chờ.
3. Xóa dữ liệu sự kiện đang chờ và timestamp liên quan.
4. Không tiếp tục nhận diện hoặc phát sự kiện mới.

Nếu cảm biến vẫn đang LOW trên thực tế, module cũng không được tự động nhận diện sản phẩm. Chỉ khi gọi `Sensor_Start()` thì quá trình nhận diện mới được kích hoạt lại.

### 5.5. Luồng hoạt động


## 6. Dependencies

| Module   | Quan hệ                                      |
| -------- | -------------------------------------------- |
| `main`   | Gọi API điều khiển và nhận sự kiện phân loại |
| `timer`  | Cung cấp tick hệ thống để tính T1            |
| GPIO HAL | Đọc tín hiệu hai cảm biến                    |

Module `main` chịu trách nhiệm quyết định hành động dựa trên sự kiện phân loại và trạng thái hệ thống.

Module `sensor` không phụ thuộc vào `status`, `data`, `conveyor` hoặc `servo`.

## 7. Testing

### Test 1 – Khởi tạo

- Gọi `Sensor_Init()`.
- Kiểm tra state machine ở `SENSOR_STOPPED`.
- Không phát sinh sự kiện phân loại.

### Test 2 – Start

- Gọi `Sensor_Start()`.
- Kiểm tra state machine chuyển sang `SENSOR_READY`.
- Kiểm tra module sẵn sàng ngay cả khi cảm biến đang LOW.

### Test 3 – Nhận diện A

- Đưa CB_Thap và CB_Cao về LOW.
- Chờ T1.
- Kiểm tra sự kiện `SENSOR_EVENT_PRODUCT_A`.

### Test 4 – Nhận diện B

- Đưa CB_Thap về LOW và CB_Cao về HIGH.
- Chờ T1.
- Kiểm tra sự kiện `SENSOR_EVENT_PRODUCT_B`.

### Test 5 – Tổ hợp không hợp lệ

- Kiểm tra HIGH/LOW và HIGH/HIGH.
- Không phát sinh sự kiện A hoặc B.
- Sau đó vẫn phải chờ cả hai cảm biến HIGH.

### Test 6 – Chờ cảm biến nhả

- Sau một lượt nhận diện, giữ ít nhất một cảm biến LOW.
- Kiểm tra module vẫn ở `SENSOR_WAIT_CLEAR`.
- Khi cả hai HIGH, kiểm tra module trở về `SENSOR_READY`.

### Test 7 – Stop khi đang chờ T1

- Gọi `Sensor_Stop()` khi module ở `SENSOR_WAIT_T1`.
- Kiểm tra lượt nhận diện bị hủy.
- Kiểm tra state machine chuyển sang `SENSOR_STOPPED`.
- Không phát sinh sự kiện sau khi T1 kết thúc.

### Test 8 – Stop khi cảm biến đang kích hoạt

- Gọi `Sensor_Stop()` khi một hoặc cả hai cảm biến LOW.
- Kiểm tra state machine chuyển sang `SENSOR_STOPPED`.
- Kiểm tra module không nhận diện và không phát sự kiện.

### Test 9 – Start lại khi cảm biến vẫn LOW

- Sau khi Stop, gọi `Sensor_Start()` khi cảm biến vẫn LOW.
- Kiểm tra module bắt đầu từ trạng thái `SENSOR_READY`.
- Kiểm tra module thực hiện quy trình T1 bình thường.

Ghi chú: Đây là bản thiết kế module, chưa phải mã nguồn triển khai và chưa được kiểm thử trên phần cứng.
