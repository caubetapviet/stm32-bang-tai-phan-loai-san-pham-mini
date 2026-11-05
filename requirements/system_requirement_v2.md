# YÊU CẦU HỆ THỐNG (BẢN ĐƠN GIẢN HÓA) – MÔ HÌNH BĂNG TẢI PHÂN LOẠI SẢN PHẨM

## 1. Tổng quan dự án

### 1.1. Tên dự án

Mô hình băng tải phân loại sản phẩm theo chiều cao sử dụng vi điều khiển STM32F103C8T6.

### 1.2. Mục tiêu

Xây dựng mô hình băng tải tự động phân loại sản phẩm thành hai loại A và B dựa trên chiều cao, dùng hai cảm biến để nhận biết sản phẩm và servo để gạt sản phẩm loại A.

Các chức năng chính:

- Điều khiển băng tải.
- Nhận biết và phân loại sản phẩm A/B.
- Điều khiển servo gạt sản phẩm A.
- Đếm và hiển thị số lượng sản phẩm đã phân loại.
- Hai chế độ hoạt động AUTO và MANUAL.
- Phát hiện lỗi và đưa hệ thống về trạng thái an toàn.
- Định hướng giám sát IoT (để giai đoạn sau).

### 1.3. Nguyên tắc đơn giản hóa

Bản này ưu tiên phương án dễ thực hiện và dễ kiểm thử. Chỉ giữ các chức năng cần thiết để hệ thống chạy đúng, các tính năng mở rộng để giai đoạn sau.

### 1.4. Nền tảng phát triển

- Vi điều khiển: STM32F103C8T6 (clock 8 MHz, HSI).
- Cấu hình ngoại vi: STM32CubeMX.
- Thư viện: STM32 HAL.
- Lập trình: Keil C (MDK-ARM).
- Mô phỏng: Proteus 8.17.

---

## 2. Thuật ngữ và quy ước

### 2.1. Trạng thái hệ thống

| Tên | Ý nghĩa |
|---|---|
| `STATE_STOP` | Hệ thống đang dừng |
| `STATE_RUNNING` | Hệ thống đang vận hành |
| `STATE_ERROR` | Hệ thống đang gặp lỗi |

Các tên trạng thái mô tả trạng thái hiện tại, không phải tên nút nhấn. Nhấn `BTN_START` là hành động yêu cầu khởi động, còn `STATE_RUNNING` là trạng thái sau khi khởi động thành công.

### 2.2. Chế độ hoạt động

| Tên | Ý nghĩa |
|---|---|
| `MODE_AUTO` | Tự động |
| `MODE_MANUAL` | Điều khiển bằng tay |

### 2.3. Nút nhấn

| Tên nút | Chức năng |
|---|---|
| `BTN_START` | Khởi động hệ thống |
| `BTN_STOP` | Dừng hệ thống |
| `BTN_MODE` | Chuyển AUTO/MANUAL |
| `BTN_RESET` | Xóa bộ đếm (ở `STATE_STOP`) hoặc xóa lỗi (ở `STATE_ERROR`) |
| `BTN_CONVEYOR` | Bật/tắt băng tải ở `MODE_MANUAL` |
| `BTN_SERVO` | Chuyển vị trí servo ở `MODE_MANUAL` |

### 2.4. Cảm biến

- `CB_Thap` (PB10): phát hiện tất cả sản phẩm.
- `CB_Cao` (PB11): chỉ phát hiện sản phẩm loại A.
- Mức `LOW`: đang phát hiện sản phẩm. Mức `HIGH`: không phát hiện.

### 2.5. Vị trí servo

- `Non-gate`: sản phẩm đi thẳng.
- `Gate`: cánh gạt quét khoảng 45° để đưa sản phẩm A về đúng vị trí.

---

## 3. Yêu cầu công nghệ

### 3.1. Nguyên lý phân loại

Hai cảm biến đặt theo phương thẳng đứng để xác định chiều cao sản phẩm.

1. Khi một trong hai cảm biến phát hiện sản phẩm (mức `LOW`) và hệ thống đang sẵn sàng, bắt đầu tính thời gian T1.
2. Hết T1, kiểm tra cả hai cảm biến:

| CB_Thap | CB_Cao | Kết quả |
|---|---|---|
| LOW | LOW | Sản phẩm A |
| LOW | HIGH | Sản phẩm B |
| HIGH | LOW | Không nhận diện, không đếm |
| HIGH | HIGH | Không ghi nhận |

3. Khi nhận diện A hoặc B, tăng bộ đếm tương ứng ngay, không phụ thuộc thao tác của servo.
4. Ở `MODE_AUTO`, sản phẩm A kích hoạt servo gạt. Sản phẩm B đi thẳng.

**Chống đếm lặp:** sau mỗi lần kiểm tra hết T1 (dù nhận diện được hay không), hệ thống chỉ nhận sản phẩm mới khi cả hai cảm biến đều ở mức `HIGH`. Nếu một cảm biến không về `HIGH`, tình huống này được phát hiện bằng lỗi `ERROR_CONTINUOUSLY_ACTIVE`.

**Điều kiện hoạt động:**

- Chỉ phân loại và đếm khi hệ thống ở `STATE_RUNNING`. Ở `STATE_STOP` và `STATE_ERROR`, tín hiệu cảm biến bị bỏ qua.
- Khi chuyển sang `STATE_RUNNING`, hệ thống sẵn sàng ngay, kể cả khi cảm biến đang `LOW` (sản phẩm đang nằm tại cảm biến được coi là sản phẩm mới).
- Khi rời `STATE_RUNNING` (STOP hoặc ERROR), lượt nhận diện đang chờ T1 bị hủy, sản phẩm đó không được đếm.

**Giả định cách đặt sản phẩm:** sản phẩm đặt cách nhau đủ xa trên băng tải, hệ thống không phân loại các sản phẩm đặt sát nhau.

**Ràng buộc T1:** T1 phải nhỏ hơn thời gian sản phẩm che cảm biến, nếu không sản phẩm đi qua sẽ không được nhận diện.

### 3.2. Điều khiển băng tải và servo

- Băng tải dùng động cơ DC qua mạch L298N. Hướng quay do hai chân IN1, IN2, tốc độ do PWM (TIM4).
- Tốc độ băng tải cố định (tạm 70%), chỉnh khi thử nghiệm.
- Servo SG90 điều khiển bằng PWM 50 Hz (TIM2). Non-gate dùng xung 0,6 ms, Gate dùng xung 1,1 ms (tạm, chỉnh khi thử thực tế). Không dùng xung 0 ms vì servo mất tín hiệu và thả lỏng.
- Khi chuyển sang `STATE_STOP` hoặc `STATE_ERROR`: băng tải dừng và servo về `Non-gate`.

### 3.3. Chế độ MODE_AUTO

Khi ở `MODE_AUTO` và `STATE_RUNNING`:

- Băng tải chạy ngay khi chuyển sang `STATE_RUNNING`.
- Nhận diện được sản phẩm A thì servo chuyển ngay sang `Gate`, giữ trong thời gian `T_hold`, rồi về `Non-gate`. Không có thời gian trễ `T_delay`.
- Sản phẩm B không có thao tác nào của servo.
- Nếu servo đang bận (đang giữ `Gate`) khi có sản phẩm A mới, lệnh gạt bị bỏ qua nhưng sản phẩm vẫn được đếm.

### 3.4. Chế độ MODE_MANUAL

Khi ở `MODE_MANUAL` và `STATE_RUNNING`:

- Vừa chuyển sang `STATE_RUNNING`: băng tải dừng, servo ở `Non-gate`.
- `BTN_CONVEYOR` bật/tắt băng tải.
- `BTN_SERVO` đảo vị trí servo giữa `Non-gate` và `Gate`, giữ nguyên đến lần nhấn tiếp theo.
- Việc phát hiện và đếm sản phẩm A/B vẫn được duy trì.
- Servo không tự gạt theo kết quả cảm biến.
- Việc giữ vị trí servo chỉ có hiệu lực trong `STATE_RUNNING`. Sang `STATE_STOP` hoặc `STATE_ERROR`, servo luôn về `Non-gate`.

`BTN_CONVEYOR` và `BTN_SERVO` chỉ có hiệu lực khi hệ thống ở `MODE_MANUAL` và `STATE_RUNNING`.

### 3.5. Chuyển đổi trạng thái

| Từ | Sang | Điều kiện |
|---|---|---|
| `STATE_STOP` | `STATE_RUNNING` | Nhấn `BTN_START` |
| `STATE_RUNNING` | `STATE_STOP` | Nhấn `BTN_STOP` |
| `STATE_RUNNING` | `STATE_ERROR` | Phát hiện một trong hai lỗi (mục 4) |
| `STATE_ERROR` | `STATE_STOP` | Nhấn `BTN_RESET` |

Không cho phép chuyển trực tiếp từ `STATE_ERROR` sang `STATE_RUNNING`. Khi chuyển sang `STATE_STOP`, các hoạt động của chu kỳ vận hành hiện tại kết thúc, lượt gạt đang chờ bị hủy.

### 3.6. Chuyển đổi chế độ

- `BTN_MODE` chỉ có hiệu lực ở `STATE_STOP`.
- Khi cấp điện, hệ thống luôn vào `STATE_STOP` và `MODE_AUTO`.

---

## 4. Phát hiện và xử lý lỗi

### 4.1. Các loại lỗi

Chỉ phát hiện lỗi khi hệ thống ở `STATE_RUNNING` **và băng tải đang chạy**, ở cả hai chế độ. Băng tải dừng thì các bộ đếm thời gian của lỗi về 0 và tính lại từ 0 khi băng tải chạy lại.

**ERROR_CONTINUOUSLY_ACTIVE**

- Một trong hai cảm biến ở mức `LOW` liên tục vượt quá thời gian T2.
- Ràng buộc: T2 > T1 và T2 > thời gian một sản phẩm đi qua cảm biến.

**ERROR_TIMEOUT**

- Không có lượt phân loại thành công nào trong thời gian timeout (dự kiến 30 giây).
- Thời gian bắt đầu tính khi băng tải chạy. Sau mỗi lượt phân loại thành công, tính lại.

Nếu hai lỗi cùng thỏa trong một lần kiểm tra, ưu tiên `ERROR_CONTINUOUSLY_ACTIVE` vì chỉ ra sự cố cụ thể hơn.

### 4.2. Quy trình xử lý lỗi

Khi phát hiện lỗi:

1. Chuyển sang `STATE_ERROR`.
2. Dừng băng tải.
3. Đưa servo về `Non-gate`, hủy lượt gạt đang chờ.
4. Ngừng nhận diện sản phẩm, hủy lượt đang chờ T1 (không đếm).
5. Bật LED báo lỗi và còi (kêu liên tục), tắt LED chạy.
6. Khóa các nút điều khiển, trừ `BTN_RESET`.
7. LCD hiển thị `STATE: ERROR` và mã lỗi.

Nếu sản phẩm đã nhận diện xong ngay trước lỗi trong cùng một vòng lặp, sản phẩm đó vẫn được đếm, nhưng lượt gạt bị hủy.

### 4.3. Xử lý sau lỗi

Nhấn `BTN_RESET` ở `STATE_ERROR`: xóa lỗi, tắt LED lỗi và còi, về `STATE_STOP`. **Bộ đếm được giữ nguyên.** Sau đó nhấn `BTN_START` để chạy lại.

---

## 5. Quản lý dữ liệu

### 5.1. Bộ đếm sản phẩm

- Hai bộ đếm lưu trong RAM: `Count_A`, `Count_B`. Tổng `Count_Total = Count_A + Count_B`.
- Cập nhật ngay khi nhận diện được A hoặc B, không phụ thuộc servo.
- Khi xảy ra lỗi, lượt đang xử lý bị hủy, các lượt đã đếm trước đó được giữ nguyên.
- **Không lưu vào ROM.** Mất nguồn thì mất dữ liệu, bộ đếm bắt đầu từ 0 khi cấp điện (chấp nhận giới hạn này).

### 5.2. Chức năng của BTN_RESET

| Trạng thái | Tác dụng |
|---|---|
| `STATE_STOP` | Xóa bộ đếm về 0 |
| `STATE_ERROR` | Xóa lỗi, về `STATE_STOP`, giữ nguyên bộ đếm |
| `STATE_RUNNING` | Không có tác dụng |

Nhấn nhanh hay nhấn giữ đều như nhau.

---

## 6. Hiển thị và cảnh báo

### 6.1. LCD 16x2 (chế độ 4-bit)

LCD luôn bật. **`STATE_STOP` và `STATE_RUNNING` dùng chung một màn hình**, gồm bốn thông tin:

- Góc trên trái: tổng số sản phẩm (`T`).
- Góc trên phải: số sản phẩm B (`B`).
- Góc dưới trái: số sản phẩm A (`A`).
- Góc dưới phải: chế độ `AUTO` hoặc `MANUAL`.

Ví dụ:

```
T:120       B:50
A:70        AUTO
```

Ở `STATE_ERROR`:

```
STATE: ERROR
ERR: TIMEOUT
```

Mã lỗi hiển thị: `TIMEOUT` hoặc `CONT_ACTIVE`. Mỗi số hiển thị tối đa 5 chữ số, lớn hơn hiện `99999`.

Vì `STATE_STOP` và `STATE_RUNNING` có màn hình giống nhau, người vận hành phân biệt hai trạng thái này bằng `LED_RUN` (sáng khi chạy).

### 6.2. LED và còi

| Thiết bị | Chân | Hoạt động |
|---|---|---|
| `LED_RUN` | PA7 | Sáng liên tục suốt `STATE_RUNNING` (cả AUTO và MANUAL), tắt ở `STATE_STOP` và `STATE_ERROR` |
| `LED_FAULT` | PB0 | Sáng liên tục suốt `STATE_ERROR` |
| `BUZZER` | PB1 | Kêu liên tục suốt `STATE_ERROR` |

Mức bật là HIGH. `LED_FAULT` và `BUZZER` luôn bật/tắt cùng nhau.

### 6.3. Giám sát IoT (giai đoạn sau)

Chưa triển khai ở bản này. Định hướng: module ESP8266 ESP-01 chạy firmware riêng, STM32 gửi một dòng văn bản định kỳ 3 giây qua USART1 (tổng, A, B, chế độ, trạng thái, mã lỗi). IoT chỉ phục vụ giám sát, STM32 vẫn đảm nhiệm điều khiển. Hiện USART1 chỉ dùng để kiểm thử qua Virtual Terminal trong Proteus.

---

## 7. Nút nhấn

### 7.1. Đặc tính

- Tín hiệu active-low, GPIO Input Pull-up.
- Chống dội phím 20 ms.
- Tại một thời điểm chỉ nhận một nút, phải nhả hết các nút mới nhận nút khác.
- Nếu đang giữ nút lúc cấp điện, các nút bị khóa đến khi nhả hết.
- Không phân biệt nhấn nhanh hay nhấn giữ.

### 7.2. Hiệu lực của nút

| Trạng thái | Nút có hiệu lực |
|---|---|
| `STATE_STOP` | `BTN_START`, `BTN_MODE`, `BTN_RESET` |
| `STATE_RUNNING`, AUTO | `BTN_STOP` |
| `STATE_RUNNING`, MANUAL | `BTN_STOP`, `BTN_CONVEYOR`, `BTN_SERVO` |
| `STATE_ERROR` | `BTN_RESET` |

---

## 8. Yêu cầu an toàn và vận hành

- Khi phát hiện lỗi, hệ thống tự chuyển sang `STATE_ERROR`.
- Khi `STATE_STOP` hoặc `STATE_ERROR`: băng tải dừng, servo ở `Non-gate`.
- Chỉ phân loại và đếm ở `STATE_RUNNING`.
- Không đổi chế độ khi `STATE_RUNNING` hoặc `STATE_ERROR`.
- Không dùng `BTN_CONVEYOR`, `BTN_SERVO` ở `MODE_AUTO`.
- `BTN_RESET` không có tác dụng ở `STATE_RUNNING`.
- Từ `STATE_ERROR` chỉ về được `STATE_STOP`, rồi nhấn `BTN_START` để chạy lại.

---

## 9. Các thông số chưa chốt

Các thông số sau chỉnh trong quá trình thử nghiệm:

| Thông số | Giá trị tạm |
|---|---|
| T1 (nhận diện sản phẩm) | 50 ms |
| T2 (cảm biến kích hoạt liên tục) | 2000 ms |
| Timeout không phân loại | 30000 ms |
| `T_hold` (servo giữ ở `Gate`) | 500 ms |
| Xung servo `Non-gate` / `Gate` | 0,6 ms / 1,1 ms |
| Tốc độ băng tải | 70% |
| Tần số PWM băng tải (TIM4) | 20 kHz (Prescaler = 1, Period = 199) |
| Dòng tiêu thụ thực tế của còi | Cần đo, đang nối trực tiếp PB1 |

Ràng buộc giữa các thông số:

- T1 nhỏ hơn thời gian sản phẩm che cảm biến.
- T2 > T1 và T2 > thời gian một sản phẩm đi qua cảm biến.
- `T_hold` nhỏ hơn khoảng thời gian tối thiểu giữa hai sản phẩm liên tiếp (áp dụng cho mọi cặp A-A, A-B, B-A), để cánh gạt không quệt vào sản phẩm khác.

---

## 10. Phạm vi thực hiện

1. Viết và kiểm thử từng module firmware (đã hoạt động: `timer`, `buttons`, `conveyor`, `servo`, `indicator`; cần kiểm thử thêm: `sensor`, `status`, `data`, `lcd`).
2. Tích hợp thành hệ thống hoàn chỉnh trong `main`, mô phỏng và kiểm thử trên Proteus.
3. Hiệu chỉnh thông số vận hành trên mô hình thật.
4. Cập nhật tài liệu thiết kế module theo bản code đã chạy ổn định.
5. Triển khai IoT (giai đoạn sau).

---

## Phụ lục: Thay đổi so với bản yêu cầu trước

| Nội dung | Bản trước | Bản đơn giản hóa |
|---|---|---|
| Lưu bộ đếm | RAM và Flash (lưu theo ngưỡng, chu kỳ) | Chỉ RAM, mất khi tắt nguồn |
| Module `sensor` | State machine 4 trạng thái, có `Start`/`Stop` | Hai cờ nội bộ, tham số `enable` |
| Servo | `T_delay` rồi gạt, giữ `T_hold` | Gạt ngay, giữ `T_hold` |
| Tốc độ băng tải | Hàm `Conveyor_SetSpeed()` | Hằng số cố định |
| LCD ở `STATE_STOP` | Luân phiên hai màn hình mỗi 2 giây | Cùng màn hình với `STATE_RUNNING` |
| Điều kiện phát hiện lỗi | `STATE_RUNNING`, băng tải chạy | Không đổi |
| IoT | Gửi định kỳ 3 giây (đã thiết kế) | Để giai đoạn sau |
