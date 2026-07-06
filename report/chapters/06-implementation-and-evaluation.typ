= Triển khai, thử nghiệm và đánh giá

Ba chương trước trình bày kiến trúc và các kỹ thuật lõi. Chương này khép lại vòng: chứng minh sản phẩm #strong[chạy thật] và #strong[đạt các mục tiêu] đã đặt ở Chương 1. Nội dung đi từ môi trường và quy trình dựng sản phẩm, tới bộ ví dụ minh họa, kết quả đo hiệu năng định lượng, phần trình diễn giao diện - chức năng, và cuối cùng là bảng đối chiếu từng mục tiêu với trạng thái đạt được kèm bằng chứng. Chương gồm các nội dung chính sau:
- Môi trường, công cụ và quy trình dựng sản phẩm.
- Bộ ví dụ minh họa.
- Kết quả hiệu năng định lượng.
- Trình diễn giao diện và chức năng.
- Đối chiếu kết quả với mục tiêu.

== Môi trường, công cụ và quy trình dựng sản phẩm

Sản phẩm được xây dựng và kiểm thử trên nền tảng Linux với chuỗi công cụ sau: Qt #strong[6.8+], CMake #strong[3.20+], trình dựng Ninja và một trình biên dịch hỗ trợ #strong[C++23]. Hai phụ thuộc bên thứ ba - `spdlog` (ghi nhật ký) và `magic_enum` (phản chiếu enum) - được kéo về ở dạng #strong[header-only] qua `FetchContent` ngay lần cấu hình đầu, rồi biên dịch thẳng vào thư viện. Nhờ vậy sản phẩm cuối `libChartPlotter.so` #strong[không để lại dấu vết thư viện ngoài] khi triển khai - một tiêu chí an toàn chuỗi cung ứng đã nêu ở Chương 1.

Cấu trúc mã nguồn tuân thủ quy ước tách #strong[header] (`include/ChartPlotter/`) và #strong[source] (`src/`) theo bố cục ánh xạ gương, toàn bộ nằm trong `namespace ChartPlotter`. Dự án #strong[không dùng] cơ chế gom tệp tự động (globbing): mỗi tệp `.cpp`/`.hpp` mới phải được khai báo tường minh vào danh sách `CHARTPLOTTER_SOURCES`/`CHARTPLOTTER_HEADERS` trong `CMakeLists.txt`. Đánh đổi công khai báo thủ công này lấy một quy trình dựng #strong[tất định]: danh sách biên dịch không bao giờ thay đổi ngầm theo trạng thái thư mục, tránh lỗi khó truy vết khi một tệp lọt vào hay rơi khỏi bản dựng ngoài ý muốn.

Bên cạnh bản dựng thường, dự án cấu hình sẵn một cây dựng riêng bật #strong[AddressSanitizer] (ASan) ở chế độ gỡ lỗi để phát hiện lỗi truy cập bộ nhớ (tràn biên, dùng sau khi giải phóng) - đặc biệt hữu ích với phần quản lý bộ đệm đa luồng và vòng đời đối tượng scene graph. Việc đo hiệu năng luôn thực hiện trên bản dựng #strong[Release] (`-DCMAKE_BUILD_TYPE=Release`); bản dựng mặc định không tối ưu chậm hơn khoảng một bậc độ lớn nên không dùng để trích dẫn số liệu.

Sản phẩm được tiêu thụ đơn giản bằng cách `import ChartPlotter` trong QML. Mỗi ví dụ dùng `qt_add_qml_module` và nạp giao diện qua `qrc:/qt/qml/<Uri>/qml/Main.qml`, với đường dẫn nạp plugin truyền vào qua định nghĩa biên dịch `PLUGIN_IMPORT_PATH`.

== Bộ ví dụ minh họa

Để kiểm chứng đủ bề rộng chức năng, dự án kèm một bộ ví dụ độc lập, mỗi ví dụ là một ứng dụng Qt hoàn chỉnh nhúng plugin và tập trung vào một khía cạnh. @tbl-examples liệt kê các ví dụ tiêu biểu.

#figure(
  table(
    columns: (32%, 68%),
    align: (left + horizon, left + horizon),
    inset: (top: 0.6em, bottom: 0.6em, left: 0.5em, right: 0.5em),
    table.header(
      table.cell(align: center)[#strong[Ví dụ]],
      table.cell(align: center)[#strong[Nội dung minh họa]],
    ),
    [`BasicChart`], [Biểu đồ đường cơ bản - điểm khởi đầu tối giản của API.],
    [`BarChart`], [Biểu đồ cột trên trục phân loại (category).],
    [`NumericBarChart`], [Biểu đồ cột trên trục số, bề rộng dải suy ra từ khoảng cách hai giá trị $x$ kề nhau.],
    [`MixedChart`], [Một series cột và nhiều series đường trên cùng một trục phân loại dùng chung.],
    [`CategoryLineChart`], [Đường trên trục phân loại chuỗi.],
    [`DateTimeLineChart`], [Đường trên trục thời gian, kiểm chứng đường phân tích ngày tháng nhanh.],
    [`PieChart`], [Biểu đồ tròn - series phi Cartesian, chuẩn hóa theo tổng.],
    [`ChartGallery`],
    [Lưới nhiều thẻ biểu đồ (đường, cột, tròn, hỗn hợp) nạp từ nhiều tệp CSV - trình diễn cùng lúc mọi loại.],

    [`RealTimeChart`], [Dòng dữ liệu thời gian thực với cửa sổ trượt.],
    [`BinanceChart`],
    [Dòng thời gian thực thật qua WebSocket tới sàn Binance - kiểm chứng đường reader mạng và chống chặn ngược.],

    [`LargeDatasetChart`], [Nạp `temp/large.csv` (10 triệu dòng, \~137 MB) - phép đo chuẩn cho thông lượng nạp.],
    [`VeryLargeDatasetChart`],
    [Nạp tệp cỡ \~5 GB với `maxCacheRows` giới hạn cửa sổ trượt ở 10 triệu dòng - kiểm chứng loại bỏ dữ liệu cũ và trần bộ nhớ.],
  ),
  caption: [Bộ ví dụ minh họa kèm theo sản phẩm],
) <tbl-examples>

== Kết quả hiệu năng định lượng

=== Thông lượng nạp dữ liệu

Phép đo chuẩn dùng ví dụ `LargeDatasetChart` nạp tệp `temp/large.csv` gồm #strong[10 triệu dòng], 2 cột số, dung lượng \~137 MB, trên bản dựng Release. Toàn bộ chuỗi đọc - phân tích - ghi bộ đệm chạy đồng bộ trên luồng xử lý dữ liệu qua kết nối `Qt::DirectConnection`, nên con số đo được là chi phí nạp thuần, không lẫn thời gian kết xuất. @tbl-ingest tổng hợp kết quả.

#figure(
  table(
    columns: (56%, 44%),
    align: (left + horizon, center + horizon),
    inset: (top: 0.6em, bottom: 0.6em, left: 0.5em, right: 0.5em),
    table.header(
      table.cell(align: center)[#strong[Hạng mục]],
      table.cell(align: center)[#strong[Kết quả]],
    ),
    [Tổng thời gian nạp 10 triệu dòng], [\~920 ms],
    [Quy đổi số (`std::from_chars`)], [\~358 ms],
    [Ghi bộ đệm cột (bulk append)], [\~71 ms],
    [Đọc tệp và quét CSV], [\~491 ms],
    [Thông lượng trung bình], [$approx 1.1 times 10^7$ dòng\/giây],
  ),
  caption: [Phân rã thời gian nạp 10 triệu dòng CSV (bản dựng Release)],
) <tbl-ingest>

Điểm đáng chú ý là sự phân rã chi phí. Phần #strong[quy đổi số] chiếm phần lớn công việc phân tích (\~358 ms) - đây chính là chỗ đường CSV nhanh phát huy tác dụng: quét byte phẳng và gọi `std::from_chars`@cppreference_from_chars thẳng trên các dải byte, không cấp phát chuỗi trung gian trong trường hợp thường gặp. Phần #strong[ghi bộ đệm] chỉ \~71 ms nhờ chiến lược nối cột theo lô, mỗi lô một lần khóa mutex, đúng như phân tích ở Chương 4. Phần còn lại (\~491 ms) là chi phí đọc tệp và quét ranh giới trường CSV. Nếu bỏ đường nhanh mà quay lại phân tích theo từng ô tạo `QString`/`QVariant`, riêng khâu quy đổi và cấp phát đã đủ đẩy tổng thời gian lên nhiều lần.

=== Chi phí tương tác độc lập với kích thước dữ liệu

Với thao tác zoom, pan và vẽ lại mỗi khung, kết quả then chốt không phải một con số FPS đơn lẻ mà là #strong[dạng tiệm cận của chi phí]. Toàn bộ phân tích ở Chương 4 và Chương 5 dẫn tới cùng một kết luận: mỗi khung tương tác có chi phí ở cỡ #strong[số điểm ảnh của vùng vẽ] ($P$), #strong[độc lập với tổng số điểm dữ liệu $N$]:

- Truy vấn kim tự tháp LOD để lấy các điểm hiển thị: $O("số xô hiển thị") approx O(P)$, không phải $O(N)$.
- Loại bỏ dữ liệu cũ theo cửa sổ trượt: $O("số dòng bị loại")$, không dựng lại cấu trúc.
- Dựng hình học và tô trên GPU: $O(P)$ mỗi khung.

Chính bất biến này là điều ví dụ `VeryLargeDatasetChart` kiểm chứng trong thực tế: một tệp \~5 GB với cửa sổ trượt ghim ở 10 triệu dòng vẫn zoom\/pan tương tác được, và bộ nhớ #strong[không phình] theo thời gian nhờ loại bỏ nguyên khối dữ liệu cũ. Con số FPS tuyệt đối phụ thuộc phần cứng đích (nhất là khi nhắm tới thiết bị nhúng HMI), nhưng #strong[hình dạng] chi phí thì không đổi - đó mới là bảo đảm mang tính kiến trúc.

Để lượng hóa hai bất biến trên, một #strong[phép thử áp lực (stress test)] được thực hiện trên ví dụ `VeryLargeDatasetChart` với cấu hình sát dữ liệu telemetry thực: một tệp CSV tĩnh \~5 GB gồm #strong[38,5 triệu dòng], mỗi dòng 21 cột (1 cột thời gian và 20 cột số), vẽ đồng thời #strong[20 series đường]; cửa sổ trượt giới hạn #strong[10 triệu dòng] giữ trong bộ nhớ. @tbl-stress tổng hợp kết quả đo, lấy từ nhật ký hiệu năng của `ChartView` (bản dựng Release, bật ghi nhật ký gỡ lỗi).

#figure(
  table(
    columns: (54%, 46%),
    align: (left + horizon, left + horizon),
    inset: (top: 0.6em, bottom: 0.6em, left: 0.5em, right: 0.5em),
    table.header(
      table.cell(align: center)[#strong[Thông số]],
      table.cell(align: center)[#strong[Giá trị đo]],
    ),
    [Tệp nguồn], [CSV tĩnh \~5 GB, 38,5 triệu dòng],
    [Cấu trúc mỗi dòng], [21 cột: 1 cột thời gian + 20 cột số],
    [Số series vẽ đồng thời], [20 đường],
    [Giới hạn cửa sổ trượt (`maxCacheRows`)], [10 triệu dòng],
    [Thời gian đọc + phân tích toàn tệp], [\~37 giây],
    [FPS tương tác khi đã ổn định], [\~50 FPS (49-53), đỉnh \~90],
    [Bộ nhớ thường trú (RSS) khi ổn định], [\~7,5 GiB, phẳng sau khi đạt trần],
  ),
  caption: [Phép thử áp lực: 20 series đường trên tệp \~5 GB \/ 38,5 triệu dòng, cửa sổ trượt 10 triệu dòng],
) <tbl-stress>

Hai quan sát khớp chính xác với dự đoán kiến trúc. Thứ nhất, #strong[bộ nhớ đạt trần rồi phẳng]: trong lúc nạp, RSS leo dần theo bộ đệm đang đầy lên (từ \~0,9 GiB lên \~6,5 GiB); nhưng khi số dòng chạm trần 10 triệu, RSS #strong[dừng ở \~7,5 GiB và không tăng nữa] dù tệp 5 GB vẫn còn đang được đọc vào - đúng cơ chế loại bỏ nguyên khối theo cửa sổ trượt (Chương 4): bộ nhớ #strong[bị chặn] chứ không tỉ lệ với $N$. Thứ hai, #strong[FPS ổn định quanh 50] khi vẽ 20 series trên 10 triệu điểm giữ lại, và giữ nguyên mức đó #strong[ngay cả khi luồng nạp vẫn đang chạy song song] - xác nhận chi phí mỗi khung ở cỡ số điểm ảnh chứ không theo số điểm dữ liệu. Con số tuyệt đối (\~50 FPS) phụ thuộc phần cứng và số series, nhưng việc nó #strong[không suy giảm] theo thời gian khi dữ liệu tiếp tục đổ vào mới là điều cốt lõi.

Về phía nạp, khâu đọc và khâu phân tích chạy trên #strong[hai luồng riêng], ghép với nhau qua semaphore đếm (chặn ngược): luồng đọc #strong[không được chạy trước] luồng xử lý quá \~8 MB (4 khối $times$ 2 MB), nên khi một trong hai chậm hơn thì stage kia bị ghì lại theo. Với tệp 5 GB này, khâu phân tích chỉ tốn \~18 giây thời gian CPU (\~16 giây quy đổi số, \~1,4 giây ghi cột) trong khi tổng nạp là \~37 giây; tức #strong[khâu đọc mới là stage chậm hơn], còn khâu phân tích thừa sức tiêu thụ kịp và phải chờ dữ liệu tới. Do đó tổng thời gian bám theo tốc độ nạp byte của luồng đọc, và toàn bộ chi phí phân tích được #strong[chồng lấn] khuất sau nó thay vì cộng dồn thêm. Bản thân thông lượng quy đổi đạt \~47 triệu giá trị số\/giây (38,5 triệu dòng $times$ 20 cột trong \~16 giây) - cùng cỡ với phép đo hai cột ở @tbl-ingest, cho thấy chi phí phân tích co giãn tuyến tính theo tổng số giá trị bất kể số cột mỗi dòng.

=== Các hằng số cấu hình then chốt

Để phần đo có thể tái lập, @tbl-constants tập hợp các hằng số điều khiển hành vi hiệu năng và kết xuất, tất cả tập trung tại `constants/ChartConstants.hpp` cùng vài lớp lõi.

#figure(
  table(
    columns: (40%, 18%, 42%),
    align: (left + horizon, center + horizon, left + horizon),
    inset: (top: 0.55em, bottom: 0.55em, left: 0.5em, right: 0.5em),
    table.header(
      table.cell(align: center)[#strong[Hằng số]],
      table.cell(align: center)[#strong[Giá trị]],
      table.cell(align: center)[#strong[Vai trò]],
    ),
    [`DataChunk::CHUNK_SIZE`], [10000], [Số dòng mỗi khối cột.],
    [`LodPyramid::kBranch`], [4], [Số con mỗi xô của kim tự tháp LOD.],
    [`LodPyramid::kMinTopBuckets`], [128], [Ngưỡng dừng làm thô ở đỉnh.],
    [`LTTB_BUCKET_SIZE_THRES`], [500], [Tỉ lệ trên mức này thì dùng trung bình thay LTTB.],
    [`MIN_ZOOM_SPAN_FRACTION`], [$10^(-4)$], [Sàn zoom-in theo tỉ lệ dải dữ liệu.],
    [`MIN_ZOOM_MAGNITUDE_FRACTION`], [$10^(-9)$], [Sàn zoom-in theo độ lớn giá trị (trục epoch-ms).],
    [Độ sâu hàng đợi reader], [4], [Số khối tối đa đang lưu chuyển (chặn ngược).],
    [Kích thước khối đọc], [2 MB], [Đơn vị đọc mỗi vòng của reader.],
    [Nấc lăn chuột], [$120$ đơn vị], [$15 degree$ mỗi nấc; $"số bước" = "góc"\/8\/15$.],
    [`zoomFactor`], [0.9], [Hệ số zoom hình học mỗi bước.],
    [`FPS_MIN` \/ `FPS_MAX`], [1 \/ 240], [Kẹp tần số khung khi điều tiết cập nhật.],
  ),
  caption: [Các hằng số cấu hình then chốt],
) <tbl-constants>

== Giao diện và chức năng

Sản phẩm demo trình diễn đầy đủ các nhóm chức năng đã cam kết ở Chương 1:

- #strong[Tùy biến biểu đồ]: đổi màu series (và màu từng lát của biểu đồ tròn) qua bảng thiết lập; đổi kiểu nét đường giữa nét liền, nét đứt và nét chấm - tất cả ngay lúc chạy. Việc chọn loại biểu đồ (đường, cột, tròn) là #strong[khai báo trong QML], mỗi loại có ví dụ riêng, không phải thao tác chuyển đổi khi đang chạy.
- #strong[Hai chế độ dữ liệu]: nạp tĩnh (offline) toàn bộ tệp rồi tự tính viewport, và dòng thời gian thực (online) với cửa sổ trượt, minh họa trực tiếp bằng ví dụ Binance qua WebSocket.
- #strong[Tương tác]: phóng to\/thu nhỏ theo trục X quanh vị trí con trỏ kèm trục Y tự co giãn theo biên độ các điểm trong khung nhìn; kéo trượt khung nhìn dọc trục X; trỏ chuột xem chi tiết (hover) với hộp thông tin bám theo điểm; và giới hạn dữ liệu bằng cửa sổ trượt (`maxCacheRows`), tự loại bỏ dữ liệu cũ nhất khi vượt ngưỡng.

Các hình dưới đây minh họa trực tiếp những chức năng vừa nêu.

#strong[Tùy biến qua bảng thiết lập.] @fig-demo-settings là bảng thiết lập cho phép đổi màu series (và màu từng lát của biểu đồ tròn) cùng kiểu nét đường (liền, đứt, chấm) ngay khi ứng dụng đang chạy.

#figure(
  image("/assets/images/demo-settings.png", width: 100%),
  caption: [Bảng thiết lập: đổi màu series và kiểu nét đường ngay lúc chạy],
) <fig-demo-settings>

#strong[Ba loại biểu đồ.] @fig-demo-line, @fig-demo-bar và @fig-demo-pie lần lượt cho thấy biểu đồ đường, cột và tròn - mỗi loại khai báo trong QML và có ví dụ riêng.

#figure(
  image("/assets/images/demo-line.png", width: 100%),
  caption: [Biểu đồ đường cơ bản],
) <fig-demo-line>

#figure(
  image("/assets/images/demo-bar.png", width: 100%),
  caption: [Biểu đồ cột],
) <fig-demo-bar>

#figure(
  image("/assets/images/demo-pie.png", width: 100%),
  caption: [Biểu đồ tròn],
) <fig-demo-pie>

#strong[Nhiều series trên cùng một trục.] @fig-demo-multi-lines cho thấy nhiều series đường chung một trục phân loại; @fig-demo-line-bar-mix là biểu đồ hỗn hợp gồm một series cột và nhiều series đường - chứng minh khả năng chồng nhiều series khác loại trên cùng hệ trục.

#figure(
  image("/assets/images/demo-multi-lines.png", width: 100%),
  caption: [Nhiều series đường trên cùng một trục phân loại],
) <fig-demo-multi-lines>

#figure(
  image("/assets/images/demo-line-bar-mix.png", width: 100%),
  caption: [Biểu đồ hỗn hợp: một series cột và nhiều series đường trên cùng trục phân loại],
) <fig-demo-line-bar-mix>

#strong[Tương tác zoom và trỏ xem chi tiết.] @fig-demo-line-zoom là trạng thái sau khi phóng to sâu theo trục X: nét vẫn sắc nhờ khử răng cưa SDF (Chương 5), còn trục Y đã tự co giãn theo biên độ các điểm trong khung nhìn. @fig-demo-line-hover minh họa hộp thông tin (tooltip) bám theo điểm khi trỏ chuột.

#figure(
  image("/assets/images/demo-line-zoom.png", width: 100%),
  caption: [Biểu đồ đường sau khi phóng to sâu theo trục X - nét vẫn sắc nhờ khử răng cưa SDF],
) <fig-demo-line-zoom>

#figure(
  image("/assets/images/demo-line-hover.png", width: 100%),
  caption: [Hộp thông tin (tooltip) bám theo điểm khi trỏ chuột],
) <fig-demo-line-hover>

#strong[Dòng dữ liệu thời gian thực.] @fig-demo-btc là biểu đồ đường cập nhật liên tục từ luồng WebSocket của sàn Binance, kiểm chứng chế độ online với cửa sổ trượt.

#figure(
  image("/assets/images/demo-btc.png", width: 100%),
  caption: [Biểu đồ đường thời gian thực từ luồng WebSocket Binance],
) <fig-demo-btc>

== Đối chiếu kết quả với mục tiêu

@tbl-objectives đối chiếu từng mục tiêu đề ra ban đầu với trạng thái đạt được và bằng chứng tương ứng, khép lại vòng với Chương 1.

#figure(
  table(
    columns: (40%, 16%, 44%),
    align: (left + horizon, center + horizon, left + horizon),
    inset: (top: 0.55em, bottom: 0.55em, left: 0.5em, right: 0.5em),
    table.header(
      table.cell(align: center)[#strong[Mục tiêu]],
      table.cell(align: center)[#strong[Trạng thái]],
      table.cell(align: center)[#strong[Bằng chứng]],
    ),
    [API hai chiều C++ #sym.arrow.l.r QML để thiết lập và vẽ],
    [Đạt],
    [`ChartView`, `GeneralConfig`, `DataSource` qua Meta-Object System (Chương 3).],

    [Hỗ trợ ba loại biểu đồ Line\/Bar\/Pie (chọn khai báo trong QML)],
    [Đạt (điều chỉnh)],
    [Bốn tầng series\/strategy\/factory\/renderer song song; loại biểu đồ #strong[chọn khi khai báo], không đổi lúc chạy; mỗi loại có ví dụ riêng.],

    [Tùy biến màu qua giao diện], [Đạt], [Thuộc tính màu của series, bảng thiết lập.],
    [Kiểu nét đường: liền, đứt, chấm], [Đạt], [Rải phần tử theo độ dài cung, tái dùng hình học capsule (Chương 5).],
    [Offline: quét một lần, tự tính viewport], [Đạt], [Đường CSV nhanh, tự co giãn X\/Y ban đầu.],
    [Online: dòng thời gian thực], [Đạt], [`RealTimeChart`, `BinanceChart` (WebSocket).],
    [Zoom linh hoạt theo phương],
    [Đạt],
    [Zoom theo trục X quanh con trỏ; trục Y #strong[tự co giãn] theo điểm trong khung nhìn thay cho zoom Y\/cả hai phương thủ công.],

    [Kéo trượt (drag)], [Đạt], [Pan dọc trục X có kẹp biên, bám mép động khi dữ liệu mới về.],
    [Trỏ xem chi tiết (pointing\/hover)], [Đạt], [Dò trúng điểm đã vẽ trong bán kính 24 px, `ChartTooltip`.],
    [Giới hạn dữ liệu, tự loại bỏ dữ liệu cũ],
    [Đạt (điều chỉnh)],
    [`maxCacheRows` giới hạn cửa sổ trượt; vượt ngưỡng thì loại nguyên khối dòng cũ nhất (`trimFront`), chi phí $O$(số dòng bị loại). Không có thao tác xóa toàn bộ thủ công.],

    [Cắt chọn vùng (crop)], [Ngoài phạm vi], [Không hiện thực; nhu cầu khảo sát vùng đã được zoom X + pan đáp ứng.],
    [Downsampling xử lý triệu điểm không giảm FPS],
    [Đạt],
    [Kim tự tháp LOD (mặc định) + LTTB; chi phí $O(P)$ độc lập $N$ (Chương 4). \~50 FPS trên 20 series \/ 10 triệu điểm, bộ nhớ phẳng (@tbl-stress).],

    [Khử răng cưa SDF tự viết trong shader],
    [Đạt],
    [`sdSegment` + `smoothstep`\/`fwidth`, độc lập độ phân giải (Chương 5).],

    [Đóng gói thành plugin động tự chứa], [Đạt], [`libChartPlotter.so`, phụ thuộc header-only nhúng sẵn.],
  ),
  caption: [Đối chiếu mục tiêu với trạng thái đạt được],
) <tbl-objectives>

Bốn điểm cần nói rõ để bảo đảm tính trung thực của báo cáo. Thứ nhất, mục tiêu ban đầu nêu "zoom theo phương ngang, phương dọc hoặc cả hai": trong quá trình hiện thực, mô hình tương tác được #strong[điều chỉnh] sang zoom trục X kết hợp trục Y #strong[tự co giãn] theo biên độ các điểm trong khung nhìn. Lựa chọn này phù hợp hơn với dữ liệu telemetry\/chuỗi thời gian - nơi trục hoành là thời gian và người dùng luôn muốn thấy trọn biên độ hiện tại - đồng thời loại bỏ thao tác thừa. Thứ hai, tính năng #strong[cắt chọn vùng (crop)] không được hiện thực; nhu cầu khoanh vùng khảo sát đã được đáp ứng đầy đủ bằng zoom trục X kết hợp kéo trượt. Thứ ba, mục tiêu ban đầu nêu "chuyển đổi linh hoạt giữa ba loại biểu đồ": thực tế loại biểu đồ được #strong[chọn khi khai báo series trong QML] chứ không có thao tác đổi loại lúc đang chạy, và cùng một tập dữ liệu cũng không biểu diễn được cả ba loại (biểu đồ tròn cần một cột giá trị, còn đường và cột cần hai cột $x$, $y$) - nên mỗi loại có ví dụ riêng thay vì một demo chuyển qua lại. Thứ tư, mục tiêu ban đầu nêu "xóa dữ liệu (clear)": sản phẩm #strong[không] cung cấp thao tác xóa toàn bộ thủ công, mà thay bằng cơ chế #strong[giới hạn dữ liệu] qua `maxCacheRows` - khi số dòng vượt ngưỡng, các dòng cũ nhất tự bị loại nguyên khối theo cửa sổ trượt; đây chính là nhu cầu cốt lõi ở chế độ online (giữ bộ nhớ hữu hạn khi chạy dài ngày). Các mục tiêu còn lại - đặc biệt hai mục tiêu hiệu năng và kiến trúc trọng tâm là downsampling và khử răng cưa SDF - đều đạt và có bằng chứng kỹ thuật xuyên suốt Chương 4 và Chương 5.
