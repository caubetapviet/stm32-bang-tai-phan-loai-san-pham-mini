# MODULE BUTTONS

## 1. Overview

Module `buttons` quản lý việc tiếp nhận thao tác từ người dùng thông qua sáu nút nhấn vật lý.

Module sử dụng phương pháp polling để đọc trạng thái GPIO, phát hiện thao tác nhấn và cung cấp sự kiện tương ứng cho module `main`.

Module có cơ chế khóa đầu vào nội bộ nhằm đảm bảo mỗi chu kỳ nhấn–nhả chỉ tạo ra một sự kiện.

## 2. Responsibilities

- Khởi tạo và đọc trạng thái sáu nút nhấn.
- Phát hiện thao tác nhấn nút dựa trên tín hiệu active-low.
- Xác định nút được nhấn và trả về sự kiện tương ứng.
- Khóa toàn bộ đầu vào sau khi ghi nhận một sự kiện.
- Chỉ mở khóa khi tất cả các nút đã được nhả.
- Khởi động ở trạng thái khóa nếu phát hiện có nút đang được giữ.

### Lưu ý: Module hiện chưa được thiết xử lý debounce theo `requirement` và không quyết định chức năng điều khiển của từng nút theo trạng thái hệ thống.

## 3. Input / Output

### Input

Sáu nút nhấn vật lý:

| Nút nhấn | GPIO | Sự kiện |
|---|---|---|
| START | PB12 | `BUTTON_EVENT_START` |
| STOP | PB13 | `BUTTON_EVENT_STOP` |
| MODE | PB14 | `BUTTON_EVENT_MODE` |
| RESET | PB15 | `BUTTON_EVENT_RESET` |
| CONVEYOR | PA12 | `BUTTON_EVENT_CONVEYOR` |
| SERVO | PA11 | `BUTTON_EVENT_SERVO` |

Các nút sử dụng cấu hình active-low. Khi nhấn, GPIO ở mức LOW; khi nhả, GPIO ở mức HIGH.

### Output

Module trả về một giá trị `ButtonEvent_t`, biểu thị nút được nhấn hoặc không có sự kiện.

## 4. API Design

### `Buttons_Init()`

```c
void Buttons_Init(void);
```

Khởi tạo module và kiểm tra trạng thái các nút tại thời điểm khởi động.

Nếu có ít nhất một nút đang được nhấn, module sẽ khóa đầu vào cho đến khi tất cả các nút được nhả.

### `Buttons_Update()`

```c
ButtonEvent_t Buttons_Update(void);
```

Đọc trạng thái các nút và cập nhật cơ chế khóa đầu vào.

Giá trị trả về thuộc kiểu `ButtonEvent_t`:

- `BUTTON_EVENT_NONE`
- `BUTTON_EVENT_START`
- `BUTTON_EVENT_STOP`
- `BUTTON_EVENT_MODE`
- `BUTTON_EVENT_RESET`
- `BUTTON_EVENT_CONVEYOR`
- `BUTTON_EVENT_SERVO`

Mỗi lần gọi chỉ trả về tối đa một sự kiện.

## 5. Internal Design

### 5.1. Cấu trúc dữ liệu

- `Button_t`: lưu GPIO port, GPIO pin và sự kiện tương ứng của từng nút.
- `buttons[BUTTON_COUNT]`: mảng chứa thông tin sáu nút nhấn.
- `inputLocked`: biến trạng thái khóa đầu vào.

### 5.2. Hàm nội bộ

`ReadButton()` đọc trạng thái GPIO và chuyển đổi thành giá trị logic:

- `1`: nút đang được nhấn.
- `0`: nút đang được nhả.

### 5.3. Cơ chế khóa đầu vào

- Khi không khóa, module quét các nút theo thứ tự khai báo trong mảng `buttons`.
- Khi phát hiện nút đầu tiên được nhấn, module trả về sự kiện tương ứng và đặt `inputLocked = 1`.
- Khi đang khóa, module không phát sinh sự kiện mới.
- Khi tất cả các nút đã được nhả, module đặt `inputLocked = 0` và kết thúc lần cập nhật đó mà không phát sinh sự kiện.

Cơ chế này đảm bảo một chu kỳ nhấn–nhả chỉ tạo ra một sự kiện, nhưng không thay thế chức năng debounce.

## 6. Dependencies

- STM32 HAL GPIO: sử dụng `HAL_GPIO_ReadPin()` để đọc trạng thái các nút.
- Module sử dụng GPIO đã được cấu hình trong quá trình khởi tạo phần cứng.

Không phụ thuộc vào module `timer`.

## 7. Testing

- Kiểm tra khả năng nhận diện từng nút nhấn.
- Kiểm tra tín hiệu active-low.
- Kiểm tra mỗi chu kỳ nhấn–nhả chỉ tạo ra một sự kiện.
- Kiểm tra cơ chế khóa khi giữ nút.
- Kiểm tra chỉ mở khóa khi tất cả các nút đã được nhả.
- Kiểm tra trường hợp khởi động khi có nút đang được giữ.
- Kiểm tra thứ tự ưu tiên khi nhiều nút được nhấn cùng lúc.
- Kiểm tra ảnh hưởng của hiện tượng dội phím do module không có debounce.
