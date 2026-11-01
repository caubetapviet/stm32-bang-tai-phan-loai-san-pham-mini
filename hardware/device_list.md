# DEVICE LIST – MINI CONVEYOR SORTING SYSTEM

## 1. Tổng quan

Danh sách thiết bị sử dụng trong dự án băng tải phân loại sản phẩm theo chiều cao, sử dụng STM32F103C8T6.

## 2. Danh sách thiết bị

### 2.1. Bộ điều khiển và giao tiếp

| STT | Thiết bị                 | Model                     | Số lượng | Chức năng                                     | 
| --: | ------------------------ | ------------------------- | -------: | --------------------------------------------- | 
|   1 | Vi điều khiển            | STM32F103C8T6 (Blue Pill) |        1 | Điều khiển toàn bộ hệ thống                   |         
|   2 | Bộ nạp chương trình      | ST-Link V2                |        1 | Nạp chương trình cho vi điều khiển            |         
|   3 | Bộ chuyển đổi USB-to-TTL | CH340                     |        1 | Giao tiếp UART với máy tính                   |         
|   4 | Module Wi-Fi             | ESP8266 ESP-01            |        1 | Kết nối Wi-Fi, phục vụ chức năng giám sát IoT |         

### 2.2. Cảm biến

| STT | Thiết bị                   | Model     | Số lượng | Chức năng                              | 
| --: | -------------------------- | --------- | -------: | -------------------------------------- | 
|   5 | Cảm biến quang             | E18-D80NK |        1 | CB_Thap – phát hiện tất cả sản phẩm    |         
|   6 | Module cảm biến hồng ngoại | LM393     |        1 | CB_Cao – phát hiện sản phẩm cao loại A |         

### 2.3. Cơ cấu chấp hành

| STT | Thiết bị                  | Model         | Số lượng | Chức năng                                |
| --: | ------------------------- | ------------- | -------: | ---------------------------------------- | 
|   7 | Động cơ servo             | TowerPro SG90 |        1 | Điều khiển cánh gạt phân loại sản phẩm A | 
|   8 | Động cơ DC                | F130          |        1 | Truyền động cho băng tải                 |                          
|   9 | Module điều khiển động cơ | L298N         |        1 | Điều khiển động cơ DC                    |                          

### 2.4. Hiển thị và cảnh báo

| STT | Thiết bị                | Model                     | Số lượng | Chức năng                                 | 
| --: | ----------------------- | ------------------------- | -------: | ----------------------------------------- | 
|  10 | LCD 16x2 kèm module I2C | LM016L + HW-61 (PCF8574T) |     1 bộ | Hiển thị trạng thái, chế độ và bộ đếm     | 
|  11 | Buzzer chủ động         | SFM-20B                   |        1 | Cảnh báo khi hệ thống xảy ra lỗi          |                          
|  12 | LED xanh                | LED đơn                   |        1 | Báo trạng thái RUN                        |                          
|  13 | LED đỏ                  | LED đơn                   |        1 | Báo trạng thái FAULT                      |                          
|  14 | Nút nhấn                | Push button               |        6 | START, STOP, MODE, RESET, CONVEYOR, SERVO |                          

### 2.5. Nguồn cấp và chuyển đổi điện áp

| STT | Thiết bị          | Model       | Số lượng | Chức năng                            | 
| --: | ----------------- | ----------- | -------: | ------------------------------------ | 
|  15 | Nguồn tổ ong      | 12 V DC     |        1 | Cung cấp nguồn DC chính cho hệ thống |         
|  16 | Module Buck DC-DC | LM2596      |        1 | Hạ áp từ nguồn DC chính              |         
|  17 | Module ổn áp      | AMS1117-3.3 |        1 | Cung cấp nguồn 3.3 V cho ESP-01      |         

## 3. Ghi chú

- LCD 16x2 và module I2C được tính là một bộ thiết bị, không tách thành hai mục riêng.
- ESP-01 và AMS1117-3.3 thuộc phần mở rộng IoT của dự án.
- Số lượng trong danh sách là số lượng dự kiến sử dụng; sẽ cập nhật nếu thay đổi trong quá trình triển khai.
- Thông số kỹ thuật, hình ảnh, giá thành, nơi mua của từng thiết bị được trình bày trong các tài liệu tương ứng thuộc thư mục `device_info/`.

