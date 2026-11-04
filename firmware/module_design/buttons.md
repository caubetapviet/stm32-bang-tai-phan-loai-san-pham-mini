# MODULE BUTTONS

## 1. Overview

Module `buttons` quản lý việc tiếp nhận thao tác từ người dùng thông qua sáu nút nhấn vật lý.

Module sử dụng phương pháp polling để đọc trạng thái GPIO, lọc nhiễu dội phím (debounce 20 ms), phát hiện thao tác nhấn và cung cấp sự kiện tương ứng cho module `main`.

Module có cơ chế khóa đầu vào nội bộ nhằm đảm bảo mỗi chu kỳ nhấn–nhả chỉ tạo ra một sự kiện.

## 2. Responsibilities

- Khởi tạo và đọc trạng thái sáu nút nhấn.
- Phát hiện thao tác nhấn nút dựa trên tín hiệu active-low.
- Lọc nhiễu dội phím: chỉ chấp nhận một nút khi nút đó được giữ ổn định đủ `BUTTON_DEBOUNCE_MS` (20 ms).
- Xác định nút được nhấn và trả về sự kiện tương ứng.
- Khóa toàn bộ đầu vào sau khi ghi nhận một sự kiện.
- Chỉ mở khóa khi tất cả các nút đã được nhả.
- Khởi động ở trạng thái khóa nếu phát hiện có nút đang được giữ.

Module không chịu trách nhiệm:

- Quyết định chức năng điều khiển của từng nút theo trạng thái hệ thống (do `main` và `status` xử lý).
- Phân biệt nhấn nhanh hay nhấn giữ.

## 3. Input / Output

### 3.1. Input

Sáu nút nhấn vật lý:

| Nút nhấn | GPIO | Sự kiện |
|---|---|---|
| START | PB12 | `BUTTON_EVENT_START` |
| STOP | PB13 | `BUTTON_EVENT_STOP` |
| MODE | PB14 | `BUTTON_EVENT_MODE` |
| RESET | PB15 | `BUTTON_EVENT_RESET` |
| CONVEYOR | PA12 | `BUTTON_EVENT_CONVEYOR` |
| SERVO | PA11 | `BUTTON_EVENT_SERVO` |

Các nút sử dụng cấu hình active-low (GPIO Input, Pull-up). Khi nhấn, GPIO ở mức LOW; khi nhả, GPIO ở mức HIGH.

Ngoài ra module dùng `Timer_GetTick()` / `Timer_IsElapsed()` của module `timer` để tính thời gian debounce.

### 3.2. Output

Module trả về một giá trị `ButtonEvent_t`, biểu thị nút được nhấn hoặc không có sự kiện.

## 4. API Design

### 4.1. Kiểu dữ liệu

```c
typedef enum
{
    BUTTON_EVENT_NONE = 0U,
    BUTTON_EVENT_START,
    BUTTON_EVENT_STOP,
    BUTTON_EVENT_MODE,
    BUTTON_EVENT_RESET,
    BUTTON_EVENT_CONVEYOR,
    BUTTON_EVENT_SERVO
} ButtonEvent_t;
```

### 4.2. Danh sách API

```c
void Buttons_Init(void);
ButtonEvent_t Buttons_Update(void);
```

| API | Chức năng |
|---|---|
| `Buttons_Init()` | Khởi tạo module và kiểm tra trạng thái các nút tại thời điểm khởi động. Nếu có ít nhất một nút đang được nhấn thì khóa đầu vào cho đến khi tất cả các nút được nhả. Gọi một lần khi khởi động, sau `Timer_Init()` và sau khi đã khởi động ngắt TIM1. |
| `Buttons_Update()` | Đọc trạng thái các nút, xử lý debounce và cơ chế khóa đầu vào. `main` gọi mỗi vòng lặp. Mỗi lần gọi chỉ trả về tối đa một sự kiện. |

### 4.3. Ví dụ sử dụng trong `main`

```c
Timer_Init();
HAL_TIM_Base_Start_IT(&htim1);
Buttons_Init();

/* Trong vòng lặp chính */
switch (Buttons_Update())
{
    case BUTTON_EVENT_START:
        /* xử lý nút START */
        break;

    case BUTTON_EVENT_STOP:
        /* xử lý nút STOP */
        break;

    default:
        break;
}
```

## 5. Internal Design

### 5.1. Cấu trúc dữ liệu

- `Button_t`: lưu GPIO port, GPIO pin và sự kiện tương ứng của từng nút.
- `buttons[BUTTON_COUNT]`: mảng chứa thông tin sáu nút nhấn.

| Biến | Kiểu | Chức năng |
|---|---|---|
| `inputLocked` | `uint8_t` | `1U`: đầu vào đang bị khóa, `0U`: đang nhận nút |
| `candidate` | `uint8_t` | Chỉ số nút đang chờ debounce, `BUTTON_NONE_INDEX` nếu không có |
| `candidateStart` | `uint32_t` | Mốc thời gian bắt đầu debounce của nút `candidate` |

### 5.2. Tham số cấu hình

| Tham số | Ý nghĩa | Giá trị |
|---|---|---|
| `BUTTON_COUNT` | Số nút nhấn | 6 |
| `BUTTON_DEBOUNCE_MS` | Thời gian giữ ổn định để chấp nhận một lần nhấn (ms) | 20 |
| `BUTTON_NONE_INDEX` | Giá trị chỉ số biểu thị "không có nút nào" | `0xFF` |

### 5.3. Hàm nội bộ

| Hàm | Chức năng |
|---|---|
| `ReadButton()` | Đọc GPIO và chuyển thành giá trị logic: `1U` nút đang nhấn, `0U` nút đang nhả |
| `FindPressed()` | Quét các nút theo thứ tự trong mảng `buttons`, trả về chỉ số nút đầu tiên đang nhấn, hoặc `BUTTON_NONE_INDEX` nếu không có |

### 5.4. Logic `Buttons_Update()`

Mỗi lần gọi, module gọi `FindPressed()` rồi xử lý theo thứ tự:

1. **Đang khóa (`inputLocked = 1U`):**
   - Nếu không có nút nào đang nhấn: mở khóa (`inputLocked = 0U`), hủy `candidate`.
   - Trả `BUTTON_EVENT_NONE`.
2. **Không có nút nào nhấn:** hủy `candidate` (nhiễu dội hoặc nút đã nhả trước 20 ms), trả `BUTTON_EVENT_NONE`.
3. **Nút khác với `candidate` hiện tại** (nút mới hoặc đổi nút): đặt `candidate` bằng nút đó, `candidateStart = Timer_GetTick()`, trả `BUTTON_EVENT_NONE`.
4. **Cùng một nút với `candidate`:**
   - Nếu `Timer_IsElapsed(candidateStart, BUTTON_DEBOUNCE_MS)` đúng: đặt `inputLocked = 1U`, hủy `candidate`, trả sự kiện của nút đó.
   - Ngược lại: trả `BUTTON_EVENT_NONE`.

### 5.5. Cơ chế debounce

- Một nút chỉ được chấp nhận khi `FindPressed()` liên tục trả về cùng một nút trong ít nhất `BUTTON_DEBOUNCE_MS`.
- Nếu nút nhả ra hoặc `FindPressed()` trả về nút khác trước khi đủ thời gian, mốc debounce được tính lại.
- Debounce chỉ áp dụng cho chiều nhấn. Chiều nhả không debounce riêng: nếu nút dội lúc nhả và module đọc thấy "đã nhả hết" thì mở khóa, nhưng lần nhấn giả sau đó cũng phải giữ đủ 20 ms mới thành sự kiện nên không phát sinh sự kiện thừa.
- Sự kiện được phát ra sau khi nút đã giữ đủ 20 ms, không phát ngay lúc phát hiện cạnh nhấn.

### 5.6. Cơ chế khóa đầu vào

- Khi không khóa, module quét các nút theo thứ tự khai báo trong mảng `buttons`.
- Khi một nút giữ ổn định đủ thời gian debounce, module trả về sự kiện tương ứng và đặt `inputLocked = 1`.
- Khi đang khóa, module không phát sinh sự kiện mới.
- Khi tất cả các nút đã được nhả, module đặt `inputLocked = 0` và kết thúc lần cập nhật đó mà không phát sinh sự kiện.
- Nếu nhiều nút được nhấn cùng lúc, nút đứng trước trong mảng `buttons` (START, STOP, MODE, RESET, CONVEYOR, SERVO) được ưu tiên.

Cơ chế này đảm bảo một chu kỳ nhấn–nhả chỉ tạo ra một sự kiện, kể cả khi người dùng giữ nút lâu.

## 6. Dependencies

| Thành phần | Quan hệ | Mô tả |
|---|---|---|
| `main` | Gọi API | Gọi `Buttons_Init()` một lần, gọi `Buttons_Update()` mỗi vòng lặp và xử lý sự kiện |
| `timer` | Phụ thuộc | `Timer_GetTick()`, `Timer_IsElapsed()` để tính thời gian debounce |
| STM32 HAL GPIO | Phụ thuộc | `HAL_GPIO_ReadPin()` để đọc trạng thái các nút |

Module sử dụng GPIO đã được cấu hình trong quá trình khởi tạo phần cứng (`MX_GPIO_Init()`).

Điều kiện chạy đúng:

- `Timer_Init()` và `HAL_TIM_Base_Start_IT(&htim1)` phải được gọi trước `Buttons_Init()`. Nếu TIM1 chưa chạy thì `Timer_GetTick()` đứng yên, không nút nào đạt đủ thời gian debounce.
- `Buttons_Update()` phải được gọi thường xuyên ở mỗi vòng lặp chính, không bị chặn bởi `HAL_Delay()` hoặc vòng chờ khác.
- Clock của chip phải đúng (8 MHz) để tick 1 ms và thời gian 20 ms chính xác.

## 7. Testing

- Kiểm tra khả năng nhận diện từng nút nhấn.
- Kiểm tra tín hiệu active-low.
- Kiểm tra mỗi chu kỳ nhấn–nhả chỉ tạo ra một sự kiện.
- Kiểm tra debounce: nhấn rồi nhả trong thời gian ngắn hơn 20 ms thì không có sự kiện.
- Kiểm tra nhấn giữ đủ 20 ms thì có đúng một sự kiện, sau đó giữ tiếp không phát thêm sự kiện.
- Kiểm tra cơ chế khóa khi giữ nút.
- Kiểm tra chỉ mở khóa khi tất cả các nút đã được nhả.
- Kiểm tra trường hợp khởi động khi có nút đang được giữ.
- Kiểm tra thứ tự ưu tiên khi nhiều nút được nhấn cùng lúc.
- Kiểm tra đổi nút trong lúc đang debounce: mốc thời gian được tính lại cho nút mới.

## Các thông số và quyết định chưa chốt

| Nội dung | Trạng thái |
|---|---|
| `BUTTON_DEBOUNCE_MS` | 20 ms (theo `system_requirements.md`), chỉnh nếu nút thực tế dội lâu hơn |
| Debounce chiều nhả | Không làm, lần nhấn giả khi nhả phải giữ đủ 20 ms mới thành sự kiện |
| Kiểm thử | Đã chạy tốt trên Proteus (bản có debounce và khóa đầu vào) |
