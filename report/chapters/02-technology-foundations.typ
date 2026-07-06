= Cơ sở công nghệ và kỹ thuật nền tảng

Chương này trang bị nền tảng công nghệ và lý thuyết vừa đủ để theo dõi các thiết kế và kỹ thuật ở những chương sau. Nội dung không nhằm trình bày lại toàn bộ một giáo trình, mà tập trung vào những khái niệm mà sản phẩm trực tiếp dựa vào. Chương gồm các nội dung chính sau:
- Qt, QML và hệ thống Meta-Object.
- Cây kết xuất Qt Quick Scene Graph và cơ chế tích hợp OpenGL.
- OpenGL 3.3 Core và pipeline đổ bóng GLSL.
- Các mẫu thiết kế được áp dụng.
- Nền tảng thuật toán trực quan hóa: giảm mẫu, kim tự tháp mức chi tiết và trường khoảng cách có dấu.

== Qt, QML và hệ thống Meta-Object

Qt là một framework C++ đa nền tảng cung cấp đầy đủ hạ tầng để xây dựng ứng dụng có giao diện đồ họa, từ quản lý cửa sổ, sự kiện chuột/bàn phím cho tới đồ họa và mạng. Trong Qt, tầng giao diện người dùng (UI) hiện đại được mô tả bằng #strong[QML] - một ngôn ngữ khai báo (declarative) cho phép dựng cây thành phần giao diện một cách ngắn gọn, tách bạch với phần logic viết bằng C++. Sự phân tách này chính là mô hình mà đề tài áp dụng: QML đóng vai trò frontend (khai báo biểu đồ, thuộc tính, thành phần giao diện), còn lõi C++ đảm nhiệm toàn bộ việc quản lý dữ liệu, tính toán và kết xuất.

Cầu nối giữa hai tầng là #strong[hệ thống Meta-Object] của Qt. Thông qua một bước tiền xử lý bằng trình biên dịch Meta-Object (MOC), mỗi lớp C++ kế thừa `QObject` được bổ sung khả năng nội quan (introspection) tại thời điểm chạy: hệ thống biết được danh sách thuộc tính, phương thức và tín hiệu của lớp. Trên nền đó, Qt cung cấp ba cơ chế then chốt mà sản phẩm khai thác:

- #strong[Thuộc tính (`Q_PROPERTY`)]: phơi bày một biến C++ ra QML để đọc/ghi và tự động cập nhật giao diện khi giá trị đổi. Các tham số như màu sắc, kiểu nét, độ rộng đường của biểu đồ đều là thuộc tính kiểu này.
- #strong[Phương thức khả gọi (`Q_INVOKABLE`) và khe (slot)]: cho phép QML gọi trực tiếp hàm C++, ví dụ để áp dụng cấu hình hay nạp dữ liệu.
- #strong[Tín hiệu và khe (signal/slot)]: cơ chế truyền thông điệp lỏng lẻo (loose coupling) giữa các đối tượng, là nền tảng của mô hình hướng sự kiện (xem mẫu Observer ở mục 2.4).

#figure(
  image("/assets/images/cpp-qml-communication-through-meta-object.png", width: 100%, height: auto),
  caption: [Sơ đồ giao tiếp hai chiều C++ $arrow.l.r$ QML qua hệ thống Meta-Object],
)

Việc đăng ký một lớp C++ thành một kiểu dùng được trong QML biến `ChartView` cùng các lớp cấu hình, nguồn dữ liệu thành các thành phần khai báo được. Cuối cùng, toàn bộ lõi được đóng gói thành một #strong[plugin động] (thư viện `.so`): ứng dụng chỉ cần khai báo `import` module tương ứng là có thể sử dụng, không phải biên dịch lại lõi. Đây là cơ sở kỹ thuật cho tính "dễ tích hợp" và "tự chứa" đã nêu ở Chương 1.

== Cây kết xuất Qt Quick Scene Graph và tích hợp OpenGL

Khác với mô hình vẽ tức thời (immediate mode) truyền thống - nơi mỗi khung hình gọi lại toàn bộ lệnh vẽ trên CPU - Qt Quick sử dụng một #strong[cây cảnh (scene graph)] được lưu giữ (retained mode). Giao diện được biểu diễn thành một cây các nút `QSGNode`; cây này được kết xuất trên một luồng kết xuất riêng, tận dụng GPU và gom các lệnh vẽ tương tự thành lô (batching) để giảm số lần gọi đồ họa. Nhờ giữ lại cấu trúc giữa các khung hình, khi chỉ một phần nhỏ thay đổi thì chỉ phần đó được cập nhật, thay vì vẽ lại tất cả.

Một `QQuickItem` tham gia vào scene graph bằng cách cài đặt phương thức `updatePaintNode`, nơi nó dựng hoặc cập nhật cây nút con của mình. Điểm mấu chốt mà đề tài dựa vào là lớp #strong[`QSGRenderNode`]: nó cho phép chèn các lệnh #strong[OpenGL thô] vào đúng vị trí trong quá trình duyệt scene graph. Nhờ đó, sản phẩm không bị giới hạn bởi các nút vẽ dựng sẵn của Qt, mà tự tay phát các lệnh vẽ hình học biểu đồ xuống GPU - vừa tận dụng được hạ tầng luồng kết xuất và quản lý trạng thái của Qt Quick, vừa toàn quyền kiểm soát cách vẽ. Cây scene graph của một biểu đồ vì thế gồm một nút kết xuất tùy biến bao bọc các lệnh OpenGL cho đường, cột, trục,...

#figure(
  image("/assets/images/scene-graph.drawio.png", width: 100%, height: auto),
  caption: [Sơ đồ cây scene graph],
)

== OpenGL 3.3 Core và pipeline đổ bóng GLSL

#strong[OpenGL] (Open Graphics Library) là một API đồ họa tiêu chuẩn để lập trình GPU. Sản phẩm yêu cầu hồ sơ (profile) #strong[Core 3.3], một mốc đủ hiện đại để dùng pipeline lập trình được (programmable pipeline) nhưng vẫn tương thích rộng với phần cứng phổ thông và thiết bị nhúng.

Trong pipeline này, dữ liệu hình học đi qua hai giai đoạn đổ bóng lập trình được, viết bằng ngôn ngữ #strong[GLSL] (OpenGL Shading Language):

+ #strong[Vertex shader]: chạy một lần cho mỗi đỉnh, biến đổi tọa độ đỉnh (thường bằng một ma trận MVP - Model-View-Projection) từ không gian dữ liệu sang không gian màn hình/clip.
+ #strong[Giai đoạn rasterization]: phần cứng nội suy các thuộc tính đỉnh và sinh ra các mảnh (fragment) tương ứng với từng điểm ảnh mà hình phủ lên.
+ #strong[Fragment shader]: chạy một lần cho mỗi mảnh, quyết định màu và độ trong suốt cuối cùng của điểm ảnh.

Dữ liệu đỉnh được nạp lên GPU qua các đối tượng bộ đệm đỉnh (VBO - Vertex Buffer Object) và mô tả bố cục bằng đối tượng mảng đỉnh (VAO - Vertex Array Object); các tham số toàn cục dùng chung cho một lệnh vẽ (màu, độ rộng, ma trận, vùng vẽ) được truyền qua biến `uniform`. Chi phí xử lý nằm chủ yếu ở GPU và tỉ lệ với số điểm ảnh được tô, không phải số điểm dữ liệu - đây là lý do đẩy hình học xuống GPU giúp giữ FPS cao ngay cả với dữ liệu lớn.

Fragment shader lập trình được cũng chính là nơi sản phẩm thực hiện khử răng cưa tự viết: thay vì dựa vào khử răng cưa của phần cứng, mỗi mảnh tự tính độ phủ theo khoảng cách hình học (kỹ thuật trình bày ở mục 2.5 và Chương 5). Ngoài ra, kết quả màu được kết hợp (blending) với nền theo chế độ alpha nhân trước (premultiplied alpha) để tránh viền tối ở mép khử răng cưa.

#figure(
  image("/assets/images/opengl-pipeline.png", width: 100%, height: auto),
  caption: [Sơ đồ pipeline OpenGL],
)

== Các mẫu thiết kế được áp dụng

Để lõi C++ vừa hiệu năng cao vừa dễ mở rộng thêm loại biểu đồ mới, kiến trúc áp dụng một số mẫu thiết kế kinh điển@gamma_design_patterns_1994. Ba mẫu đóng vai trò xương sống:

- #strong[Strategy (Chiến lược).] Mẫu này đóng gói một họ thuật toán có thể hoán đổi cho nhau sau một giao diện chung. Ở đây, mỗi loại biểu đồ có một chiến lược riêng (`ISeriesStrategy`) chịu trách nhiệm biến dữ liệu đã phân giải cùng viewport hiện tại thành dữ liệu kết xuất. Nhờ vậy, `ChartView` không cần biết chi tiết từng loại biểu đồ; việc thêm một cách dựng hình mới chỉ là bổ sung một chiến lược.

- #strong[Factory (Nhà máy).] Mẫu này tách việc tạo đối tượng khỏi nơi sử dụng, để mã gọi không phụ thuộc vào lớp cụ thể. Một nhà máy thành phần (`ISeriesComponentFactory`) chịu trách nhiệm khớp một series với đúng chiến lược và bộ kết xuất tương ứng; một bộ tra cứu chọn nhà máy phù hợp theo loại series. Đây là cơ chế giữ cho bốn tầng mở rộng (series, strategy, factory, renderer) luôn đồng bộ.

- #strong[Observer (Quan sát).] Mẫu này thiết lập quan hệ một-nhiều, trong đó đối tượng bị quan sát thay đổi thì các đối tượng quan sát được thông báo. Trong Qt, cơ chế signal/slot chính là hiện thực tự nhiên của mẫu này. Sản phẩm dùng nó cho luồng dữ liệu: khi có dữ liệu mới, tầng quản lý dữ liệu phát tín hiệu "có ảnh chụp dữ liệu sẵn sàng" và `ChartView` phản ứng để dựng lại biểu đồ, nhờ đó frontend và backend rời rạc (decoupled) với nhau.

#figure(
  image("/assets/images/series-strat-factory-renderer-relation.drawio.png", width: 100%, height: auto),
  caption: [Sơ đồ quan hệ giữa series, strategy, factory và renderer],
)

== Nền tảng thuật toán trực quan hóa

Ba nhóm kỹ thuật thuật toán và đồ họa dưới đây là cốt lõi tạo nên hiệu năng và chất lượng hiển thị của sản phẩm. Mục này chỉ giới thiệu khái niệm và ý tưởng; cơ chế chi tiết cùng phân tích độ phức tạp được trình bày ở Chương 4 và Chương 5.

=== Giảm mẫu giữ hình dạng

Khi số điểm dữ liệu vượt xa số điểm ảnh, việc vẽ hết mọi điểm vừa lãng phí vừa không cải thiện hình ảnh. #strong[Giảm mẫu (downsampling)] là kỹ thuật chọn ra một tập con điểm nhỏ hơn nhiều nhưng vẫn giữ được hình dạng trực quan của tín hiệu. Một thuật toán tiêu biểu là #strong[LTTB] (Largest Triangle Three Buckets)@steinarsson_downsampling_2013: chia dữ liệu thành các "xô" và từ mỗi xô chọn điểm tạo thành tam giác có diện tích lớn nhất với điểm đã chọn trước đó và điểm đại diện của xô kế tiếp - qua đó ưu tiên giữ lại những điểm mà đường gấp khúc đổi hướng mạnh, tức các đặc trưng mà mắt người dễ nhận ra. Ý tưởng này được sản phẩm mở rộng và kết hợp với cấu trúc kim tự tháp mức chi tiết ở phần sau.

=== Kim tự tháp mức chi tiết (mipmap)

Ý tưởng #strong[kim tự tháp mức chi tiết (LOD)] bắt nguồn từ kỹ thuật #strong[mipmap] trong đồ họa@williams_pyramidal_1983: chuẩn bị trước nhiều phiên bản dữ liệu ở các độ phân giải giảm dần, để khi hiển thị chỉ cần đọc mức phù hợp với mật độ điểm ảnh thay vì xử lý toàn bộ dữ liệu gốc. Áp dụng vào biểu đồ, sản phẩm dựng một kim tự tháp các "xô" tóm tắt (lưu giá trị nhỏ nhất và lớn nhất của từng nhóm điểm liên tiếp) trên chuỗi điểm đã sắp theo trục X. Khi phóng to/thu nhỏ, hệ thống chỉ đọc số xô cỡ số điểm ảnh theo phương ngang, biến chi phí mỗi khung hình gần như độc lập với tổng số điểm.

=== Trường khoảng cách có dấu (SDF)

#strong[Trường khoảng cách có dấu (SDF)] là một hàm cho biết, tại mỗi điểm trong mặt phẳng, khoảng cách ngắn nhất tới biên của một hình - mang dấu âm nếu nằm trong hình và dương nếu nằm ngoài@quilez_distfunctions2d. Trong đồ họa 2D, nhiều hình cơ bản (đoạn thẳng, đường tròn) có công thức SDF gọn và tính được trực tiếp trong fragment shader. Khi biết khoảng cách của một điểm ảnh tới nét vẽ, ta có thể tính chính xác độ phủ của nét lên điểm ảnh đó và tạo ra một dải chuyển tiếp mượt ở mép - tức #strong[khử răng cưa giải tích], không phụ thuộc vào khử răng cưa đa mẫu của phần cứng và giữ độ sắc nét ở mọi mức phóng to. Đây là nền tảng cho kỹ thuật vẽ nét của sản phẩm ở Chương 5.
