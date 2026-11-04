# MODULE MAIN

## 1. Overview

`main.c` là nơi điều phối toàn bộ firmware. `main` đọc thông tin từ các module đầu vào (`buttons`, `sensor`, `timer`), hỏi `status` xem yêu cầu có hợp lệ không, rồi ra lệnh cho các module chức năng (`conveyor`, `servo`, `indicator`, `data`, `LCD`, `IoT`).

`main` không viết lại logic của các module. Mỗi quyết định thuộc về module tương ứng:

- Trạng thái, chế độ, lỗi: `status`.
- Nhận diện sản phẩm: `sensor`.
- Bộ đếm và lưu ROM: `data`.
- Chạy/dừng băng tải, vị trí servo, đèn và còi, hiển thị, gửi IoT: các module thiết bị.

`main` chỉ làm hai việc: **chuyển sự kiện thành lời gọi API** và **thực hiện các hành động đi kèm khi đổi trạng thái** (dừng băng tải, đưa servo về `Non-gate`...).

Toàn bộ code của `main` viết trong các vùng `USER CODE` của CubeMX để không mất khi tạo lại project. Không dùng hàm phụ, mọi logic nằm trực tiếp trong `while (1)` để dễ theo dõi.

## 2. Responsibilities

- Khởi tạo các module theo thứ tự đã định (mục 4).
- Mỗi vòng lặp: đọc nút nhấn, đọc cảm biến, kiểm tra lỗi, cập nhật các việc nền.
- Chuyển sự kiện nút nhấn thành lời gọi API của `status`, và kiểm tra điều kiện cho `BTN_CONVEYOR`, `BTN_SERVO`.
- Khi đổi trạng thái (`STATE_RUNNING`, `STATE_STOP`, `STATE_ERROR`), ra lệnh cho `conveyor`, `servo`, `sensor`, `indicator`.
- Cập nhật bộ đếm khi `sensor` báo sản phẩm A hoặc B, ra lệnh gạt servo ở `MODE_AUTO`.
- Truyền dữ liệu cho `LCD` và `IoT`.

Module không chịu trách nhiệm:

- Cấu hình ngoại vi (do CubeMX).
- Nhận diện sản phẩm, tính T1, T2, timeout, `T_delay`, `T_hold` (do `sensor`, `status`, `servo`).
- Ghi Flash, ghép chuỗi hiển thị, gửi UART (do `data`, `LCD`, `IoT`).

## 3. Input / Output

### 3.1. Input

| Input | Nguồn | Mô tả |
|---|---|---|
| `Buttons_Update()` | `buttons` | Sự kiện nút nhấn (tối đa một sự kiện mỗi vòng lặp) |
| `Sensor_Update()` | `sensor` | Sự kiện phân loại `SENSOR_EVENT_PRODUCT_A` / `PRODUCT_B` / `NONE` |
| `Sensor_GetState()` | `sensor` | Mức hai cảm biến, truyền cho `Status_Update()` |
| `Conveyor_IsRunning()` | `conveyor` | Băng tải đang chạy hay dừng, truyền cho `Status_Update()` |
| `Status_GetState/Mode/Error()` | `status` | Trạng thái, chế độ, mã lỗi hiện tại |
| `Data_GetCountTotal/A/B()` | `data` | Bộ đếm từ RAM |

### 3.2. Output

| Đích | Lệnh |
|---|---|
| `status` | `Status_Start/Stop/ToggleMode/ClearError()`, `Status_Update()` |
| `conveyor` | `Conveyor_Start/Stop/Toggle()` |
| `servo` | `Servo_StartSort/Toggle/Reset/Update()` |
| `sensor` | `Sensor_Start/Stop()` |
| `data` | `Data_IncrementA/B()`, `Data_Update()`, `Data_Reset()` |
| `indicator` | `Indicator_SetRun()`, `Indicator_SetFault()` |
| `LCD`, `IoT` | `LCD_Update()`, `IoT_Update()` |

## 4. Thứ tự khởi tạo

CubeMX sinh sẵn: `HAL_Init()`, `SystemClock_Config()`, `MX_GPIO_Init()`, `MX_TIM1_Init()`, `MX_USART1_UART_Init()`, `MX_TIM2_Init()`, `MX_TIM4_Init()`.

Code người dùng đặt trong `USER CODE BEGIN 2`:

```c
Timer_Init();
HAL_TIM_Base_Start_IT(&htim1);   /* tick 1 ms chạy trước mọi module cần thời gian */

Indicator_Init();                /* LED, còi tắt */
Conveyor_Init();                 /* IN1 = IN2 = 0, start PWM TIM4 */
Servo_Init();                    /* Non-gate, start PWM TIM2 */

Data_Init();                     /* nạp bộ đếm từ Flash */
Status_Init();                   /* STATE_STOP, MODE_AUTO */
Sensor_Init();                   /* SENSOR_STOPPED */
Buttons_Init();                  /* khóa nếu đang giữ nút lúc cấp điện */

LCD_Init();                      /* tốn khoảng 60 ms vì HAL_Delay */
IoT_Init();                      /* mốc 3 giây tính sau khi LCD xong */
```

Lý do sắp xếp:

- `Timer_Init()` và start TIM1 đứng đầu vì `Data`, `Status`, `Servo`, `LCD`, `IoT` đều dùng `Timer_GetTick()`.
- Các thiết bị chấp hành (đèn, còi, băng tải, servo) khởi tạo sớm để ở trạng thái an toàn ngay khi cấp điện.
- `LCD_Init()` dùng `HAL_Delay()` (SysTick) nên không phụ thuộc TIM1. Nó đứng trước `IoT_Init()` để chu kỳ 3 giây không bị tính vào thời gian khởi tạo LCD.
- Khi cấp điện luôn vào `STATE_STOP` và `MODE_AUTO` (do `Status_Init()`).

`#include` các module đặt trong `USER CODE BEGIN Includes`: `timer.h`, `buttons.h`, `sensor.h`, `status.h`, `data.h`, `conveyor.h`, `servo.h`, `indicator.h`, `lcd.h`, `iot.h`. Không đưa `lcd_i2c.h` vào project Keil (xem `lcd.md`).

## 5. Cấu trúc vòng lặp chính

Thứ tự trong mỗi vòng lặp: **nút nhấn, cảm biến và đếm, phát hiện lỗi, việc nền**. Vòng lặp không có `HAL_Delay()` hay vòng chờ nào.

```c
/* USER CODE BEGIN WHILE */
while (1)
{
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */

    /* ===== 1. Nút nhấn ===== */
    switch (Buttons_Update())
    {
        case BUTTON_EVENT_START:
            if (Status_Start())
            {
                Servo_Reset();
                Sensor_Start();
                Indicator_SetRun(1U);
                if (Status_GetMode() == MODE_AUTO) { Conveyor_Start(); }
                else                               { Conveyor_Stop();  }
            }
            break;

        case BUTTON_EVENT_STOP:
            if (Status_Stop())
            {
                Conveyor_Stop();
                Servo_Reset();
                Sensor_Stop();
                Indicator_SetRun(0U);
            }
            break;

        case BUTTON_EVENT_MODE:
            Status_ToggleMode();             /* chỉ đổi được ở STATE_STOP */
            break;

        case BUTTON_EVENT_RESET:
            if (Status_GetState() == STATE_STOP)
            {
                Data_Reset();
            }
            else if (Status_ClearError())    /* STATE_ERROR -> STATE_STOP */
            {
                Indicator_SetFault(0U);
            }
            break;                           /* STATE_RUNNING: không làm gì */

        case BUTTON_EVENT_CONVEYOR:
            if ((Status_GetState() == STATE_RUNNING) &&
                (Status_GetMode()  == MODE_MANUAL))
            {
                Conveyor_Toggle();
            }
            break;

        case BUTTON_EVENT_SERVO:
            if ((Status_GetState() == STATE_RUNNING) &&
                (Status_GetMode()  == MODE_MANUAL))
            {
                Servo_Toggle();
            }
            break;

        default:
            break;
    }

    /* ===== 2. Cảm biến và bộ đếm ===== */
    SensorEvent_t ev = Sensor_Update();
    uint8_t classified = 0U;

    if (ev == SENSOR_EVENT_PRODUCT_A)
    {
        Data_IncrementA();
        classified = 1U;
        if (Status_GetMode() == MODE_AUTO) { Servo_StartSort(); }
    }
    else if (ev == SENSOR_EVENT_PRODUCT_B)
    {
        Data_IncrementB();
        classified = 1U;
    }

    /* ===== 3. Phát hiện lỗi ===== */
    SensorState_t s;
    Sensor_GetState(&s);

    if (Status_Update(Conveyor_IsRunning(),
                      s.cb_thap_active, s.cb_cao_active, classified))
    {
        Conveyor_Stop();
        Servo_Reset();                       /* hủy lượt gạt vừa bắt đầu */
        Sensor_Stop();                       /* hủy lượt đang chờ T1 */
        Indicator_SetRun(0U);
        Indicator_SetFault(1U);              /* LED lỗi và còi */
    }

    /* ===== 4. Việc nền ===== */
    SystemState_t state  = Status_GetState();
    SystemMode_t  mode   = Status_GetMode();
    ErrorCode_t   error  = Status_GetError();
    uint32_t      total  = Data_GetCountTotal();
    uint32_t      countA = Data_GetCountA();
    uint32_t      countB = Data_GetCountB();

    Servo_Update();
    Data_Update((state == STATE_RUNNING) ? 1U : 0U);
    LCD_Update(state, mode, error, total, countA, countB);
    IoT_Update(state, mode, error, total, countA, countB);
}
/* USER CODE END 3 */
```

## 6. Quy tắc điều phối

### 6.1. Bảng hành động khi đổi trạng thái

| Sự kiện | Điều kiện | Hành động của `main` |
|---|---|---|
| `BTN_START` | `Status_Start()` trả `1U` | `Servo_Reset()`, `Sensor_Start()`, `Indicator_SetRun(1U)`; `MODE_AUTO`: `Conveyor_Start()`; `MODE_MANUAL`: `Conveyor_Stop()` |
| `BTN_STOP` | `Status_Stop()` trả `1U` | `Conveyor_Stop()`, `Servo_Reset()`, `Sensor_Stop()`, `Indicator_SetRun(0U)` |
| Vào `STATE_ERROR` | `Status_Update()` trả `1U` | `Conveyor_Stop()`, `Servo_Reset()`, `Sensor_Stop()`, `Indicator_SetRun(0U)`, `Indicator_SetFault(1U)` |
| `BTN_RESET` ở `STATE_ERROR` | `Status_ClearError()` trả `1U` | `Indicator_SetFault(0U)` |
| `BTN_RESET` ở `STATE_STOP` | `Status_GetState() == STATE_STOP` | `Data_Reset()` |
| `BTN_MODE` | `Status_ToggleMode()` | Không có hành động đi kèm (chỉ đổi được ở `STATE_STOP`) |

### 6.2. Hiệu lực của nút nhấn (đối chiếu mục 7.3 của `system_requirements.md`)

| Trạng thái | Nút có hiệu lực | Cách `main` đảm bảo |
|---|---|---|
| `STATE_STOP` | `START`, `MODE`, `RESET` | `Status_Stop()` trả `0U`; `CONVEYOR`, `SERVO` bị chặn bởi điều kiện `STATE_RUNNING` |
| `STATE_RUNNING`, AUTO | `STOP` | `Status_Start()`, `ToggleMode()`, `ClearError()` trả `0U`; `RESET` không làm gì; `CONVEYOR`, `SERVO` bị chặn bởi điều kiện `MODE_MANUAL` |
| `STATE_RUNNING`, MANUAL | `STOP`, `CONVEYOR`, `SERVO` | Điều kiện kiểm tra trong `main` |
| `STATE_ERROR` | `RESET` | Các hàm `Status_*` khác trả `0U`; `CONVEYOR`, `SERVO` bị chặn |

### 6.3. Quy ước thứ tự trong vòng lặp

- Nút nhấn chạy **trước** cảm biến. Vòng lặp vừa nhấn START thì `Sensor_Start()` đã gọi trước `Sensor_Update()`, và băng tải ở `MODE_AUTO` đã chạy trước `Status_Update()`, nên timeout tính từ lúc băng tải chạy (đúng `status.md`).
- Vòng lặp vừa nhấn STOP thì `Sensor_Stop()` đã chạy trước `Sensor_Update()`, không có sự kiện nào lọt qua.
- Phát hiện lỗi chạy **sau** cảm biến, nên cờ `classified` của vòng lặp đó được `Status_Update()` dùng để đặt lại timeout trước khi kiểm tra.
- `state`, `mode`, `error`, bộ đếm được lấy ở mục 4, tức **sau** khi nút nhấn và phát hiện lỗi đã đổi trạng thái, nên LCD và IoT luôn nhận giá trị mới nhất của vòng lặp.

### 6.4. Xử lý trùng thời điểm giữa phân loại và lỗi

- Lỗi xảy ra khi `sensor` còn chờ T1: `Sensor_Stop()` hủy lượt nhận diện, sản phẩm không được đếm, không gạt.
- Sản phẩm đã nhận diện xong, cùng vòng lặp đó xảy ra lỗi: sản phẩm **vẫn được đếm** (đã nhận diện thành công trước lỗi), nhưng `Servo_Reset()` hủy lượt gạt vừa bắt đầu.
- `classified = 1U` chỉ đúng trong một vòng lặp, `main` khai báo lại biến này ở đầu mục 2 mỗi vòng (mặc định `0U`).

### 6.5. Điều kiện đếm

`main` không kiểm tra `STATE_RUNNING` trước khi tăng bộ đếm, vì `sensor` chỉ phát sự kiện sau `Sensor_Start()`, tức chỉ ở `STATE_RUNNING`. Nếu cần chắc chắn hơn, thêm điều kiện `Status_GetState() == STATE_RUNNING` bao quanh mục 2 (một dòng).

## 7. Dependencies

| Thành phần | Quan hệ | Mô tả |
|---|---|---|
| `timer` | Gọi API | `Timer_Init()`, và `HAL_TIM_Base_Start_IT(&htim1)` do `main` gọi |
| `buttons` | Gọi API | `Buttons_Init()`, `Buttons_Update()` |
| `sensor` | Gọi API | `Sensor_Init/Start/Stop/Update/GetState()` |
| `status` | Gọi API | `Status_Init/Start/Stop/ToggleMode/ClearError/Update()` và các hàm `Get` |
| `data` | Gọi API | `Data_Init/IncrementA/IncrementB/Update/Reset()` và các hàm `Get` |
| `conveyor` | Gọi API | `Conveyor_Init/Start/Stop/Toggle/IsRunning()` |
| `servo` | Gọi API | `Servo_Init/Update/StartSort/Toggle/Reset()` |
| `indicator` | Gọi API | `Indicator_Init/SetRun/SetFault()` |
| `LCD`, `IoT` | Gọi API | `LCD_Init/Update()`, `IoT_Init/Update()` |
| CubeMX (`tim.h`, `usart.h`, `gpio.h`) | Phụ thuộc | Khởi tạo ngoại vi, `htim1` để start ngắt TIM1 |

Giữa các module không gọi lẫn nhau, mọi liên kết đều đi qua `main`.

## 8. Testing

### Test 1: Cấp điện

- Cấp điện, không nhấn nút.
- Kiểm tra `STATE_STOP`, `MODE_AUTO`, LCD hiện màn 1, băng tải đứng, servo ở `Non-gate`, đèn và còi tắt.

### Test 2: Giữ nút lúc cấp điện

- Giữ một nút khi cấp điện.
- Kiểm tra không có sự kiện nào cho đến khi nhả hết nút.

### Test 3: START ở AUTO

- Nhấn START ở `MODE_AUTO`.
- Kiểm tra `STATE_RUNNING`, `LED_RUN` sáng, băng tải chạy ngay, LCD hiện bố cục `RUNNING`.

### Test 4: START ở MANUAL

- Đổi sang `MODE_MANUAL` ở `STATE_STOP`, nhấn START.
- Kiểm tra `STATE_RUNNING`, `LED_RUN` sáng, băng tải dừng, servo ở `Non-gate`.

### Test 5: Phân loại A ở AUTO

- Cho sản phẩm A đi qua.
- Kiểm tra `Count_A` và tổng tăng ngay, servo chờ `T_delay`, sang `Gate`, giữ `T_hold`, về `Non-gate`.

### Test 6: Phân loại B

- Cho sản phẩm B đi qua.
- Kiểm tra `Count_B` và tổng tăng, servo không chuyển động.

### Test 7: MANUAL

- Ở `MODE_MANUAL`, `STATE_RUNNING`: nhấn `CONVEYOR`, `SERVO`.
- Kiểm tra băng tải bật/tắt, servo đổi vị trí và giữ nguyên.
- Cho sản phẩm A đi qua: bộ đếm tăng, servo không tự gạt.
- Ở `MODE_AUTO`: nhấn `CONVEYOR`, `SERVO` không có tác dụng.

### Test 8: STOP

- Đang chạy, nhấn STOP.
- Kiểm tra `STATE_STOP`, băng tải dừng, servo về `Non-gate`, `LED_RUN` tắt, lượt gạt đang chờ bị hủy, LCD hiện màn 1.
- Nhấn START lại, kiểm tra không có lượt gạt cũ nào tiếp tục.

### Test 9: Đổi chế độ

- Nhấn MODE ở `STATE_STOP`: chế độ đổi.
- Nhấn MODE ở `STATE_RUNNING` và `STATE_ERROR`: không đổi.

### Test 10: Lỗi và RESET

- Gây `ERROR_TIMEOUT`, rồi `ERROR_CONTINUOUSLY_ACTIVE`.
- Kiểm tra băng tải dừng, servo về `Non-gate`, `LED_RUN` tắt, `LED_FAULT` và còi bật, LCD hiện `STATE: ERROR` kèm mã lỗi, mọi nút trừ RESET không có tác dụng.
- Nhấn RESET: về `STATE_STOP`, đèn lỗi và còi tắt, bộ đếm giữ nguyên.

### Test 11: Lỗi khi đang chờ T1

- Tạo lỗi đúng lúc `sensor` đang chờ T1.
- Kiểm tra lượt nhận diện bị hủy, bộ đếm không tăng.

### Test 12: RESET bộ đếm

- Ở `STATE_STOP`, nhấn RESET: bộ đếm về 0 trên LCD, tắt nguồn bật lại vẫn bằng 0.
- Ở `STATE_RUNNING`, nhấn RESET: không có tác dụng.

### Test 13: Lưu và khôi phục bộ đếm

- Đếm vài sản phẩm, chờ `Data` lưu ROM, tắt rồi cấp điện lại.
- Kiểm tra bộ đếm khôi phục bằng giá trị đã lưu gần nhất.

### Test 14: Vòng lặp không bị chặn

- Trong lúc servo gạt và `IoT` gửi, kiểm tra nút nhấn, cảm biến, LCD vẫn phản hồi bình thường.

## Các thông số và quyết định chưa chốt

| Nội dung | Trạng thái |
|---|---|
| Khai báo biến trong thân `while (1)` | Dùng C99. Nếu Keil báo lỗi thì chuyển khai báo lên trước `while (1)` |
| Điều kiện `STATE_RUNNING` khi đếm | Không kiểm tra thêm (sensor đã đảm bảo) |
| Khối lệnh STOP và ERROR giống nhau | Chấp nhận lặp lại để dễ đọc, không tách hàm phụ |
| Watchdog (IWDG) | Không dùng ở bản hiện tại |
| `HAL_TIM_Base_Start_IT(&htim1)` | Do `main` gọi, không do module `timer` gọi (xem `timer.md`) |
| Tối ưu hóa và MicroLIB của Keil | Chưa xác định, ảnh hưởng hằng số trễ của `LCD` |
