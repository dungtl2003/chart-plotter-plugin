#heading(numbering: none)[Kết luận và hướng phát triển]

#strong[Kết quả đạt được]

Qua quá trình thực hiện đồ án, em đã xây dựng hoàn chỉnh #strong[ChartPlotter] - một thư viện vẽ biểu đồ hiệu năng cao đóng gói dưới dạng plugin QML của Qt 6, với lõi xử lý C++23 và kết xuất trên GPU. Sản phẩm đạt được các kết quả cụ thể sau:

- #strong[Về lý thuyết:]
  - Nắm vững và vận dụng được một loạt kỹ thuật trực quan hóa dữ liệu quy mô lớn: giảm mẫu giữ hình dạng (LTTB), kim tự tháp mức chi tiết (LOD) min\/max cho zoom\/pan chi phí $O("số pixel")$, và khử răng cưa giải tích dựa trên trường khoảng cách có dấu (SDF) trong fragment shader.
  - Hiểu sâu mô hình lập trình đa luồng và bất đồng bộ của Qt: tách luồng đọc dữ liệu, điều tiết luồng bằng semaphore đếm (backpressure), bàn giao dữ liệu xuyên luồng kiểu sao chép khi ghi (COW), và dừng an toàn theo hai pha.
  - Làm chủ đường ống đồ họa OpenGL và cơ chế tích hợp lệnh OpenGL thô vào scene graph của Qt Quick qua `QSGRenderNode`, cùng giao tiếp hai chiều C++ #sym.arrow.l.r QML qua Meta-Object System.

- #strong[Về ứng dụng:]
  - Đã xây dựng thành công thư viện hỗ trợ ba loại biểu đồ (đường, cột, tròn), hai chế độ dữ liệu tĩnh (offline) và dòng thời gian thực (online), cùng các thao tác tương tác zoom trục X kèm tự co giãn trục Y, kéo trượt, trỏ xem chi tiết và giới hạn dữ liệu theo cửa sổ trượt.
  - Sản phẩm đạt các chỉ tiêu hiệu năng đề ra: nạp 10 triệu dòng CSV trong #strong[dưới 1 giây] (\~920 ms) trên bản dựng Release, và giữ được chi phí mỗi khung tương tác ở cỡ số điểm ảnh - #strong[độc lập với tổng số điểm dữ liệu]. Phép thử áp lực trên tệp \~5 GB (38,5 triệu dòng, 20 series đường) cho thấy bộ nhớ đạt trần \~7,5 GiB rồi #strong[phẳng] nhờ cửa sổ trượt 10 triệu dòng, và FPS giữ ổn định quanh 50 ngay cả khi dữ liệu vẫn đang được nạp song song.
  - Toàn bộ logic được đóng gói gọn thành một thư viện động tự chứa (`libChartPlotter.so`), nhúng sẵn các phụ thuộc header-only nên không để lại dấu vết thư viện ngoài - thuận lợi cho việc triển khai an toàn và tích hợp vào ứng dụng Qt bất kỳ.
  - Kèm theo một bộ ví dụ đầy đủ minh họa từng loại biểu đồ, chế độ dữ liệu và kịch bản sử dụng, trong đó có ví dụ dòng dữ liệu trực tiếp qua WebSocket.

#strong[Hạn chế]

Do giới hạn về thời gian và phạm vi đồ án, sản phẩm vẫn còn một số hạn chế:

- Bộ lập kế hoạch bố cục (`ChartLayoutPlanner`) hiện tính lề giữa vùng vẽ và các thành phần (tiêu đề, chú giải, trục) bằng các #strong[hằng số gán cứng (magic number)] thay vì đo theo kích thước thật của từng thành phần, nên bố cục chưa thật sự co giãn tối ưu ở mọi cấu hình.
- Biểu đồ tròn hiện ở dạng đơn giản: mỗi biểu đồ chỉ một series.
- Dự án #strong[chưa có bộ kiểm thử tự động]; việc xác minh thay đổi hiện dựa vào chạy ví dụ và quan sát kết quả kết xuất, chưa đủ bảo đảm chống hồi quy khi mã lớn dần.
- Phần kết xuất gắn với OpenGL 3.3 Core, chưa trừu tượng hóa qua lớp đồ họa của Qt nên khả năng di động sang nền tảng đồ họa khác còn hạn chế.

#strong[Hướng phát triển]

Từ các kết quả và hạn chế trên, em đề xuất các hướng phát triển tiếp theo:

- #strong[Mở rộng loại biểu đồ]: bổ sung các series mới như điểm rời rạc (scatter), vùng (area) và nến (candlestick) - tận dụng đúng điểm nối mở rộng bốn tầng series\/strategy\/factory\/renderer đã thiết kế, nên chi phí thêm loại mới là tối thiểu.
- #strong[Hoàn thiện bố cục]: thay các hằng số lề gán cứng bằng phép đo kích thước thật của tiêu đề, chú giải và nhãn trục để bố cục tự canh chính xác ở mọi cấu hình.
- #strong[Trừu tượng hóa lớp đồ họa]: chuyển phần kết xuất sang lớp trừu tượng đồ họa của Qt (RHI) để chạy được trên nhiều backend (Vulkan, Metal, Direct3D) ngoài OpenGL, tăng khả năng di động - đặc biệt trên các thiết bị nhúng dùng backend khác.
- #strong[Kiểm thử và kiểm chứng]: xây dựng bộ kiểm thử hồi quy tự động cho các thuật toán lõi (giảm mẫu, tính biên, canh lề kim tự tháp) và đo hiệu năng có hệ thống, kể cả kiểm chứng đầy đủ chế độ truy vấn LTTB của kim tự tháp.
