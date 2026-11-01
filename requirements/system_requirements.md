# YÊU CẦU HỆ THỐNG – MÔ HÌNH BĂNG TẢI PHÂN LOẠI SẢN PHẨM

## 1. Tổng quan dự án

### 1.1. Tên dự án

Mô hình băng tải phân loại sản phẩm theo chiều cao sử dụng vi điều khiển STM32F103C8T6.

### 1.2. Mục tiêu

Xây dựng mô hình băng tải có khả năng tự động phân loại sản phẩm thành hai loại A và B dựa trên chiều cao, sử dụng cảm biến để nhận biết sản phẩm và servo để thực hiện thao tác phân loại.

Hệ thống có các chức năng chính:

- Điều khiển băng tải.
- Nhận biết và phân loại sản phẩm A/B.
- Điều khiển servo để đưa sản phẩm về đúng vị trí.
- Đếm và hiển thị số lượng sản phẩm đã phân loại.
- Hỗ trợ hai chế độ hoạt động AUTO và MANUAL.
- Phát hiện lỗi và đưa hệ thống về trạng thái an toàn.
- Lưu bản sao dữ liệu đếm vào bộ nhớ không mất dữ liệu (ROM) để khôi phục khi cấp điện lại.
- Định hướng tích hợp IoT để giám sát hệ thống.

### 1.3. Nền tảng phát triển

- Vi điều khiển: STM32F103C8T6.
- Môi trường cấu hình ngoại vi: STM32CubeMX.
- Thư viện lập trình: STM32 HAL.
- Môi trường lập trình: Keil C.
- Phần mềm mô phỏng: Proteus 8.17.

---

## 2. Thuật ngữ và quy ước

### 2.1. Trạng thái hệ thống

Hệ thống có ba trạng thái:

| Tên trạng thái  | Ý nghĩa                 |
|-----------------|-------------------------|
| `STATE_STOP`    | Hệ thống đang dừng.     |
| `STATE_RUNNING` | Hệ thống đang vận hành. |
| `STATE_ERROR`   | Hệ thống đang gặp lỗi.  |

Các tên trạng thái trên dùng để mô tả trạng thái hiện tại của hệ thống, không phải tên nút nhấn.

### 2.2. Chế độ hoạt động

Hệ thống có hai chế độ:

| Tên chế độ    | Ý nghĩa                     |
|---------------|-----------------------------|
| `MODE_AUTO`   | Chế độ tự động.             |
| `MODE_MANUAL` | Chế độ điều khiển bằng tay. |

Chế độ hoạt động xác định cách hệ thống điều khiển băng tải và servo khi ở `STATE_RUNNING`.

### 2.3. Nút nhấn

Các nút nhấn vật lý được quy ước như sau:

| Tên nút        | Chức năng                                                                              |
|----------------|----------------------------------------------------------------------------------------|
| `BTN_START`    | Khởi động hệ thống.                                                                    |
| `BTN_STOP`     | Dừng hệ thống.                                                                         |
| `BTN_MODE`     | Chuyển đổi chế độ AUTO/MANUAL.                                                         |
| `BTN_RESET`    | Đặt lại bộ đếm (ở `STATE_STOP`) hoặc xóa lỗi (ở `STATE_ERROR`), tùy theo trạng thái hệ thống. |
| `BTN_CONVEYOR` | Điều khiển bật/tắt băng tải trong chế độ MANUAL.                                       |
| `BTN_SERVO`    | Điều khiển chuyển đổi vị trí servo trong chế độ MANUAL.                                |

Cần phân biệt:

- Nhấn `BTN_STOP` là một hành động của người vận hành.
- `STATE_STOP` là trạng thái hệ thống sau khi đã dừng.
- Nhấn `BTN_START` là hành động yêu cầu khởi động.
- `STATE_RUNNING` là trạng thái hệ thống sau khi khởi động thành công.

### 2.4. Quy ước tín hiệu cảm biến

- `CB_Thap`: cảm biến thấp, phát hiện tất cả sản phẩm.
- `CB_Cao`: cảm biến cao, chỉ phát hiện sản phẩm loại A.
- Mức `LOW`: cảm biến đang phát hiện sản phẩm.
- Mức `HIGH`: cảm biến không phát hiện sản phẩm.

### 2.5. Quy ước vị trí servo

- `Non-gate`: vị trí không phân loại, sản phẩm đi thẳng.
- `Gate`: vị trí phân loại, servo thực hiện thao tác đưa sản phẩm A về đúng vị trí.

---

## 3. Yêu cầu công nghệ

### 3.1. Nguyên lý phân loại

Hệ thống sử dụng hai cảm biến đặt theo phương thẳng đứng để xác định chiều cao sản phẩm:

- Cảm biến thấp (`CB_Thap`): phát hiện tất cả sản phẩm đi qua.
- Cảm biến cao (`CB_Cao`): chỉ phát hiện sản phẩm loại A.

Nguyên lý phân loại:

1. Khi hệ thống ở trạng thái sẵn sàng nhận sản phẩm mới (xem cơ chế tái kích hoạt bên dưới) và một trong hai cảm biến phát hiện sản phẩm, hệ thống bắt đầu tính thời gian T1.
2. Sau khi hết thời gian T1, hệ thống kiểm tra trạng thái của cả hai cảm biến:

   - `CB_Thap = LOW` và `CB_Cao = LOW`: xác định sản phẩm loại A.
   - `CB_Thap = LOW` và `CB_Cao = HIGH`: xác định sản phẩm loại B.
   - Chỉ `CB_Cao = LOW`: tiếp tục chờ tín hiệu từ `CB_Thap`.
   - Cả hai cảm biến đều ở mức `HIGH`: không ghi nhận sản phẩm.
3. Khi phân loại thành công, hệ thống phát sinh sự kiện tương ứng với loại sản phẩm.
4. Trong chế độ AUTO, servo thực hiện thao tác phân loại theo loại sản phẩm.
5. Bộ đếm được cập nhật ngay khi hệ thống xác định được loại sản phẩm A hoặc B, không phụ thuộc vào thao tác của servo.

Giá trị T1 chưa được chốt chính thức và sẽ được hiệu chỉnh trong quá trình thử nghiệm.

**Cơ chế tái kích hoạt:**

- Sau mỗi lượt phân loại thành công, hoặc lượt bị loại bỏ vì cả hai cảm biến ở mức `HIGH`, hệ thống chỉ nhận sản phẩm mới khi **cả hai cảm biến `CB_Thap` và `CB_Cao` đều ở mức `HIGH`**.
- Cơ chế này tránh trường hợp cùng một sản phẩm còn nằm tại vị trí cảm biến bị phân loại và đếm lặp lại sau mỗi khoảng T1.
- Khi hệ thống vừa chuyển sang `STATE_RUNNING`, hệ thống ở trạng thái sẵn sàng ngay, kể cả khi cảm biến đang ở mức `LOW`. Sản phẩm đang nằm tại vị trí cảm biến lúc này được coi là sản phẩm mới nếu giữ cảm biến ở mức `LOW` đủ thời gian T1.

**Giả định về cách đặt sản phẩm:**

- Hệ thống không được thiết kế để phân loại các sản phẩm đặt sát nhau. Sản phẩm cần được đặt cách xa nhau trên băng tải.
- Trường hợp hai sản phẩm đặt quá sát nhau làm cảm biến không trở về mức `HIGH` được phát hiện thông qua lỗi `ERROR_CONTINUOUSLY_ACTIVE` (xem mục 4.1).

### 3.2. Điều khiển băng tải và servo

- Băng tải sử dụng động cơ DC, được điều khiển thông qua mạch công suất.
- Tốc độ động cơ được điều chỉnh bằng tín hiệu PWM.
- Servo được điều khiển bằng tín hiệu PWM để thực hiện thao tác phân loại sản phẩm.
- Khi hệ thống chuyển sang `STATE_STOP` hoặc `STATE_ERROR`:

  - Băng tải phải dừng.
  - Servo phải trở về vị trí `Non-gate`.

Các thông số như tốc độ băng tải, góc servo và thời gian giữ servo sẽ được hiệu chỉnh trong quá trình thử nghiệm.

### 3.3. Chế độ MODE_AUTO

Khi hệ thống ở `MODE_AUTO` và `STATE_RUNNING`:

- Hệ thống tự động vận hành băng tải. Băng tải chạy ngay khi hệ thống chuyển sang `STATE_RUNNING`.
- Cảm biến nhận biết sản phẩm và xác định loại sản phẩm.
- Servo tự động thực hiện thao tác phân loại sản phẩm A.
- Sản phẩm B đi thẳng, không cần thao tác gạt của servo.
- Bộ đếm được cập nhật ngay khi xác định được loại sản phẩm.
- Hệ thống bắt đầu tính thời gian timeout ngay khi chuyển sang `STATE_RUNNING` ở chế độ AUTO.
- Sau mỗi lượt phân loại thành công, thời gian timeout được tính lại.

### 3.4. Chế độ MODE_MANUAL

Khi hệ thống ở `MODE_MANUAL` và `STATE_RUNNING`:

- Khi vừa chuyển sang `STATE_RUNNING`, băng tải dừng và servo ở vị trí `Non-gate`.
- Người vận hành có thể điều khiển băng tải và servo bằng các nút nhấn riêng.
- Nhấn `BTN_CONVEYOR` để chuyển đổi trạng thái bật/tắt băng tải.
- Nhấn `BTN_SERVO` để chuyển đổi vị trí servo giữa `Non-gate` và `Gate`.
- Servo giữ nguyên vị trí sau khi được điều khiển cho đến lần nhấn `BTN_SERVO` tiếp theo.
- Quá trình phát hiện và đếm sản phẩm A/B vẫn được duy trì.
- Không tự động thực hiện thao tác phân loại bằng servo theo kết quả cảm biến.
- Hai lỗi `ERROR_TIMEOUT` và `ERROR_CONTINUOUSLY_ACTIVE` vẫn được phát hiện ở chế độ này. Cả hai chỉ tính thời gian khi băng tải đang chạy (xem mục 4.1).

`BTN_CONVEYOR` và `BTN_SERVO` chỉ có hiệu lực khi hệ thống ở `MODE_MANUAL` và `STATE_RUNNING`.

Việc giữ nguyên vị trí servo chỉ có hiệu lực trong khi hệ thống đang `STATE_RUNNING`. Khi chuyển sang `STATE_STOP` hoặc `STATE_ERROR`, servo luôn trở về vị trí `Non-gate`.

### 3.5. Chuyển đổi trạng thái hệ thống

Hệ thống có ba trạng thái: `STATE_STOP`, `STATE_RUNNING` và `STATE_ERROR`.

**Từ STATE_STOP sang STATE_RUNNING:**

- Người vận hành nhấn `BTN_START`.
- Hệ thống bắt đầu hoạt động theo `MODE_AUTO` hoặc `MODE_MANUAL` đã chọn.
- Ở `MODE_AUTO`: băng tải chạy ngay và thời gian timeout bắt đầu được tính.
- Ở `MODE_MANUAL`: băng tải dừng và servo ở vị trí `Non-gate` cho đến khi người vận hành điều khiển.
- Hệ thống ở trạng thái sẵn sàng nhận sản phẩm ngay (xem cơ chế tái kích hoạt ở mục 3.1).

**Từ STATE_RUNNING sang STATE_STOP:**

- Người vận hành nhấn `BTN_STOP`.
- Hệ thống dừng băng tải.
- Servo trở về vị trí `Non-gate`.
- LCD tiếp tục bật và hiển thị `STATE_STOP` cùng chế độ hiện tại.
- Các hoạt động của chu kỳ vận hành hiện tại được kết thúc.
- Các tác vụ phân loại hoặc điều khiển servo đang chờ thực hiện không được tiếp tục sau khi khởi động lại.

**Từ STATE_RUNNING sang STATE_ERROR:**

- Hệ thống phát hiện một trong các lỗi đã quy định.
- Hệ thống tự động chuyển sang `STATE_ERROR` và thực hiện quy trình xử lý lỗi.

**Từ STATE_ERROR sang STATE_STOP:**

- Người vận hành nhấn `BTN_RESET`.
- Hệ thống xóa trạng thái lỗi và cảnh báo.
- Hệ thống trở về `STATE_STOP` và chờ lệnh khởi động mới.
- Bộ đếm của các lượt phân loại thành công trước đó được giữ nguyên.

Không cho phép chuyển trực tiếp từ `STATE_ERROR` sang `STATE_RUNNING`.

### 3.6. Chuyển đổi chế độ hoạt động

- Người vận hành nhấn `BTN_MODE` để chuyển đổi giữa `MODE_AUTO` và `MODE_MANUAL`.
- `BTN_MODE` chỉ có hiệu lực khi hệ thống ở `STATE_STOP`.
- Không cho phép thay đổi chế độ khi hệ thống đang `STATE_RUNNING` hoặc `STATE_ERROR`.

---

## 4. Phát hiện và xử lý lỗi

### 4.1. Các loại lỗi

Hiện tại hệ thống thống nhất hai loại lỗi liên quan đến cảm biến. Cả hai lỗi đều được phát hiện ở `MODE_AUTO` và `MODE_MANUAL`.

**ERROR_CONTINUOUSLY_ACTIVE**

- Xảy ra khi cảm biến duy trì trạng thái phát hiện sản phẩm (mức `LOW`) liên tục vượt quá thời gian cho phép T2.
- Hệ thống phải phát hiện lỗi và chuyển sang `STATE_ERROR`.
- Ở `MODE_AUTO`: thời gian T2 được tính bất cứ khi nào cảm biến ở mức `LOW` (băng tải luôn chạy trong `STATE_RUNNING`).
- Ở `MODE_MANUAL`: thời gian T2 chỉ được tính khi băng tải đang chạy. Bộ đếm thời gian về 0 khi băng tải dừng và được tính lại từ 0 khi băng tải chạy trở lại.
- Ràng buộc: T2 phải lớn hơn T1 và lớn hơn thời gian một sản phẩm đi qua cảm biến, để sản phẩm bình thường không gây lỗi.

**ERROR_TIMEOUT**

- Xảy ra khi hệ thống chờ sản phẩm hoặc chờ quá trình nhận biết sản phẩm vượt quá thời gian cho phép.
- Thời gian timeout dự kiến là 30 giây.
- Ở `MODE_AUTO`: thời gian bắt đầu được tính ngay khi hệ thống chuyển sang `STATE_RUNNING` (băng tải chạy ngay khi vận hành).
- Ở `MODE_MANUAL`: thời gian chỉ được tính khi băng tải đang chạy. Bộ đếm thời gian về 0 khi băng tải dừng và được tính lại từ 0 khi băng tải chạy trở lại.
- Ở cả hai chế độ, sau mỗi lượt phân loại thành công, thời gian timeout được tính lại.

### 4.2. Quy trình xử lý lỗi

Khi phát hiện `ERROR_CONTINUOUSLY_ACTIVE` hoặc `ERROR_TIMEOUT`:

1. Chuyển hệ thống sang `STATE_ERROR`.
2. Dừng băng tải.
3. Ngừng quá trình phân loại.
4. Đưa servo về vị trí `Non-gate`.
5. Bật đèn LED báo lỗi và còi cảnh báo.
6. Hủy lượt phân loại đang được xử lý (nếu có). Lượt này không làm thay đổi bộ đếm.
7. Khóa các nút điều khiển, ngoại trừ `BTN_RESET`.
8. LCD tiếp tục bật để hiển thị trạng thái `STATE_ERROR` và mã lỗi.

### 4.3. Xử lý sau lỗi

Khi hệ thống ở `STATE_ERROR`, người vận hành nhấn `BTN_RESET` để:

- Xóa trạng thái lỗi.
- Tắt đèn LED báo lỗi và còi cảnh báo.
- Đưa hệ thống về `STATE_STOP`.
- Giữ nguyên dữ liệu đếm của các lượt phân loại thành công trước đó. `BTN_RESET` ở `STATE_ERROR` không xóa bộ đếm.
- Đưa servo về vị trí `Non-gate`.
- LCD chuyển sang giao diện hiển thị `STATE_STOP` và chế độ hiện tại.

Sau khi RESET, người vận hành cần nhấn `BTN_START` để khởi động lại hệ thống.

---

## 5. Quản lý và lưu trữ dữ liệu

### 5.1. Bộ đếm sản phẩm

Hệ thống sử dụng hai bộ đếm lưu trong RAM:

- `Count_A`: tổng số sản phẩm loại A đã phân loại thành công.
- `Count_B`: tổng số sản phẩm loại B đã phân loại thành công.

Tổng số sản phẩm được tính theo công thức:

`Count_Total = Count_A + Count_B`

Giá trị của các bộ đếm trong RAM là giá trị được hiển thị lên LCD và gửi lên IoT.

Nguyên tắc cập nhật:

- Cập nhật bộ đếm ngay khi cảm biến xác định được loại sản phẩm A hoặc B, không phụ thuộc vào thao tác của servo.
- Không có giá trị đếm tạm cần xác nhận hoặc khôi phục.
- Không tính các sản phẩm chưa phân loại thành công.
- Khi xảy ra lỗi, lượt phân loại đang xử lý bị hủy và không làm thay đổi bộ đếm. Các lượt đã phân loại thành công trước đó được giữ nguyên.

### 5.2. Chức năng RESET bộ đếm

Chức năng của `BTN_RESET` phụ thuộc vào trạng thái hệ thống. Nhấn nhanh hay nhấn giữ đều có tác dụng như nhau.

**Khi hệ thống ở STATE_STOP:**

- `BTN_RESET` xóa bộ đếm trong RAM và dữ liệu đã lưu trong ROM về 0.

**Khi hệ thống ở STATE_ERROR:**

- `BTN_RESET` xóa trạng thái lỗi và đưa hệ thống về `STATE_STOP`.
- Không thay đổi dữ liệu đếm của các lượt phân loại thành công trước đó.

**Khi hệ thống ở STATE_RUNNING:**

- `BTN_RESET` không có tác dụng.

### 5.3. Lưu trữ lâu dài (ROM)

ROM (bộ nhớ không mất dữ liệu) dùng để lưu bản sao của các bộ đếm trong RAM.

Yêu cầu:

- Khi cấp điện, hệ thống nạp giá trị từ ROM vào các bộ đếm trong RAM làm giá trị ban đầu.
- ROM lưu lại giá trị của RAM khi mất điện (nếu thực hiện được) hoặc lưu theo chu kỳ.
- Nếu lưu theo chu kỳ, sau khi mất điện có thể mất các lượt đếm kể từ lần lưu gần nhất.
- `BTN_RESET` ở `STATE_STOP` phải xóa cả dữ liệu đã lưu trong ROM.

Phương án bộ nhớ và phương pháp lưu trữ chưa được quyết định. Sẽ được lựa chọn trong giai đoạn thiết kế phần cứng và phần mềm.

---

## 6. Hiển thị và giám sát

### 6.1. Nguyên tắc hiển thị LCD

LCD 16x2 luôn được bật trong cả ba trạng thái `STATE_STOP`, `STATE_RUNNING` và `STATE_ERROR`.

Nội dung hiển thị thay đổi tùy theo trạng thái hệ thống.

**Khi hệ thống ở STATE_STOP:**

- Dòng 1: hiển thị `STATE_STOP`.
- Dòng 2: hiển thị chế độ hiện tại `MODE_AUTO` hoặc `MODE_MANUAL`.

Ví dụ:

```
STATE: STOP
MODE: AUTO
```

**Khi hệ thống ở STATE_RUNNING:**

LCD hiển thị bốn thông tin (giá trị lấy từ các bộ đếm trong RAM):

- Góc trên bên trái: tổng số sản phẩm A và B.
- Góc dưới bên trái: số lượng sản phẩm A.
- Góc trên bên phải: số lượng sản phẩm B.
- Góc dưới bên phải: chế độ hoạt động AUTO hoặc MANUAL.

Ví dụ:

```
T:120       B:50
A:70        AUTO
```

**Khi hệ thống ở STATE_ERROR:**

- Dòng 1: hiển thị `STATE_ERROR`.
- Dòng 2: hiển thị mã lỗi tương ứng.

Ví dụ:

```
STATE: ERROR
ERR: TIMEOUT
```

LCD phải tiếp tục hiển thị trạng thái lỗi cho đến khi người vận hành nhấn `BTN_RESET` và hệ thống chuyển về `STATE_STOP`.

### 6.2. Cảnh báo lỗi

Khi hệ thống ở `STATE_ERROR`:

- Đèn LED báo lỗi sáng.
- Còi phát tín hiệu cảnh báo.
- LCD hiển thị trạng thái lỗi và mã lỗi tương ứng.

Hai mã lỗi cần nhận biết:

- `ERROR_CONTINUOUSLY_ACTIVE`.
- `ERROR_TIMEOUT`.

### 6.3. Giám sát IoT

Hệ thống có định hướng tích hợp IoT thông qua module ESP8266 ESP-01.

Các thông tin dự kiến gửi lên giao diện IoT:

- Tổng số sản phẩm (giá trị bộ đếm trong RAM).
- Số lượng sản phẩm A (giá trị bộ đếm trong RAM).
- Số lượng sản phẩm B (giá trị bộ đếm trong RAM).
- Chế độ `MODE_AUTO`/`MODE_MANUAL`.
- Trạng thái `STATE_STOP`/`STATE_RUNNING`/`STATE_ERROR`.
- Thông tin lỗi khi có sự cố.

IoT chủ yếu phục vụ giám sát. Việc điều khiển và quản lý bộ đếm vẫn do STM32 đảm nhiệm.

Giao tiếp với ESP8266 chưa được triển khai trong giai đoạn hiện tại. UART1 hiện được sử dụng để kiểm thử và theo dõi dữ liệu bằng Virtual Terminal trong Proteus.

---

## 7. Quy ước nút nhấn và tín hiệu điều khiển

### 7.1. Chức năng nút nhấn

- `BTN_START`: khởi động hệ thống từ `STATE_STOP`.
- `BTN_STOP`: yêu cầu hệ thống chuyển từ `STATE_RUNNING` sang `STATE_STOP`.
- `BTN_MODE`: chuyển đổi `MODE_AUTO`/`MODE_MANUAL` khi `STATE_STOP`.
- `BTN_RESET`: đặt lại bộ đếm (ở `STATE_STOP`) hoặc xóa lỗi (ở `STATE_ERROR`), tùy theo trạng thái.
- `BTN_CONVEYOR`: bật/tắt băng tải trong `MODE_MANUAL` và `STATE_RUNNING`.
- `BTN_SERVO`: chuyển đổi vị trí servo trong `MODE_MANUAL` và `STATE_RUNNING`.

### 7.2. Đặc tính nút nhấn

- Các nút nhấn sử dụng tín hiệu active-low.
- Các nút nhấn có cơ chế chống dội phím.
- `BTN_RESET` không phân biệt nhấn nhanh hay nhấn giữ.

### 7.3. Quy tắc hiệu lực

| Trạng thái                   | Nút nhấn có hiệu lực                    |
|------------------------------|-----------------------------------------|
| `STATE_STOP`                 | `BTN_START`, `BTN_MODE`, `BTN_RESET`    |
| `STATE_RUNNING`, MODE_AUTO   | `BTN_STOP`                              |
| `STATE_RUNNING`, MODE_MANUAL | `BTN_STOP`, `BTN_CONVEYOR`, `BTN_SERVO` |
| `STATE_ERROR`                | `BTN_RESET`                             |

---

## 8. Yêu cầu an toàn và vận hành

- Khi phát hiện lỗi, hệ thống phải tự động chuyển sang `STATE_ERROR`.
- Khi chuyển sang `STATE_STOP` hoặc `STATE_ERROR`, băng tải phải dừng.
- Servo phải trở về vị trí `Non-gate` khi hệ thống ở `STATE_STOP` hoặc `STATE_ERROR`.
- LCD luôn bật để hiển thị thông tin tương ứng với trạng thái hệ thống.
- Không tiếp tục phân loại khi hệ thống đang `STATE_ERROR`.
- Không cho phép thay đổi chế độ khi hệ thống đang `STATE_RUNNING` hoặc `STATE_ERROR`.
- Không cho phép sử dụng `BTN_CONVEYOR` và `BTN_SERVO` khi hệ thống ở `MODE_AUTO`.
- Khi nhấn `BTN_RESET` trong `STATE_ERROR`, hệ thống trở về `STATE_STOP` và chờ lệnh `BTN_START`.
- Dữ liệu của các lượt phân loại thành công trước khi xảy ra lỗi phải được bảo toàn.
- `BTN_RESET` không có tác dụng khi hệ thống ở `STATE_RUNNING`.

---

## 9. Các thông số chưa chốt

Các thông số sau chưa được xác định chính thức và sẽ được hiệu chỉnh trong quá trình thử nghiệm:

1. Thời gian T1 dùng để xác định loại sản phẩm.
2. Thời gian T2 dùng để phát hiện cảm biến duy trì trạng thái active liên tục.
3. Góc quay và thời gian giữ servo tại các vị trí phân loại.
4. Phương án phần cứng, phương pháp lưu trữ dữ liệu lâu dài và chu kỳ lưu vào ROM.
5. Các thông số vận hành thực tế của băng tải và cơ cấu phân loại.

Ràng buộc giữa các thông số: T2 > T1 và T2 > thời gian một sản phẩm đi qua cảm biến.

Giá trị timeout hiện dự kiến là 30 giây.

---

## 10. Phạm vi thực hiện

Dự án được triển khai theo các giai đoạn:

1. Hoàn thiện yêu cầu công nghệ và lưu đồ thuật toán.
2. Thiết kế phần cứng và thống nhất kết nối.
3. Cấu hình STM32 bằng STM32CubeMX.
4. Phát triển và kiểm thử từng module firmware.
5. Tích hợp các module thành hệ thống hoàn chỉnh.
6. Mô phỏng và kiểm thử trên Proteus.
7. Hiệu chỉnh thông số vận hành.
8. Nghiên cứu và triển khai chức năng IoT.
9. Đánh giá kết quả và hoàn thiện mô hình.

Các kết quả kiểm thử, lỗi còn tồn tại và tiến độ thực hiện được ghi nhận riêng trong mục `test`.
