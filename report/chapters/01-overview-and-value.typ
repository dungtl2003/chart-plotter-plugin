= Tổng quan đề tài và giá trị mang lại

Chương này xác lập bối cảnh của đề tài, chỉ ra bài toán và các thách thức kỹ thuật mà một thư viện trực quan hóa dữ liệu hiệu năng cao phải giải quyết, phân tích hạn chế của những giải pháp sẵn có, và quan trọng nhất là làm rõ #strong[giá trị cùng tính mới] mà sản phẩm mang lại cho đơn vị. Chương gồm các nội dung chính sau:
- Bối cảnh và nhu cầu thực tiễn.
- Bài toán và các thách thức kỹ thuật.
- Hạn chế của các giải pháp trực quan hóa hiện có.
- Giá trị và tính mới của sản phẩm.
- Mục tiêu và phạm vi.

== Bối cảnh và nhu cầu thực tiễn

Trong hoạt động nghiên cứu và vận hành tại các đơn vị kỹ thuật cao như Viện Hàng không Vũ trụ Viettel, dữ liệu dạng chuỗi thời gian (time series) xuất hiện ở khắp nơi: tín hiệu đo lường từ xa (telemetry) của thiết bị bay, dòng số liệu cảm biến, dữ liệu radar, log vận hành của hệ thống nhúng, kết quả mô phỏng số. Đặc điểm chung của các nguồn dữ liệu này là #strong[khối lượng rất lớn] và #strong[tốc độ sinh cao]: một phiên đo có thể sản sinh hàng triệu tới hàng chục triệu điểm, còn ở chế độ giám sát trực tuyến, dữ liệu chảy về liên tục không dừng.

Nhu cầu đặt ra là một công cụ trực quan hóa vừa phục vụ #strong[phân tích hồi cứu (offline)] - mở nguyên một tập dữ liệu lịch sử lớn để khảo sát - vừa phục vụ #strong[giám sát thời gian thực (online)] - theo dõi dòng dữ liệu đang tới với độ trễ thấp. Công cụ đó phải giữ được thao tác tương tác mượt mà (phóng to, thu nhỏ, kéo trượt, trỏ xem chi tiết) ngay cả khi số điểm lên tới hàng triệu, đồng thời phải nhẹ đủ để chạy trên cả máy trạm lẫn thiết bị hiển thị (HMI) nhúng với tài nguyên hạn chế.

Đây chính là khoảng trống mà đề tài hướng tới: xây dựng một #strong[plugin vẽ biểu đồ (Chart Plotter)] hiệu năng cao trên nền C++ và Qt Framework, đủ mạnh cho quy mô hàng triệu điểm và dòng thời gian thực, nhưng vẫn gọn nhẹ và dễ tích hợp vào bất kỳ ứng dụng Qt nào của đơn vị.

== Bài toán và các thách thức kỹ thuật

Bài toán cốt lõi - trực quan hóa dữ liệu chuỗi thời gian quy mô lớn, tương tác được, theo cả hai chế độ offline và online - phân rã thành bốn thách thức kỹ thuật. Mỗi thách thức tương ứng với một nhóm giải pháp được trình bày chi tiết ở các chương sau:

+ #strong[Hiệu năng ở quy mô hàng triệu điểm.] Vẽ trực tiếp toàn bộ điểm dữ liệu ra màn hình là bất khả thi: số lượng đỉnh hình học vượt xa số điểm ảnh (pixel) mà màn hình có thể biểu diễn, khiến tốc độ khung hình (FPS) sụp đổ. Cần một cơ chế #strong[giảm mẫu (downsampling)] thông minh, giữ được hình dạng trực quan của tín hiệu mà chỉ vẽ số điểm cỡ số pixel.

+ #strong[Dòng dữ liệu thời gian thực với cửa sổ trượt.] Ở chế độ online, dữ liệu tới liên tục nên bộ nhớ sẽ phình vô hạn nếu giữ tất cả. Cần một #strong[cửa sổ trượt] loại bỏ dữ liệu cũ theo thời gian, nhưng thao tác loại bỏ và cập nhật cấu trúc phải rẻ để không gây giật hình khi chạy dài ngày.

+ #strong[Chất lượng nét vẽ độc lập thiết bị và độ phân giải.] Nét biểu đồ phải mượt, không răng cưa, ở mọi mức phóng to và trên mọi thiết bị - kể cả phần cứng nhúng không hỗ trợ tốt khử răng cưa đa mẫu (MSAA). Điều này đòi hỏi một kỹ thuật khử răng cưa tự tính toán ngay trong bộ đổ bóng (shader).

+ #strong[Tính đóng gói và an toàn khi triển khai.] Sản phẩm phải là một thành phần #strong[tự chứa], dễ nhúng, không kéo theo các phụ thuộc thư viện bên ngoài gây rủi ro về giấy phép, kích thước hay chuỗi cung ứng - một yêu cầu đặc biệt quan trọng với môi trường triển khai của đơn vị.

== Hạn chế của các giải pháp trực quan hóa hiện có

Trên thị trường đã có nhiều thư viện vẽ biểu đồ, nhưng khi soi chiếu vào bốn thách thức trên, mỗi lựa chọn đều vấp phải ít nhất một giới hạn cốt tử.

Nhóm thư viện đi kèm hệ sinh thái Qt như #strong[Qt Charts]@qt_charts_module hướng tới sự tiện dụng và tính năng phong phú cho các biểu đồ quy mô vừa, nhưng không được thiết kế cho tập dữ liệu hàng triệu điểm cập nhật liên tục; hiệu năng suy giảm nhanh và thiếu cơ chế giảm mẫu tích hợp. Thư viện mã nguồn mở phổ biến #strong[QCustomPlot]@qcustomplot_lib mạnh về widget tĩnh nhưng dựa trên `QPainter` (CPU raster) nên khó đạt FPS cao với dữ liệu động cực lớn, và cũng không có khử răng cưa dựa trên shader. Các thư viện thương mại chuyên biệt tuy giải quyết được bài toán quy mô lớn nhưng đi kèm chi phí bản quyền cao và ràng buộc giấy phép - không phù hợp cho một thành phần cần nhúng sâu, tùy biến tự do và làm chủ hoàn toàn về mặt sở hữu trí tuệ.

#figure(
  table(
    columns: (20%, 20%, 20%, 20%, 20%),
    align: (left + horizon, center + horizon, center + horizon, center + horizon, center + horizon),
    inset: (top: 0.6em, bottom: 0.6em, left: 0.5em, right: 0.5em),
    table.header(
      table.cell(align: center)[#strong[Giải pháp]],
      table.cell(align: center)[#strong[Triệu điểm / thời gian thực]],
      table.cell(align: center)[#strong[Khử răng cưa bằng shader]],
      table.cell(align: center)[#strong[Phụ thuộc & giấy phép]],
      table.cell(align: center)[#strong[Nhúng / di động]],
    ),
    [Qt Charts / QtGraphs], [Hạn chế], [Không], [Theo Qt], [Trung bình],
    [QCustomPlot (QPainter/CPU)], [Hạn chế], [Không], [Nhẹ], [Trung bình],
    [Thư viện thương mại], [Tốt], [Có], [Bản quyền cao], [Ràng buộc],
    [#strong[ChartPlotter (đề tài)]], [#strong[Tốt]], [#strong[Có (SDF)]], [#strong[Tự chứa]], [#strong[Cao]],
  ),
  caption: "So sánh các giải pháp trực quan hóa theo bốn tiêu chí cốt lõi",
) <tbl-solution-compare>

Nói cách khác, các lựa chọn hiện có #strong[hoặc] nghẽn ở quy mô lớn, #strong[hoặc] kéo theo phụ thuộc/giấy phép nặng, #strong[hoặc] phụ thuộc khử răng cưa của phần cứng. Chưa có lời giải nào đồng thời thỏa mãn cả bốn tiêu chí trong đúng bối cảnh của đơn vị - và đó là lý do đề tài này ra đời.

== Giá trị và tính mới của sản phẩm

Giá trị cốt lõi của sản phẩm không nằm ở việc "vẽ lại được biểu đồ" - điều mọi thư viện đều làm - mà ở chỗ #strong[kết hợp một loạt kỹ thuật tiên tiến thành một plugin gọn nhẹ, giải đúng bốn thách thức đặc thù] mà các giải pháp sẵn có bỏ ngỏ. Bảng #ref(<tbl-value>, supplement: none) tổng hợp các điểm khác biệt, mỗi điểm đều gắn với một cơ sở kỹ thuật cụ thể và một luận cứ về độ phức tạp/số liệu (được chứng minh chi tiết ở Chương 4 và Chương 5).

#figure(
  table(
    columns: (24%, 42%, 34%),
    align: (left + horizon, left + horizon, left + horizon),
    inset: (top: 0.6em, bottom: 0.6em, left: 0.5em, right: 0.5em),
    table.header(
      table.cell(align: center)[#strong[Điểm khác biệt]],
      table.cell(align: center)[#strong[Cơ sở kỹ thuật & độ phức tạp]],
      table.cell(align: center)[#strong[Giá trị cho đơn vị]],
    ),
    [Vẽ triệu điểm vẫn mượt],
    [Kim tự tháp mức chi tiết (LOD) min/max: mỗi khung zoom/pan chỉ đọc số "xô" cỡ số pixel ngang, #strong[O(số pixel)] - gần như độc lập với tổng số điểm N],
    [Khảo sát tín hiệu/telemetry dài mà không giảm FPS, dù N là 1 triệu hay 100 triệu],

    [Thời gian thực + cửa sổ trượt],
    [Loại bỏ dữ liệu cũ chỉ #strong[O(số điểm bị loại)], không dựng lại cấu trúc; bộ nhớ có trần],
    [Màn hình giám sát chạy 24/7 không phình bộ nhớ, không khựng định kỳ],

    [Khử răng cưa tự viết bằng SDF],
    [Trường khoảng cách có dấu tính ngay trong fragment shader; chi phí #strong[O(1) mỗi điểm ảnh], không thêm hình học],
    [Nét mượt, độc lập độ phân giải, #strong[không cần MSAA phần cứng] - hợp thiết bị nhúng],

    [Plugin động tự chứa],
    [Phụ thuộc (spdlog, magic_enum) nhúng dạng header-only; thư viện `.so` không để lại dấu vết thư viện ngoài],
    [Triển khai an toàn, không rủi ro giấy phép/chuỗi cung ứng, dễ tích hợp],

    [Kiến trúc reader/parser trực giao],
    [Tách rời nguồn truyền (file/FIFO/socket/websocket) khỏi bộ phân tích định dạng: mọi transport ghép với mọi format],
    [Cắm được giao thức dữ liệu tùy biến của đơn vị mà không sửa lõi],

    [Nạp dữ liệu tốc độ cao],
    [Đường CSV nhanh: quét byte-span + `std::from_chars` không cấp phát; #strong[dưới 1 s (\~920 ms) cho 10 triệu dòng] (bản Release)],
    [Mở dữ liệu lịch sử lớn gần như tức thời],

    [Backend C++23 + kết xuất GPU],
    [Toàn bộ hình học đẩy xuống GPU, không vẽ lại bằng CPU mỗi khung],
    [Nhẹ CPU, dành tài nguyên cho các tác vụ tính toán khác],

    [Làm chủ hoàn toàn],
    [Mã nguồn thuộc sở hữu đơn vị, mở rộng theo bốn tầng song song (series/strategy/factory/renderer)],
    [Chủ động phát triển thêm loại biểu đồ, không phí bản quyền],
  ),
  caption: "Các điểm khác biệt của sản phẩm và giá trị tương ứng cho đơn vị",
) <tbl-value>

Ba luận điểm đáng nhấn mạnh nhất - cũng là "tính mới" so với mặt bằng công cụ hiện dùng:

- #strong[Chi phí hiển thị gần như hằng số theo số điểm.] Nhờ cấu trúc kim tự tháp LOD, thao tác zoom/pan không quét lại toàn bộ dữ liệu mà chỉ đọc một số "xô" tỉ lệ với bề rộng màn hình. Đây là điểm phá vỡ giới hạn tuyến tính O(N) mà các thư viện raster thông thường mắc phải, và là nền tảng cho trải nghiệm tương tác mượt ở quy mô triệu điểm.

- #strong[Khử răng cưa giải tích, không phụ thuộc phần cứng.] Thay vì trông cậy vào khử răng cưa đa mẫu của GPU, sản phẩm tính trực tiếp độ phủ của mỗi điểm ảnh theo khoảng cách hình học tới nét vẽ (kỹ thuật SDF@quilez_distfunctions2d). Nét vẽ vì thế sắc nét ở mọi mức phóng to và đồng nhất trên mọi thiết bị.

- #strong[Tự chứa và an toàn triển khai.] Việc đóng gói toàn bộ logic - kể cả các phụ thuộc - vào một thư viện động duy nhất, không để lại dấu vết thư viện ngoài, mang lại sự an tâm về giấy phép và bảo mật khi nhúng vào sản phẩm của một đơn vị kỹ thuật cao.

Tổng hòa, sản phẩm không phải bản sao của một thư viện có sẵn, mà là sự tích hợp có chủ đích các kỹ thuật ở trình độ hiện đại - giảm mẫu theo tam giác lớn nhất (LTTB)@steinarsson_downsampling_2013, kim tự tháp min/max, khử răng cưa SDF, ảnh chụp dữ liệu sao-chép-khi-ghi, điều tiết luồng nạp - đóng gói lại thành một công cụ đúng nhu cầu đặc thù của đơn vị.

== Mục tiêu và phạm vi

=== Mục tiêu

Đề tài đặt ra bốn nhóm mục tiêu:

+ #strong[Chức năng nền tảng và tùy biến biểu đồ.] Cung cấp API giao tiếp hai chiều giữa C++ và QML để thiết lập và vẽ biểu đồ; hỗ trợ ba loại biểu đồ Line, Bar và Pie, chọn loại theo cách khai báo trong QML; cho phép tùy biến màu sắc trực quan và kiểu nét cho biểu đồ đường (liền, đứt, chấm) ngay lúc chạy.

+ #strong[Chế độ nạp và xử lý dữ liệu.] Chế độ offline đọc trọn dữ liệu tĩnh, tự tính viewport/tỉ lệ (min-max của trục X, Y) và kết xuất trong một lượt quét; chế độ online xử lý và vẽ dòng dữ liệu thời gian thực.

+ #strong[Thao tác tương tác.] Phóng to/thu nhỏ theo trục X (trục hoành); trục Y (trục tung) tự co giãn theo biên độ của các điểm nằm trong khung nhìn hiện tại; kéo trượt (drag) khung nhìn dọc trục X; trỏ xem chi tiết (hover); và giới hạn lượng dữ liệu giữ lại bằng cửa sổ trượt, tự động loại bỏ dữ liệu cũ nhất khi vượt ngưỡng.

+ #strong[Nâng cao hiệu năng và kiến trúc.] Triển khai thuật toán giảm mẫu (LTTB và kim tự tháp LOD) ở tầng C++ để xử lý mượt tập dữ liệu hàng triệu điểm mà không giảm FPS; triển khai kỹ thuật khử răng cưa SDF tự viết trong fragment shader.

=== Phạm vi

Đề tài tập trung vào lõi thư viện trực quan hóa: ba loại biểu đồ (Line, Bar, Pie), hai chế độ dữ liệu (offline và online), tương tác zoom theo trục X kèm tự co giãn trục Y, kéo trượt, trỏ xem chi tiết, giới hạn dữ liệu theo cửa sổ trượt, và đóng gói sản phẩm dưới dạng một Qt plugin động cùng bộ ví dụ minh họa. Các vấn đề nằm ngoài phạm vi gồm: hệ thống thu thập/lưu trữ dữ liệu phía nguồn, và các loại biểu đồ chuyên biệt khác (scatter, area, candlestick,...) - được nêu như hướng phát triển ở phần kết luận.
