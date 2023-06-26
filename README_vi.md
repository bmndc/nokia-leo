| | Nokia 6300 4G (nokia-leo) |
| --- | --- |
| Ngày ra mắt | 13 tháng 11 năm 2020 |
| Mã máy | TA-1286, TA-1287, TA-1294, TA-1324 |
| ***Cấu hình*** | |
| Vi xử lý | Qualcomm MSM8909 Snapdragon 210<br>(4 x 1.1GHz Cortex-A7) |
| RAM | 512MB LPDDR2/3 |
| Chip đồ hoạ | Adreno 304 |
| Bộ nhớ trong | 4GB (+ tối đa 32GB thẻ nhớ microSDHC) |
| Mạng | - 2G GSM<br>- 3G UMTS<br>- 4G LTE (Cat 4): band 1, 3, 5, 7, 8, 20<br>- Hỗ trợ VoLTE & VoWiFi<br>- Hai khe thẻ Nano-SIM |
| Màn hình | 320 x 240 (167 PPI)<br>TFT LCD 2.4 inch, 16 triệu màu |
| Bluetooth | 4.0, A2DP, LE |
| Wi-Fi | 802.11b/g/n, có phát Wi-Fi |
| Tính năng đi kèm | GPS |
| Camera | Sau: VGA, có đèn flash |
| Kích thước<br>(D x R x C) | 131.4 x 53 x 13.7 (mm) |
| Khối lượng | 104.7 (g) |
| Cổng | - Cổng microUSB sạc và truyền dữ liệu USB 2.0<br>- Jack cắm tai nghe 3.5mm |
| Pin | Li-Ion 1500mAh (BL-4XL) có thể tháo rời |
| ***Phần mềm*** |  |
| Hệ điều hành | KaiOS 2.5.4 |
| Phiên bản | (TA-1286) 12.00.17.01, 20.00.17.01, 30.00.17.01 |

## Mẹo sử dụng

- Để chụp màn hình, nhấn cùng lúc hai phím `*` và `#`.
- Nhấn giữ 1 phím số 2-9 để bật tính năng Quay số nhanh.

## Vấn đề thường gặp

- Bình thường, KaiOS hay có chức năng tắt các ứng dụng (app) chạy nền và tối ưu RAM để giúp máy hoạt động được trơn tru nhất chỉ với lượng RAM ít ỏi thế này. Thế nhưng không rõ lý do gì mà trên máy này, tính năng tối ưu RAM hoạt động quá mức dẫn tới các ứng dụng bị tắt liên tục, kể cả cái đang hiện trên màn hình cũng bị "đột tử".

  Cái này có thể khắc phục bằng cách sau khi root máy (hướng dẫn ở dưới), thêm dòng này trong đoạn mã script khởi động của máy ở phân vùng /boot để tắt tính năng tối ưu RAM. Nhớ phải tạo thêm 1 tệp RAM ảo (swapfile) để tránh bị tràn RAM:
```
echo "0" > /sys/module/lowmemorykiller/parameters/enable_lmk
```
- Phím bấm thường xuyên bị nhảy chữ là do khoảng nhận phím của app bàn phím `keyboard.gaiamobile.org` trong hệ điều hành bị ngắn. Bên BananaHackers có [hướng dẫn khắc phục vấn đề này](https://ivan-hc.github.io/bananahackers/fix-the-keypad-speed.html), cần root máy trước khi thực hiện.
- Nếu để Wi-Fi bật liên tục thì sẽ bị hao pin cực nhanh. Không dùng thì nhớ tắt đi.
- Dữ liệu vị trí GPS có thể bị lệch khi bật 4G. Chưa biết lý do nhưng bắt buộc phải chỉnh về 3G/2G trong *Cài đặt > Mạng di động & dữ liệu > chọn SIM > Loại mạng* nếu muốn GPS xác định đúng vị trí.
- Nếu khoá máy mà quên mã PIN thì có thể phá khoá bằng cách giữ nút nguồn trên đỉnh máy, chọn *Trình dọn dẹp Bộ nhớ > Dọn dẹp Bộ nhớ sâu*.

### KaiOS nói chung

- Tin nhắn không tự chuyển từ SMS sang MMS khi nhắn trong nhóm, dẫn tới phiền phức khi tin nhắn không gửi lên nhóm mà lại gửi riêng biệt đến từng người trong nhóm. Chỉ có thể tạm thời khắc phục bằng cách thêm "chủ đề" (*Tuỳ chọn > Thêm chủ đề*) hoặc thêm file đính kèm mỗi lần nhắn để nó chuyển sang gửi bằng MMS.
- Nếu di chuyển giữa mấy ô nhập liệu thì chế độ gõ T9 sẽ tự chuyển về mặc định, kiểu nếu bật gõ viết hoa, viết số hay tiên đoán từ ở lần gõ trước thì lần gõ sau phải tự bật lại kiểu gõ đấy.
- Không đổi được âm báo tin nhắn mới hay âm báo báo thức ngoài danh sách được chỉ định, kể cả sau khi thiết lập trong Cài đặt. Hệ điều hành không quản lý hai loại này mà lại để cho app Tin nhắn (`sms.gaiamobile.org`) và Đồng hồ (`clock.gaiamobile.org`) tự quản lý.

  Để mà đổi 2 loại âm báo này thì cần phải root máy, sau đó dùng ADB để trích thư mục 2 app từ `/system/b2g/webapps`, giải nén, thay file nhạc mặc định rồi nén app lại và chuyển vào `/data/local/webapps`. Sau đó tiếp tục dùng ADB để trích xuất file `/data/local/webapps/webapps.json` và đổi cái `basePath` của 2 app từ `system/b2g/webapps` sang `/data/local/webapps`.
  Bên BananaHackers có [hướng dẫn cách làm chi tiết](https://ivan-hc.github.io/bananahackers/clock-alarms.html#h.unmy3yif91xs) để đổi 2 loại âm báo này.
- Chức năng đồng bộ email, lịch và danh bạ với tài khoản Google hay gặp lỗi không đồng bộ được. Nên dùng thiết lập Nâng cao trong app E-mail và chỉ NHẬP danh bạ thay bị đồng bộ trong app Danh bạ.
- Nhắc tới lịch, kể cả nếu thiết lập được đồng bộ lịch của tài khoản Google với điện thoại, chỉ có cái lịch với tên đặt theo địa chỉ email thì mới đồng bộ vào app Lịch mặc định trên máy.
- Những ứng dụng mặc định trên máy, kể cả app Danh bạ và Nhạc, được viết trên thư viện React nổi tiếng với việc lãng phí tài nguyên, dẫn tới việc các app tải rất chậm, đặc biệt khi có nhiều mục lưu trữ trong Danh bạ hay có nhiều bài nhạc trong máy. Chưa kể KaiOS 2.5 được thiết kế trên nền tảng Gecko 48.0a1 lỗi thời từ 2016.
- Thiếu tính năng và ứng dụng bên thứ ba là vấn đề hệ trọng tất yếu.

## Code ẩn

- `*#*#33284#*#*`: Bật/tắt chế độ gỡ lỗi để can thiệp sâu bên trong điện thoại bằng ADB và DevTools để cài ứng dụng bên ngoài, truy cập phân vùng hệ thống...
- `*#06#`: Hiển thị Mã nhận dạng thiết bị quốc tế IMEI, dùng để khoá máy từ xa hoặc thử nghiệm ứng dụng trên Cổng phát hành ứng dụng KaiStore dành cho lập trình viên (KaiStore Submission Portal).
- `*#0000#`: Hiển thị một số thông tin thiết bị như số phiên bản, ngày phát hành phiên bản, mã số máy và mã cập nhật CUID.

## Chế độ khởi động đặc biệt

- **Chế độ phục hồi**: Khi máy đang tắt, nhấn giữ phím nguồn trên đỉnh máy và phím `*` cùng lúc; hoặc chạy lệnh `adb reboot recovery` khi đang kết nối với máy tính. Khi ở trong chế độ này, có thể xoá mọi dữ liệu trong máy bằng cách xoá phân vùng /data và /cache (đặc biệt hữu ích khi quên mã PIN mở máy), xem nhật ký và phát hiện các lỗi khi khởi động máy, cài file cập nhật thông qua `adb sideload` hoặc từ thẻ nhớ.
- **Chế độ EDL**: Khi máy đang tắt, nhấn đồng thời phím nguồn trên đỉnh máy, phím `*` và `#`; hoặc chạy lệnh `adb reboot edl` khi đang kết nối với máy tính. Màn hình sẽ chớp logo `enabled by KaiOS` rồi tắt, lúc này nếu có file chữ ký phù hợp, có thể sử dụng phần mềm đặc chế của Qualcomm để thoải mái đọc và ghi phân vùng trên máy mà không bị hệ điều hành ngăn cấm. Để thoát chế độ này cần tháo và lắp lại pin.

Kho lưu trữ file chữ ký EDL của BananaHackers có file chữ ký EDL dành cho các phiên bản máy Nokia 6300 4G không phải thị trường Hoa Kỳ (TA-1324), có thể tải về tại [đây](https://edl.bananahackers.net/loaders/8k.mbn) với ID phần cứng 0x009600e100420029 (repo có sẵn [1 bản dự phòng](https://github.com/minhduc-bui1/nokia-leo/blob/blob/main/8k.mbn)). Phiên bản thị trường Hoa Kỳ của máy này có chữ ký PK_HASH khác với bản quốc tế, do đó cần một file chữ ký EDL khác không có trong kho của BananaHackers.
