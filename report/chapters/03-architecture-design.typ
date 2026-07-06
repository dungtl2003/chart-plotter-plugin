= Phân tích yêu cầu và thiết kế kiến trúc tổng thể hệ thống

Chương này chuyển từ bối cảnh và nền tảng sang bản thiết kế của sản phẩm. Trước hết là phân tích yêu cầu chức năng và phi chức năng; tiếp đó là kiến trúc phân lớp cùng luồng dữ liệu một chiều xuyên suốt hệ thống, bộ lập kế hoạch bố cục quyết định hệ tọa độ và kiểm tra hợp lệ, mô hình đa luồng bảo đảm giao diện luôn mượt, điểm nối mở rộng cho các loại biểu đồ, và cuối cùng là giao diện lập trình phía QML. Chương gồm các nội dung chính sau:
- Yêu cầu hệ thống.
- Kiến trúc phân lớp và luồng dữ liệu một chiều.
- Bộ lập kế hoạch bố cục và kiểm tra hợp lệ.
- Mô hình đa luồng.
- Điểm nối mở rộng: series, strategy, factory, renderer.
- Giao diện lập trình phía QML và các thành phần giao diện.

== Yêu cầu hệ thống

=== Đối tượng sử dụng

Sản phẩm là một thư viện được nhúng vào ứng dụng Qt, nên có hai nhóm đối tượng: #strong[lập trình viên tích hợp] - người khai báo biểu đồ trong QML và cấp nguồn dữ liệu; và #strong[người dùng cuối] - người tương tác trực tiếp với biểu đồ đã dựng (phóng to, kéo trượt, xem chi tiết). Các yêu cầu dưới đây phục vụ cả hai nhóm.

=== Yêu cầu chức năng

Bảng #ref(<tbl-fr>, supplement: none) liệt kê các yêu cầu chức năng, tổng hợp từ mục tiêu đề tài ở Chương 1.

#figure(
  table(
    columns: (10%, 32%, 58%),
    align: (center + horizon, left + horizon, left + horizon),
    inset: (top: 0.6em, bottom: 0.6em, left: 0.5em, right: 0.5em),
    table.header(
      table.cell(align: center)[#strong[Mã]],
      table.cell(align: center)[#strong[Yêu cầu]],
      table.cell(align: center)[#strong[Mô tả]],
    ),
    [FR1], [Vẽ ba loại biểu đồ], [Hỗ trợ ba loại biểu đồ Line, Bar, Pie; chọn loại theo cách khai báo trong QML.],

    [FR2],
    [Tùy biến hình thức],
    [Tùy chỉnh màu sắc; đặt kiểu nét cho biểu đồ đường: liền (Solid), đứt (Dash), chấm (Dot).],

    [FR3],
    [Nạp dữ liệu offline],
    [Đọc trọn tập dữ liệu tĩnh, tự tính viewport/tỉ lệ (min-max trục X, Y) và kết xuất trong một lượt.],

    [FR4], [Nạp dữ liệu online], [Xử lý và vẽ dòng dữ liệu thời gian thực chảy về liên tục.],
    [FR5],
    [Phóng to/thu nhỏ],
    [Zoom theo trục X quanh vị trí con trỏ; trục Y tự co giãn theo biên độ của các điểm nằm trong khung nhìn hiện tại.],

    [FR6],
    [Tương tác trực tiếp],
    [Kéo trượt (drag) khung nhìn dọc trục X; trỏ xem chi tiết (hover); giới hạn dữ liệu theo cửa sổ trượt, tự loại bỏ dữ liệu cũ nhất khi vượt ngưỡng.],

    [FR7], [Giao diện lập trình hai chiều], [Cung cấp API C++ <-> QML để thiết lập, cấu hình và điều khiển biểu đồ.],
  ),
  caption: "Danh mục yêu cầu chức năng",
) <tbl-fr>

=== Yêu cầu phi chức năng

Bảng #ref(<tbl-nfr>, supplement: none) liệt kê các yêu cầu phi chức năng - phần lớn bắt nguồn từ bốn thách thức kỹ thuật ở Chương 1 và định hình các quyết định thiết kế của những chương sau.

#figure(
  table(
    columns: (10%, 32%, 58%),
    align: (center + horizon, left + horizon, left + horizon),
    inset: (top: 0.6em, bottom: 0.6em, left: 0.5em, right: 0.5em),
    table.header(
      table.cell(align: center)[#strong[Mã]],
      table.cell(align: center)[#strong[Yêu cầu]],
      table.cell(align: center)[#strong[Mô tả]],
    ),
    [NFR1], [Hiệu năng], [Hiển thị và tương tác mượt (FPS cao) với tập dữ liệu tới hàng triệu điểm.],
    [NFR2], [Bộ nhớ có trần], [Ở chế độ online, giới hạn bộ nhớ qua cửa sổ trượt để chạy dài ngày không phình.],
    [NFR3], [Độ trễ nạp thấp], [Nạp nhanh các tập dữ liệu lớn; không chặn giao diện trong khi nạp.],
    [NFR4],
    [Tính đóng gói và di động],
    [Đóng gói thành một plugin động tự chứa, dễ nhúng, không để lại dấu vết thư viện ngoài.],

    [NFR5], [Khả năng mở rộng], [Thêm loại biểu đồ mới mà không sửa đổi lõi hiện có.],
    [NFR6], [Chất lượng hiển thị], [Nét vẽ mượt, khử răng cưa, sắc nét ở mọi mức phóng to và trên mọi thiết bị.],
  ),
  caption: "Danh mục yêu cầu phi chức năng",
) <tbl-nfr>

== Kiến trúc phân lớp và luồng dữ liệu một chiều

Toàn bộ hệ thống được tổ chức thành hai lớp lớn giao tiếp qua hệ thống Meta-Object: #strong[lớp frontend] khai báo bằng QML (biểu đồ, thuộc tính, thành phần giao diện) và #strong[lớp backend] viết bằng C++ (quản lý dữ liệu, phân giải series, dựng dữ liệu kết xuất, bố cục, viewport và kết xuất OpenGL). Nguyên tắc thiết kế bao trùm là #strong[luồng dữ liệu một chiều]: dữ liệu chảy từ nguồn tới màn hình theo một hướng duy nhất, không có vòng phản hồi ngược làm rối trạng thái. Đường đi này được minh họa ở #ref(<fig-data-pipeline>, supplement: none) và mô tả chi tiết dưới đây.

#figure(
  image("/assets/images/data-pipeline.drawio.png", width: 100%, height: auto),
  caption: [Sơ đồ luồng dữ liệu một chiều],
) <fig-data-pipeline>

Các chặng chính của đường ống:

+ #strong[Nguồn dữ liệu (`DataSource`)]: một đối tượng QML trỏ tới một `url` và một định dạng (CSV, JSON qua WebSocket,...).
+ #strong[Tầng quản lý dữ liệu (`DataManager` / `DataManagerPool`)]: mỗi nguồn có một `DataManager` điều phối bộ đọc (reader) và bộ phân tích (parser), ghi vào bộ đệm dữ liệu dạng cột (`DataBuffer`).
+ #strong[Ảnh chụp (snapshot)]: bộ đệm sinh ra các ảnh chụp bất biến và phát lên frontend.
+ #strong[`ChartView`]: nhận ảnh chụp, phân giải series (`SeriesDataResolver`) để xác định cột, kiểu dữ liệu và phạm vi trục dùng chung.
+ #strong[Kế hoạch bố cục (`ChartLayoutPlanner`)]: kiểm tra tính hợp lệ của tập series, chọn hệ tọa độ và tính lề vùng vẽ trước khi dựng dữ liệu - trình bày ở mục kế tiếp.
+ #strong[Chiến lược (`ISeriesStrategy`)]: dựng dữ liệu kết xuất (`RenderData`) từ series đã phân giải và viewport hiện tại - đây là nơi tiêu thụ bộ nhớ đệm điểm và bộ giảm mẫu (Chương 4).
+ #strong[Kết xuất]: gói kết xuất (`ChartRenderPackage`) được đưa vào nút scene graph (`ChartRenderNode`), nơi các bộ kết xuất OpenGL (đường, cột, trục) phát lệnh vẽ xuống GPU (Chương 5).

Một lựa chọn thiết kế then chốt: #strong[dữ liệu được quy về kiểu `double` càng sớm càng tốt] ngay tại tầng ghi bộ đệm. Chuỗi ký tự và ngày tháng được ánh xạ thành mã số/mốc thời gian (epoch-ms), nhờ đó toàn bộ phần phía sau - tính biên, giảm mẫu, bộ nhớ đệm điểm, kết xuất - làm việc trên cùng một kiểu số đồng nhất, đơn giản và nhanh. `ChartView` đóng vai trò nhạc trưởng: nó sở hữu bể dữ liệu, bộ điều khiển viewport, các chiến lược, kế hoạch bố cục, mô hình chú giải và bộ nhớ đệm điểm toàn cục.

== Bộ lập kế hoạch bố cục và kiểm tra hợp lệ

Giữa bước `ChartView` nhận tập series và bước chiến lược dựng dữ liệu kết xuất có một chặng trung gian: #strong[bộ lập kế hoạch bố cục (`ChartLayoutPlanner`)]. Mỗi khi tập series thay đổi (thêm hoặc bớt series lúc khai báo), `ChartView` gọi `buildPlan` trên luồng giao diện để sinh một #strong[kế hoạch bố cục (`ChartLayoutPlan`)]. Kế hoạch này trả lời ba câu hỏi #strong[trước khi] bất kỳ dữ liệu nào được dựng để vẽ:

+ #strong[Tập series có hợp lệ không?] Bộ lập kế hoạch chạy tuần tự một chuỗi #strong[chính sách (policy)] - mỗi chính sách là một luật kiểm tra độc lập, cắm thêm hoặc gỡ bỏ mà không đụng phần còn lại. Chỉ cần một chính sách bác bỏ, kế hoạch bị đánh dấu #strong[không hợp lệ] kèm thông điệp lỗi, và `ChartView` bỏ qua lần dựng đó (ghi cảnh báo) thay vì vẽ ra một biểu đồ vô nghĩa. Các luật hiện hành tóm tắt ở #ref(<tbl-policies>, supplement: none).
+ #strong[Dùng hệ tọa độ nào?] Bộ lập kế hoạch phân loại từng series qua `qobject_cast`: series thuộc nhánh `XYSeries` (đường, cột) dùng hệ #strong[Cartesian], series `PieSeries` dùng hệ #strong[Pie]; series không nhận dạng được làm kế hoạch không hợp lệ. Hệ tọa độ chung của biểu đồ được suy ra từ đó và quyết định nhánh dựng dữ liệu về sau (XY hay Pie) cũng như cách vẽ trục.
+ #strong[Chừa lề bao nhiêu?] Kế hoạch tính #strong[lề vùng vẽ (plot margins)] để dành chỗ cho tiêu đề, chú giải và trục: xuất phát từ lề mặc định đều bốn cạnh rồi thu hẹp cạnh tương ứng khi có tiêu đề hoặc chú giải ở phía đó. Đây vẫn là các #strong[hằng số gán cứng (magic number)] - một hạn chế đã ghi nhận ở phần kết luận, dự kiến thay bằng cách đo theo kích thước thật của từng thành phần.

#figure(
  table(
    columns: (36%, 64%),
    align: (left + horizon, left + horizon),
    inset: (top: 0.6em, bottom: 0.6em, left: 0.5em, right: 0.5em),
    table.header(
      table.cell(align: center)[#strong[Chính sách]],
      table.cell(align: center)[#strong[Luật]],
    ),
    [Không rỗng], [`ChartView` phải có ít nhất một series.],
    [Không trộn XY với Pie],
    [Series đường/cột và series tròn không được ở chung một biểu đồ, vì tròn chuẩn hóa theo tổng và thuộc hệ tọa độ khác.],

    [Tối đa một series cột], [Nhiều series cột trên cùng một biểu đồ chưa được hỗ trợ.],
    [Tối đa một series tròn], [Mỗi biểu đồ chỉ một series tròn.],
  ),
  caption: "Các chính sách kiểm tra hợp lệ của bộ lập kế hoạch bố cục",
) <tbl-policies>

Đáng chú ý, danh sách luật #strong[không] cấm trộn đường với cột: nhiều series đường cùng một series cột #strong[được phép] chung một biểu đồ trên trục hạng mục dùng chung (ví dụ `MixedChart`). Việc gom kiểm tra hợp lệ vào một chuỗi chính sách tách rời giúp mỗi luật ngắn gọn, kiểm thử độc lập, và khi thêm loại biểu đồ mới chỉ cần bổ sung chính sách tương ứng mà không sửa phần lõi - cùng tinh thần với điểm nối mở rộng ở mục kế tiếp.

== Mô hình đa luồng

Để thỏa mãn yêu cầu "không chặn giao diện khi nạp" (NFR3) và giữ FPS ổn định, hệ thống phân tách công việc thành bốn vai trò luồng, tóm tắt ở Bảng #ref(<tbl-threads>, supplement: none).

#figure(
  table(
    columns: (30%, 70%),
    align: (left + horizon, left + horizon),
    inset: (top: 0.6em, bottom: 0.6em, left: 0.5em, right: 0.5em),
    table.header(
      table.cell(align: center)[#strong[Vai trò luồng]],
      table.cell(align: center)[#strong[Nhiệm vụ]],
    ),
    [Luồng giao diện (GUI)],
    [Vòng lặp sự kiện chính của Qt: xử lý thao tác chuột/lăn, lập kế hoạch bố cục, dựng gói kết xuất và điều phối cập nhật. Toàn hệ thống chỉ một luồng này.],

    [Luồng kết xuất scene graph],
    [Chạy `updatePaintNode` và phát lệnh OpenGL xuống GPU dưới vòng lặp kết xuất phân luồng của Qt Quick, song song với luồng giao diện.],

    [Luồng xử lý dữ liệu],
    [Phân tích byte thành hàng/lô, quy về `double`, ghi vào bộ đệm và dựng ảnh chụp. Mỗi `DataManager` (mỗi nguồn) có một luồng xử lý riêng.],

    [Luồng đọc (reader)],
    [Nhập/xuất (I/O) chặn hoặc theo dòng: đọc byte thô từ tệp, socket hoặc WebSocket. Mỗi `DataManager` có một luồng đọc riêng.],
  ),
  caption: "Bốn vai trò luồng trong hệ thống",
) <tbl-threads>

Như vậy mỗi nguồn dữ liệu đóng góp hai luồng riêng (đọc và xử lý), còn luồng giao diện và luồng kết xuất là duy nhất, dùng chung cho cả biểu đồ.

Điểm tinh tế nằm ở #strong[cách nối các chặng]. Trong nhánh nạp, chỉ bước vượt luồng đầu tiên - từ luồng đọc sang luồng xử lý - dùng kết nối kiểu hàng đợi (`QueuedConnection`) để chuyển an toàn giữa hai luồng; toàn bộ chuỗi phía sau (phân tích, quy đổi, ghi bộ đệm, dựng ảnh chụp) dùng kết nối trực tiếp (`DirectConnection`), chạy đồng bộ trên cùng luồng xử lý. Cách bố trí này vừa tách được I/O tốn thời gian ra khỏi giao diện, vừa tránh chi phí điều phối không cần thiết trong chuỗi xử lý.

Việc bàn giao dữ liệu giữa luồng xử lý và luồng giao diện dựa trên #strong[ảnh chụp sao-chép-khi-ghi (copy-on-write)]: ảnh chụp chỉ sao chép các "tay cầm" tới khối dữ liệu bất biến chứ không sao chép dữ liệu thực, nên một bộ đệm hàng trăm MB được chụp trong thời gian cực ngắn và luồng giao diện đọc đúng khối dữ liệu mà luồng ghi tạo ra mà không tranh chấp. Còn khâu bàn giao từ luồng giao diện sang luồng kết xuất diễn ra trong pha đồng bộ scene graph của Qt Quick - lúc luồng giao diện tạm dừng - nên gói kết xuất được trao tay không cần khóa; riêng bộ đếm khung hình `frameSwapped` đi ngược từ luồng kết xuất về luồng giao diện qua một biến nguyên tử (atomic) nới lỏng. Cơ chế chặn ngược (backpressure) ở luồng đọc và các bất biến của ảnh chụp được phân tích chi tiết ở Chương 4.

#ref(<fig-threads>, supplement: none) tóm tắt bốn luồng theo dạng làn bơi cùng kiểu kết nối nối giữa chúng: các bước trong một luồng xếp theo chiều dọc, còn mũi tên cắt ngang các làn ghi rõ kiểu kết nối (`QueuedConnection` hay `DirectConnection`), điểm bàn giao ảnh chụp sao-chép-khi-ghi và đường phản hồi `frameSwapped`.

#figure(
  image("/assets/images/threads.drawio.png", width: 100%, height: auto),
  caption: [Bốn vai trò luồng và cách nối giữa chúng],
) <fig-threads>

== Điểm nối mở rộng: series, strategy, factory, renderer

Yêu cầu "thêm loại biểu đồ mà không sửa lõi" (NFR5) được hiện thực bằng #strong[bốn tầng phân cấp song song]. Mỗi loại biểu đồ xuất hiện đồng thời ở cả bốn tầng, và việc thêm một loại mới chỉ là bổ sung một lớp con ở mỗi tầng, như tóm tắt ở Bảng #ref(<tbl-seam>, supplement: none).

#figure(
  table(
    columns: (14%, 36%, 50%),
    align: (left + horizon, left + horizon, left + horizon),
    inset: (top: 0.6em, bottom: 0.6em, left: 0.5em, right: 0.5em),
    table.header(
      table.cell(align: center)[#strong[Tầng]],
      table.cell(align: center)[#strong[Giao diện cơ sở]],
      table.cell(align: center)[#strong[Vai trò]],
    ),
    [series], [`AbstractSeries`], [Phơi bày thuộc tính ra QML: cột dữ liệu, màu sắc, kiểu nét.],
    [strategy],
    [`ISeriesStrategy`],
    [Dựng dữ liệu kết xuất từ series đã phân giải và viewport; tiêu thụ bộ nhớ đệm điểm và bộ giảm mẫu.],

    [factory],
    [`ISeriesComponentFactory`],
    [Khớp một series với đúng chiến lược và bộ kết xuất; một bộ tra cứu chọn nhà máy theo loại series.],

    [renderer], [`IOpenGLRenderer`], [Phát lệnh OpenGL để vẽ (khởi tạo, vẽ, giải phóng, nạp dữ liệu).],
  ),
  caption: "Bốn tầng mở rộng song song khi thêm một loại biểu đồ",
) <tbl-seam>

Đây chính là hiện thực cụ thể của mẫu Strategy và Factory ở mục 2.4. Với một loại biểu đồ phi Cartesian như Pie (không có trục X-Y thông thường), ngoài bốn tầng trên còn cần bổ sung một hệ tọa độ tương ứng và các chính sách bố cục riêng trong bộ lập kế hoạch bố cục - nhưng phần lõi và các loại biểu đồ khác hoàn toàn không bị ảnh hưởng.

== Giao diện lập trình phía QML và các thành phần giao diện

Nhờ hệ thống Meta-Object, việc dựng một biểu đồ trở nên khai báo và ngắn gọn. `ChartView` khai báo thuộc tính mặc định là danh sách "nội dung", nên các series và nguồn dữ liệu chỉ cần đặt làm phần tử con của nó. Đoạn mã dưới minh họa cách dựng một biểu đồ đường từ tệp CSV:

```qml
ChartView {
    id: chart
    anchors.fill: parent

    DataSource {
        id: dataSrc
        url: "file:///path/to/data.csv"
        format: ChartEnums.DataFormat.Csv
        hasHeader: true
    }

    LineSeries {
        source: dataSrc
        xColumn: 0
        yColumn: 1
        name: "Alpha"
        strokePattern: ChartEnums.StrokePattern.Dash
    }

    generalConfig { antialiasing: 2; lineWidth: 3 }
    legendItem: Legend { model: chart.legendModel }
}
```

Xoay quanh `ChartView` là một tập thành phần giao diện đi kèm, đều đăng ký trong cùng module `ChartPlotter`:

- #strong[`GeneralConfig`]: nhóm tham số kết xuất toàn cục đặt được từ QML (độ rộng nét, mức khử răng cưa, số vạch chia, FPS,...).
- #strong[`ChartTitle`, `Legend`]: tiêu đề và chú giải, gắn vào biểu đồ qua các thuộc tính `titleItem`, `legendItem`.
- #strong[`ChartSettings`]: bảng thiết lập trực quan cho phép người dùng đổi màu sắc và kiểu nét ngay lúc chạy.
- #strong[`ChartZoomButton`]: nút hỗ trợ thao tác phóng to/thu nhỏ.
- #strong[`ChartTooltip`]: lớp phủ hiển thị chỉ báo và hộp thông tin khi trỏ chuột.

Riêng cơ chế trỏ xem chi tiết (hover), `ChartView` phơi bày một thuộc tính mô tả điểm đang được trỏ (tọa độ, tên series, nhãn trục,...); thành phần `ChartTooltip` chỉ cần ràng buộc vào thuộc tính đó để vẽ chỉ báo và hộp thông tin. Chi tiết thuật toán dò trúng điểm được trình bày ở Chương 5.
