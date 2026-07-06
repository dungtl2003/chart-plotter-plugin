= Kỹ thuật xử lý dữ liệu và tối ưu hiệu năng

Chương này đi sâu vào #strong[đường dữ liệu đi vào] của hệ thống - từ lúc byte thô được đọc lên tới lúc trở thành các điểm sẵn sàng vẽ. Đây là nơi tập trung phần lớn kỹ thuật tối ưu hiệu năng, và cũng là chỗ quyết định liệu sản phẩm có giữ được lời hứa "mượt ở quy mô hàng triệu điểm" hay không. Chương gồm các nội dung chính sau:
- Nạp dữ liệu tốc độ cao: luồng đọc riêng và điều tiết luồng bằng semaphore.
- Đường CSV nhanh và phân tích ngày tháng không cấp phát.
- Lưu trữ cột, ảnh chụp sao chép khi ghi và cửa sổ trượt.
- Tính biên tăng tiến và lỗi "đóng băng" khi loại bỏ dữ liệu.
- Bộ nhớ đệm điểm tăng tiến.
- Kim tự tháp mức chi tiết (LOD) - cấu trúc giảm mẫu lõi.
- Thuật toán giảm mẫu LTTB.
- Các bộ giảm mẫu dự phòng.

== Nạp dữ liệu tốc độ cao: luồng đọc riêng và điều tiết luồng

=== Ý tưởng

Một bộ đọc dữ liệu tốt phải đồng thời đạt ba mục tiêu tưởng chừng mâu thuẫn:

+ Không bao giờ chặn luồng giao diện.
+ Chồng lấn (overlap) thao tác nhập/xuất (I/O) với thao tác phân tích để tận dụng thời gian chờ đĩa/mạng.
+ Không để một ổ đĩa nhanh đọc vượt lên rồi nạp cả tệp vào bộ nhớ trong khi bộ phân tích chậm hơn còn đang xử lý.

Thiết kế giải quyết cả ba bằng bộ ba: #strong[luồng đọc riêng + đọc theo khối cố định (chunk) + hàng đợi chặn dựa trên semaphore đếm].

=== Luồng đọc riêng và đọc theo khối

Mỗi `DataManager` sở hữu một `QThread` riêng và chuyển bộ đọc sang luồng đó, nên toàn bộ vòng lặp đọc chạy tách biệt, chồng lấn được với chuỗi phân tích - quy đổi - ghi bộ đệm chạy trên luồng xử lý dữ liệu. Bộ đọc tệp lặp `while (!file.atEnd())`, mỗi vòng đọc một khối cố định (mặc định #strong[2 MB]) rồi chuyển đi. Khối cố định giúp giới hạn tập làm việc (working set) và cấp cho bộ phân tích những đơn vị công việc đều đặn, có kích thước tương đương nhau.

=== Điều tiết luồng bằng semaphore đếm

Điểm tinh tế nhất là cơ chế #strong[chặn ngược (backpressure)]. `AbstractDataReader` giữ một `QSemaphore`@qt_qsemaphore mà số giấy phép (permit) của nó #strong[chính là] số chỗ trống trong hàng đợi:

- Lúc cấu hình, hàng đợi được cấp `capacity` giấy phép. Bộ đọc tĩnh (tệp/FIFO) dùng độ sâu 4 khối; luồng trực tiếp (socket/websocket) truyền `0` để #strong[tắt] chặn ngược (không giới hạn), vì một bộ xử lý mạng không được phép bị chặn.
- Bộ đọc phát khối qua `emitChunk()`, hàm này #strong[giành một giấy phép trước khi phát] - nên khi đã có 4 khối phát mà chưa tiêu thụ, luồng đọc tự chặn tại đây thay vì chất đống khối lên hàng đợi sự kiện.
- Bên tiêu thụ, sau khi phân tích và ghi xong một khối, giải phóng #strong[đúng một] giấy phép, cho phép luồng đọc bị chặn sản xuất khối kế tiếp.

Nhờ vậy, bộ đọc luôn chạy trước bộ phân tích #strong[nhiều nhất là 4 khối], giới hạn bộ nhớ đang lưu chuyển ở mức xấp xỉ $4 times 2 "MB" approx 8 "MB"$, bất kể đĩa nhanh đến đâu hay bộ phân tích tụt lại bao xa.

#figure(
  image("/assets/images/semaphore-backpressure.drawio.png", width: 100%, height: auto),
  caption: [Sơ đồ điều tiết luồng bằng semaphore đếm],
) <fig-semaphore-backpressure>

=== Dừng hai pha: `requestStop()` và `stop()`

Quá trình tắt được tách chủ đích thành #strong[hai] phương thức với #strong[hai kiểu kết nối] khác nhau, vì một luồng đọc vừa cần được #strong[ngắt] vừa cần được #strong[dọn dẹp], mà hai nhu cầu này kéo về hai hướng ngược nhau:

- #strong[`requestStop()` - lệnh ngắt.] Chỉ làm một việc: đặt một cờ `std::atomic_bool`. Nó nối kiểu trực tiếp (`DirectConnection`) nên chạy #strong[đồng bộ ngay trên luồng gọi], bất kể luồng đọc đang làm gì. Việc chỉ chạm vào một cờ nguyên tử phi khóa (lock-free) là điều khiến lời gọi xuyên luồng này an toàn. Vòng lặp đọc kiểm tra cờ mỗi vòng, còn `emitChunk` thăm dò semaphore bằng `tryAcquire` có thời hạn (\~50 ms) thay vì chặn vô hạn - nên một bộ đọc đang bị chặn ở hàng đợi đầy vẫn thấy cờ trong khoảng 50 ms rồi thoát.
- #strong[`stop()` - dọn dẹp gắn với luồng.] Nó nối kiểu hàng đợi (`QueuedConnection`) nên chạy #strong[trên chính luồng đọc], qua vòng lặp sự kiện của luồng đó. Đây là nơi các tài nguyên có #strong[tính liên kết luồng (thread affinity)] như `QTcpSocket`, `QWebSocket` #strong[bắt buộc] phải đóng, vì một `QObject` chỉ được chạm từ đúng luồng nó thuộc về.

Nếu #strong[chỉ giữ cờ nguyên tử], các bộ đọc socket/websocket không có vòng lặp chặn để phá và không có chỗ đóng socket an toàn, dẫn tới rò rỉ kết nối và hành vi không xác định; nếu #strong[chỉ giữ `stop()` hàng đợi], khi luồng đang chặn trong `file.read()` hay chờ semaphore thì nó #strong[không] quay vòng lặp sự kiện, nên lời gọi hàng đợi nằm mãi trong hàng đợi và hàm hủy `quit()`/`wait()` chờ vô hạn - #strong[bế tắc (deadlock)]. Chỉ cờ nguyên tử nối trực tiếp mới chạm được luồng đang bị chặn ngay lập tức, và chỉ khe hàng đợi mới là chỗ dọn dẹp gắn-luồng chạy an toàn.

== Đường CSV tốc độ cao và phân tích ngày tháng

=== Đường CSV nhanh: quét byte và tránh cấp phát

Một bộ phân tích CSV truyền thống tạo một `QString`/`QVariant` cho #strong[mỗi ô]. Với 10 triệu dòng $times$ 2 cột, đó là 20 triệu lần cấp phát bộ nhớ - đúng nút thắt cổ chai. Đường nhanh (`FastCsvDataParser`) thay vào đó quét byte bằng một vòng lặp phẳng và phát ra các #strong[dải byte thô (byte-span)] gom vào một `CsvRowBatch`; việc quy đổi sang `double` chỉ diễn ra một lần, ở phía sau, bằng `std::from_chars`@cppreference_from_chars không cấp phát.

Các điểm mấu chốt của hiện thực:
- #strong[Xử lý ranh giới khối.] Một bản ghi dở dang ở cuối khối được cất vào `m_carry` và ghép vào đầu khối kế tiếp - đây là điều làm cho đầu vào theo dòng, chia khối, trở nên đúng đắn.
- #strong[Cấu trúc phẳng.] `CsvRowBatch` giữ ba vector phẳng: `storage` (byte trường đã giải mã), bảng dải `(fieldStart, fieldLen)`, và `rowStart` ánh xạ dòng tới trường. Với trường không có dấu nháy, bộ phân tích #strong[sao chép nguyên cả trường một lần] thay vì nối từng byte.
- #strong[Giữ trường ở dạng khung nhìn chuỗi (`string_view`) vào `storage`], nhờ đó việc dựng `QString` đắt đỏ được hoãn tới đúng một chỗ thực sự cần chuỗi (dòng tiêu đề và các cột kiểu chuỗi) và bỏ qua hoàn toàn với số và ngày tháng.

Đây là cách hiện thực khả thi vì CSV@rfc4180_csv là một văn phạm chính quy (regular grammar): một lượt quét tiến duy nhất với tập trạng thái nhỏ (đang trong trường / đang trong nháy) là đủ. Sau khi phân tích, `DataBufferUpdater::parseBatch` chuyển mỗi dải trường thành `double` vào các vector dàn xếp (staging) theo cột, rồi #strong[bổ sung hàng loạt tất cả cột cùng lúc với một lần khóa mutex duy nhất]. Chi phí nạp được đo tách thành hai phần `convert` (quy đổi `std::from_chars`) và `appendRow` (bổ sung cột), giúp định vị chính xác điểm nóng.

Kết quả tham chiếu: #strong[nạp 10 triệu dòng trong dưới 1 giây] (\~920 ms) trên bản Release, trong đó chi phí lớn nhất là đọc và quét CSV, kế đến là quy đổi số, còn bổ sung cột theo lô chỉ chiếm phần nhỏ. Phân rã số liệu chi tiết được trình bày ở Chương 6.

=== Phân tích ngày tháng nhanh: `days_from_civil`

Cột ngày tháng là điểm nóng nạp dữ liệu thứ hai. Thay vì dựng `QDateTime` cho mỗi dòng (đắt), `DataBufferUpdater` #strong[chọn một chiến lược phân tích cho mỗi cột đúng một lần], kiểm chứng nó với một tham chiếu đúng, rồi tái sử dụng cho mọi dòng còn lại. Đường nhanh `fastDateMs` chuyển ngày lịch sang mốc thời gian epoch-ms #strong[mà không dựng `QDate`/`QDateTime`], dùng thuật toán lịch của Howard Hinnant@hinnant_date_algorithms:

$ "days" = "era" times 146097 + "doe" - 719468 $

trong đó `days` là số ngày kể từ mốc Unix (1970-01-01), còn các hằng số mã hóa chu kỳ Gregory 400 năm ($146097$ ngày) và độ lệch từ mốc nội bộ của thuật toán (0000-03-01) tới mốc Unix ($719468$ ngày). Mọi giá trị được diễn giải theo giờ UTC.

Điểm đáng chú ý về mặt kỹ thuật là #strong[cách bảo đảm đúng đắn]: chọn đường nhanh một cách mù quáng có thể âm thầm đổi giá trị epoch-ms với một định dạng biên (edge-case), khiến các điểm bị dịch ngang. Do đó đường nhanh được yêu cầu #strong[tái tạo đúng kết quả của một đường tham chiếu] (bản chuyển đổi tổng quát, chậm nhưng chắc) trên giá trị đầu tiên của cột; chỉ khi trùng khớp mới được dùng. Nhờ vậy vừa bảo đảm giá trị trục X không bao giờ lệch, vừa chỉ trả giá cho đường chậm đúng một lần trên mỗi cột.

== Lưu trữ cột, ảnh chụp sao chép khi ghi và cửa sổ trượt

=== Lưu trữ cột theo khối

`DataBuffer` tổ chức mỗi cột thành một vector các khối `DataChunk`, mỗi khối là một mảng `double` kích thước cố định (`CHUNK_SIZE = 10000`). Việc bổ sung lấp đầy khối đuôi bằng các đoạn `memcpy`, chỉ cấp phát khối mới khi tràn. Địa chỉ hóa một hàng là phép chia và lấy dư đơn giản: `chunkIdx = row / CHUNK_SIZE`, `localIdx = row % CHUNK_SIZE`.

Vậy tại sao lại chia khối thay vì một vector lớn duy nhất? Có hai lý do chính:

+ Việc bổ sung không bao giờ phải cấp phát lại và di dời #strong[toàn bộ] cột - chỉ khối đuôi dở dang lớn lên.
+ #strong[Loại bỏ ở đầu có thể bỏ nguyên cả khối] mà không phải dịch chuyển phần còn lại.

=== Ảnh chụp sao chép khi ghi (copy-on-write)

Hàm `snapshot()` chỉ sao chép các #strong[tay cầm `shared_ptr`] tới khối dữ liệu (cùng danh mục, tên cột và các bộ đếm), dưới khóa mutex. Một bộ đệm 137 MB được chụp trong #strong[vài micro giây] vì không một giá trị `double` nào bị sao chép; luồng giao diện đọc đúng những khối bất biến mà luồng ghi đã tạo ra. Hai bộ đếm `epochId` (ngẫu nhiên mỗi lần dựng lại bộ đệm) và `version` (tăng đơn điệu mỗi lần thay đổi) cho phép bên tiêu thụ #strong[phát hiện "có gì thay đổi không" với chi phí $O(1)$], không cần so sánh dữ liệu.

=== Cửa sổ trượt: loại bỏ theo khối

Ở chế độ trực tuyến, khi vượt ngưỡng `maxCacheRows`, hàm `trimToCapLocked()` loại bỏ #strong[nguyên các khối ở đầu]:

```cpp
while (m_rowCount > m_maxRows && probe.size() > 1) {
  for (col : m_columns) col.chunks.erase(col.chunks.begin());
  m_firstRowId += CHUNK_SIZE;  // id tuyệt đối của hàng sống thứ 0 tiến lên
  m_rowCount  -= CHUNK_SIZE;
}
```

Bất biến then chốt: các khối ở đầu luôn #strong[đầy đủ] `CHUNK_SIZE` (chỉ khối cuối mới dở dang), nên bỏ chúng vẫn giữ mọi khối còn lại thẳng hàng theo bội số `CHUNK_SIZE` và phép địa chỉ hóa vẫn đúng. Biến `firstRowId` là #strong[id tuyệt đối] của hàng sống thứ 0: dù cửa sổ vật lý trượt đi, bên tiêu thụ vẫn giữ được các id hàng ổn định, chỉ tăng. Chính `firstRowId` là thứ giúp bộ nhớ đệm điểm và bộ tính biên phân biệt "hàng đuôi mới" với "hàng đã rơi khỏi đầu" - nền tảng cho hai mục tiếp theo.

== Tính biên tăng tiến và lỗi "đóng băng" khi loại bỏ dữ liệu

Biên dữ liệu tuyệt đối (`absoluteXRange`/`absoluteYRange`) chính là #strong[phạm vi dữ liệu đầy đủ] trao cho `ViewportController`; khi người dùng chưa phóng to, biểu đồ khớp đúng phạm vi này. Tính sai phạm vi X sẽ dịch chuyển mọi điểm trên màn hình.

=== Tính biên tăng tiến

Quét lại toàn bộ hàng mỗi frame để tìm min/max sẽ là #strong[$O("cửa sổ")$] - hàng triệu giá trị mỗi ảnh chụp khi đang truyền dòng. Thay vào đó, bộ tính biên chỉ quét #strong[phần đuôi mới bổ sung], lấy min/max của phần chênh ($Delta$) đó rồi hợp (union) vào phạm vi đã lưu. Phép hợp là một bộ tích lũy chạy thuần túy: $min = min(a, b)$, $max = max(a, b)$ - nó chỉ #strong[nới rộng]. Chi phí do đó là #strong[$O(Delta)$] thay vì $O("cửa sổ")$.

=== Vấn đề: đồ thị "đóng băng" khi loại bỏ

Một bộ tích lũy chỉ-hợp #strong[không nhớ hàng nào sinh ra cực trị], nên nó không bao giờ #strong[nâng] được min - chỉ hạ min xuống. Điều đó đúng khi dữ liệu chỉ tăng. Nhưng với cửa sổ trượt, bộ đệm loại bỏ các hàng cũ ở đầu, trong khi `resolvedXRange.min` vẫn mang giá trị x của những hàng không còn tồn tại. Miền tự-co-giãn kẹt ở `[x_cũ_nhất, x_mới_nhất]` trong khi dữ liệu sống chỉ là cửa sổ trượt `[x_sống_đầu, x_mới_nhất]`. Khi dòng tiếp tục, `x_mới_nhất` cứ lớn lên nhưng mép trái bị ghim vào quá khứ xa, khiến cửa sổ sống thành một dải ngày càng mỏng nép sát #strong[mép phải] - đường như trượt sang phải và co lại tới khi biến mất, còn các vạch trục X (tính lại trên miền khổng lồ đã đóng băng) nhảy loạn mỗi khung.

=== Cách khắc phục: kẹp min lên mép sống

Không thể trừ một hàng đã loại ra khỏi min/max mà không quét lại, và quét lại sẽ phá thiết kế $O(Delta)$. Nhưng cũng không cần: trong chế độ truyền dòng có trần, #strong[x là không giảm], nên mép dưới mới đơn giản chính là x của hàng sống đầu tiên. Sau phép hợp, khi `firstRowId > 0`, bộ tính biên nâng min lên bằng x hợp lệ đầu tiên:

```cpp
for (qint64 row = 0; row < snapshot.rowCount; ++row) {
  const double v = snapshot.valueAt(resolved.xColumnIndex, row);
  if (!std::isnan(v) && std::isfinite(v)) {
    cache.resolvedXRange.min = std::max(cache.resolvedXRange.min, v);
    break;  // O(1) trong trường hợp thường
  }
}
```

Bốn chi tiết khiến cách này vừa đúng vừa rẻ:

+ Dùng `std::max` chứ không gán - min đã lưu luôn `≤ x_sống_đầu` nên `max` chọn đúng mép sống, và tự bảo vệ khỏi vô tình nới rộng.
+ Vòng lặp kèm `break` bỏ qua các giá trị `NaN`/vô hạn ở đầu, cho chi phí #strong[$O(1)$] trong trường hợp thường.
+ Chỉ chạm `min`, không chạm `max`, vì loại bỏ ở đầu chỉ làm cũ mép dưới.
+ Chỉ áp cho trục X, không cho Y - vì trục Y được tự co giãn mỗi khung theo các điểm thực sự thấy trong cửa sổ X hiện tại, nên một Y min/max cũ là vô hại.

Sau khi áp dụng, đồ thị không còn "đóng băng" nữa: cửa sổ sống luôn khớp với dữ liệu sống, trục X tự co giãn theo cửa sổ sống, các vạch trục nhảy đúng theo dữ liệu hiện tại và vẫn #strong[giữ được $O(Delta)$].

== Bộ nhớ đệm điểm tăng tiến

Tính lại các điểm `QPointF` và biên từ đầu mỗi khung sẽ là $O(N)$ cho mỗi thao tác zoom/pan/truyền dòng - tử huyệt ở quy mô hàng triệu điểm. Bộ nhớ đệm điểm làm cho phần xử lý mỗi series trở thành #strong[$O(Delta)$] (chỉ hàng mới) cộng #strong[$O("số pixel thấy được")$] (lát cắt thực sự vẽ). `ChartView` sở hữu một `QHash` khóa theo bộ ba `(sourceId, xCol, yCol)`, còn `LineSeriesStrategy::build()` là bên tiêu thụ chính.

=== Vì sao dùng `std::deque` chứ không `QVector`

Điểm đã quy đổi được giữ trong `std::deque<QPointF>` (và kim tự tháp LOD cũng giữ các "xô" của nó trong deque). Đây là lựa chọn có chủ đích và mang tính sống còn cho cửa sổ trượt: một `QVector` khi xóa ở đầu sẽ #strong[dịch chuyển (memmove) toàn bộ phần đuôi] xuống mỗi lần loại bỏ #strong[và] dịch mọi chỉ số, buộc phải dựng lại kim tự tháp $O("cửa sổ")$ mỗi lần - biến một lần nạp có trần thành #strong[bậc hai (quadratic)] và giật rõ rệt. Deque giải phóng nguyên các nút ở đầu mà không chạm phần đuôi, nên loại bỏ ở đầu chỉ tốn #strong[$O("số bị loại")$]. Ngoài ra deque còn tránh phần dư cấp phát dôi và các đợt cấp phát lại và sao chép định kỳ của `QVector` khi series nhiều triệu điểm lớn dần.

=== Hàm `build()` và chuỗi xử lý tăng tiến

`build()` là phương thức cốt lõi của chiến lược series (`ISeriesStrategy`, ở đây là `LineSeriesStrategy`) đã giới thiệu ở điểm nối mở rộng của Chương 3. Mỗi khi cần dựng lại biểu đồ - lúc có ảnh chụp dữ liệu mới, lúc người dùng zoom/pan, hay khi bố cục thay đổi - `ChartView` gọi `build()` để biến series đã phân giải cùng viewport hiện tại thành dữ liệu kết xuất (`RenderData`). Đây chính là nơi tiêu thụ bộ nhớ đệm điểm và bộ giảm mẫu, nên toàn bộ gánh nặng hiệu năng của đường dữ liệu đi vào dồn về đây; nếu `build()` tốn $O(N)$ mỗi lần gọi thì mọi tối ưu phía trước đều vô nghĩa. Để tránh điều đó, `build()` duy trì một chuỗi bước với các bất biến bảo đảm chỉ xử lý phần chênh:
+ #strong[Kiểm tra epoch.] Nếu `epochId` khác với ảnh chụp thì bộ đệm đã bị dựng lại - đặt lại toàn bộ cache.
+ #strong[Mốc nước cao (high-water mark).] `processedRowCount` là một id hàng #strong[tuyệt đối, đơn điệu], không phải số đếm sống. Việc mới cần làm là `max(processedRowCount, liveStart), liveEnd)`; loại bỏ ở đầu làm `firstRowId` tiến lên nhưng không bao giờ lùi mốc nước cao.
+ #strong[Chỉ quy đổi phần chênh] thành điểm, rồi bổ sung vào đuôi deque. Cờ `appendedOnly` là đúng khi mọi điểm mới nằm tại/sau x lớn nhất trước đó - trường hợp truyền dòng thường gặp, cho phép kim tự tháp mở rộng tăng tiến thay vì dựng lại.
+ #strong[Loại bỏ ở đầu cache - $O("số bị loại")$.] Khi `firstRowId > 0`, tiền tố cũ (các điểm trước x đầu của cửa sổ sống, tìm bằng `std::lower_bound`) bị bỏ. Chiến lược hỏi kim tự tháp qua `trimFront` xem được phép bỏ bao nhiêu điểm mà vẫn giữ các xô thẳng hàng, rồi xóa đúng chừng ấy từ đầu deque để điểm và kim tự tháp luôn đồng bộ chỉ số.
+ #strong[Đồng bộ kim tự tháp LOD] - $O(Delta)$ khi bổ sung đuôi; dựng lại toàn phần $O(N)$ chỉ xảy ra khi thực sự đặt lại.
+ #strong[Cắt lát và giảm mẫu.] Tìm nhị phân phạm vi x thấy được ($O(log N)$), rồi hoặc sao chép lát nhỏ nguyên trạng, hoặc truy vấn kim tự tháp ($O("số xô thấy được") approx O("số pixel")$).

Cộng gộp chi phí của toàn bộ một lần gọi `build()` cho ra kết quả then chốt của cả chương. Gọi $N$ là tổng số điểm sống, $d$ là số hàng mới bổ sung và $e$ là số điểm bị loại ở khung hiện tại, ba tình huống tách bạch rõ:

- #strong[Khung truyền dòng] (có dữ liệu mới về): quy đổi, bổ sung và đồng bộ kim tự tháp đều #strong[$O(d)$], loại bỏ ở đầu #strong[$O(e)$], cắt lát #strong[$O(log N)$], truy vấn giảm mẫu #strong[$O("số pixel")$]. Tổng là #strong[$O(d + e + log N + "số pixel")$]. Ở trạng thái ổn định, cửa sổ trượt chặn $d$ và $e$ ở mức nhỏ, nên thực chất chỉ còn #strong[$approx O("số pixel")$].
- #strong[Khung tương tác thuần] (zoom/pan, không có dữ liệu mới): $d = e = 0$, kiểm tra epoch và đồng bộ kim tự tháp rơi vào nhánh không đổi #strong[$O(1)$], chỉ còn cắt lát #strong[$O(log N)$] và truy vấn #strong[$O("số pixel")$]. Tổng #strong[$O(log N + "số pixel") approx O("số pixel")$] - #strong[độc lập với $N$].
- #strong[Khung đặt lại] (nạp lần đầu, dữ liệu đảo thứ tự, đổi trần giữa dòng,...): dựng lại toàn phần #strong[$O(N)$], nhưng chỉ xảy ra một lần chứ không phải mỗi khung.

So với cách tính lại từ đầu tốn #strong[$O(N)$ mỗi khung], `build()` hạ chi phí của thao tác tương tác - thứ người dùng cảm nhận trực tiếp - xuống còn cỡ #strong[số pixel màn hình], gần như không phụ thuộc vào việc dữ liệu có một triệu hay một trăm triệu điểm. Đây chính là cơ sở định lượng cho tuyên bố "vẽ triệu điểm vẫn mượt" nêu ở Chương 1.

== Kim tự tháp mức chi tiết (LOD)

Đây là cấu trúc giảm mẫu lõi và cũng là nền tảng cho lời hứa "chi phí hiển thị gần như hằng số theo số điểm".

=== Ý tưởng

Kim tự tháp LOD là một #strong[kim tự tháp mip (mipmap)@williams_pyramidal_1983 mức chi tiết trên chuỗi điểm đã sắp theo x]. Mỗi #strong[xô (bucket)] lưu #strong[điểm mẫu thực] có y nhỏ nhất và y lớn nhất trong một #strong[số lượng cố định] điểm thô liên tiếp. Mức 0 gom `kBranch = 4` điểm thô mỗi xô; mỗi mức thô hơn gom 4 xô của mức dưới, nên nhịp (span) tăng theo cấp số nhân (4, 16, 64,...) cho tới khi đỉnh còn `≤ 128` xô.

#figure(
  image("/assets/images/lod-pyramid.drawio.png", width: 100%, height: auto),
  caption: [Cấu trúc kim tự tháp mức chi tiết (LOD): xô chia theo số lượng điểm, mức thô hơn gom 4 xô của mức dưới],
) <fig-lod-pyramid>

Điểm cốt tử của thiết kế: xô chia theo #strong[số lượng] (count), không theo #strong[bề rộng x]. Nhờ vậy, tiền tố #strong[ổn định khi bổ sung đuôi] - thêm một điểm mới chỉ viết lại xô dở dang cuối cùng của mỗi mức, lịch sử không bao giờ được chọn lại. Đây chính là tính chất làm cho truyền dòng rẻ. Việc lưu #strong[nguyên điểm] (không phải điểm giữa tổng hợp) là thứ cho phép một truy vấn phát ra đúng gai (spike) tại vị trí x thật của nó.

=== Cập nhật và dựng lại tăng tiến

Mỗi khung, kim tự tháp nhận về toàn bộ dãy điểm hiện có và tự quyết định làm ít nhất có thể. Bước điều phối so sánh số điểm lần này với số điểm nó đã phản ánh ở lần dựng trước, rồi chọn một trong bốn kịch bản, mỗi kịch bản một mức chi phí khác nhau:

- #strong[Không có gì thay đổi.] Số điểm y hệt lần trước nên kim tự tháp đã đúng sẵn, chỉ tốn một phép so sánh số nguyên rồi trả về ngay. Đây là kịch bản của #strong[mọi khung zoom/pan thuần túy] - người dùng chỉ tương tác chứ không có dữ liệu mới về - nên phần cập nhật tốn #strong[$O(1)$]. Đây chính là lý do cốt lõi khiến thao tác tương tác gần như miễn phí.
- #strong[Không còn điểm nào.] Xóa sạch mọi mức - #strong[$O(1)$].
- #strong[Chỉ có điểm mới nối vào đuôi] (trường hợp truyền dòng thường gặp): dựng lại tăng tiến, chỉ đụng tới phần vừa thêm - #strong[$O(Delta)$].
- #strong[Thay đổi không phải nối đuôi] (nạp lần đầu, dữ liệu đảo thứ tự, số điểm giảm do đổi trần,...): dựng lại toàn bộ từ đầu - #strong[$O(N)$].

Cả dựng toàn bộ lẫn dựng tăng tiến đều đi qua cùng một thủ tục ba pha, chỉ khác nhau ở điểm bắt đầu:
+ #strong[Dựng mức mịn nhất từ điểm thô.] Thủ tục #strong[cắt bỏ đúng cái xô đuôi còn dở dang] của lần trước rồi dựng tiếp từ đó. Nhờ vậy, khi truyền dòng, nó chỉ viết lại nhiều nhất #strong[một] xô cũ ở mỗi mức chứ không đụng cả dãy.
+ #strong[Lan truyền thay đổi lên các mức thô hơn.] Mỗi mức trên được tổng hợp lại từ 4 xô con bên dưới, nhưng chỉ ở vùng vừa bị thay đổi. Vì vùng này thu hẹp 4 lần mỗi khi lên một mức, tổng số xô phải tính lại của cả cây chỉ khoảng #strong[$1 / 3$] số xô của phần dữ liệu thêm mới.
+ #strong[Mọc thêm mức thô mới] khi cây cao dần lên, cho tới khi mức đỉnh còn không quá 128 xô. Điều này khống chế chiều cao cây ở khoảng #strong[$ log_4(N / 128) $] mức, bảo đảm truy vấn thô nhất không bao giờ phải đọc quá chừng 128 xô.

=== Loại bỏ cửa sổ trượt không cần dựng lại

Khi cửa sổ trượt loại bỏ các hàng cũ, kim tự tháp phải bỏ những xô cũ nhất cho khớp. Nếu đánh chỉ số lại từ gốc mới thì cửa sổ điểm mà mỗi xô tổng hợp sẽ dịch đi, buộc phải dựng lại toàn bộ - đúng thứ ta muốn tránh. Cách giải quyết là một #strong[mẹo canh lề]: thay vì bỏ đúng số điểm được yêu cầu, chỉ bỏ đi một lượng là #strong[bội số của nhịp mức đỉnh] (nhịp lớn nhất).

Lý do: để mọi xô #strong[còn sống] vẫn thẳng hàng với gốc chỉ số mới, số điểm bỏ đi phải là bội số của #strong[mọi] nhịp cùng lúc. Mà nhịp thô nhất vốn là bội số của mọi nhịp mịn hơn (mỗi nhịp gấp 4 lần nhịp dưới nó), nên chỉ cần làm tròn #strong[xuống] theo nhịp đỉnh là thỏa mãn ràng buộc ở tất cả các mức đồng thời. Sau đó mỗi mức chỉ việc bỏ đi số xô đầu tương ứng, #strong[không phải tính lại min/max của bất kỳ xô nào] - mỗi xô sống sót vẫn tổng hợp đúng những điểm thô cũ, chỉ nằm ở chỉ số thấp hơn. Nhờ dùng deque (mục 4.5), các phép bỏ ở đầu này tốn #strong[$O("số bị loại")$] thay vì phải dịch chuyển cả mảng như với vector thường.

Một điểm thiết kế cần chú ý là vấn đề #strong[sự đồng bộ hai chiều]: phép loại bỏ trả về số điểm nó #strong[thực sự] bỏ (một bội số của nhịp đỉnh, thường ít hơn lượng được yêu cầu), và bên gọi #strong[phải] xóa đúng chừng ấy điểm ở đầu dãy điểm của mình, để chỉ số điểm và chỉ số xô luôn khớp nhau. Nhiều nhất một nhịp đỉnh điểm cũ còn nán lại như phần dư vô hại nằm ngoài mép trái khung nhìn. Kết cục: loại bỏ của một dòng có trần chỉ tốn #strong[$O("số bị loại")$] - vài phép bỏ xô đầu ở mỗi mức cộng một phép xóa đầu dãy điểm - #strong[không dựng lại và không có đợt giật định kỳ].

=== Truy vấn để vẽ: băng bao min/max

Khi cần dữ liệu để vẽ, kim tự tháp nhận một dải điểm thấy được (xác định bằng tìm nhị phân trên trục x), chọn mức đủ thô để số xô nằm trong ngân sách hiển thị (xấp xỉ số pixel ngang của khung vẽ), rồi chỉ đọc các xô giao với dải đó.

Chế độ truy vấn đang dùng là #strong[băng bao (envelope) giữ mọi gai]: với mỗi xô giao nhau, nó phát ra #strong[cả hai cực trị] - điểm có y nhỏ nhất và điểm có y lớn nhất - sắp theo thứ tự x. Vì mỗi xô góp đúng điểm min và max #strong[thật] của mình, #strong[mọi gai dọc trong dữ liệu đều sống sót] dù thu nhỏ đến đâu. Đây chính là ưu điểm quyết định so với phép lấy mẫu thưa đơn giản (chỉ giữ mỗi điểm thứ n): cách đó dễ đánh rơi đúng các đỉnh/đáy nhọn - vốn là những đặc trưng đáng chú ý nhất của tín hiệu - trong khi băng bao bảo toàn trọn biên độ. Số xô đọc xấp xỉ ngân sách nên chi phí mỗi truy vấn là #strong[$O("số pixel")$], độc lập với $N$.

Ý tưởng giảm mẫu min/max cho chuỗi thời gian này cũng chính là cách các bộ vẽ dạng sóng âm thanh và nhiều thư viện đồ thị áp dụng, và có cơ sở lý thuyết trong công trình M4@jugel_m4_2014.

=== Tổng hợp chi phí

Gộp lại toàn bộ, chi phí của kim tự tháp được phân tách rõ theo từng loại thao tác trong một khung:
- #strong[Dựng lần đầu hoặc đặt lại]: #strong[$O(N)$], nhưng chỉ xảy ra một lần chứ không phải mỗi khung.
- #strong[Thêm dữ liệu mới] (truyền dòng): #strong[$O(Delta)$] - chỉ tỉ lệ với lượng vừa thêm.
- #strong[Loại bỏ cửa sổ trượt]: #strong[$O("số bị loại")$] - không dựng lại.
- #strong[Khung tương tác thuần] (zoom/pan, không dữ liệu mới): phần cập nhật #strong[$O(1)$].
- #strong[Truy vấn để vẽ] (băng bao min/max): #strong[$O("số pixel")$], độc lập với $N$.

Nói cách khác, trong lúc người dùng đang tương tác, #strong[không một thao tác nào trên kim tự tháp tỉ lệ với tổng số điểm $N$]\; $N$ chỉ xuất hiện đúng một lần ở lần dựng đầu tiên, và ở số hạng logarit của phép tìm nhị phân. Chính điều này biến "một triệu điểm" hay "một trăm triệu điểm" thành con số không còn ảnh hưởng tới độ mượt - hiện thực hóa lời hứa "chi phí hiển thị gần như hằng số theo số điểm" nêu ở đầu mục.

== Thuật toán giảm mẫu LTTB

=== Ý tưởng

#strong[Largest-Triangle-Three-Buckets (LTTB)]@steinarsson_downsampling_2013 giảm mẫu $N$ điểm xuống $M$ điểm mà giữ hình dạng trực quan tốt hơn nhiều so với cách lấy mỗi điểm thứ $n$ đơn giản. Nó chia dữ liệu thành $M - 2$ xô bằng nhau (điểm đầu và cuối luôn được giữ), và từ mỗi xô chọn #strong[điểm tạo tam giác có diện tích lớn nhất] với (a) điểm đã chọn liền trước và (b) điểm #strong[trung bình] của xô kế tiếp.

#figure(
  image("/assets/images/lttb.png", width: 100%, height: auto),
  caption: [Hình tam giác có diện tích lớn nhất tạo bởi 3 xô và điểm C là điểm tạm thời ở xô cuối],
) <fig-lttb>

=== Công thức và tối ưu

Diện tích tam giác với ba đỉnh $A$ (điểm đã chọn trước), $B$ (ứng viên trong xô hiện tại), $C$ (trung bình xô kế):

$ "Area" = 1/2 dot |(x_A - x_C)(y_B - y_A) - (x_A - x_B)(y_C - y_A)| $

Mã nguồn #strong[hoist các số hạng hằng theo xô] ($"constX" = x_A - x_C$, $"constY" = y_C - y_A$), #strong[bỏ hệ số $1/2$] (chỉ cần so sánh tương đối), và dùng trực tiếp `source[j]` làm $B$ để tránh sao chép. Diện tích xô là một đại lượng thay thế (proxy) cho #strong[tầm quan trọng thị giác]: một điểm mà cùng các láng giềng bao một tam giác lớn là điểm đường đổi hướng gấp - đúng những đặc trưng mắt người chú ý. Lấy trung bình xô kế cho $C$ tạo một hướng "nhìn trước" ổn định để lựa chọn không bị nhiễu chi phối.

=== Song song hóa

Vòng lặp xô được chia cho `idealThreadCount()` luồng làm việc qua `QtConcurrent::run`, gộp lại bằng `QFutureSynchronizer`. Mỗi luồng #strong[xấp xỉ] điểm $A$ khởi đầu của mình theo chỉ số (`aIndex = startBucket * bucketSize`) để các luồng độc lập với nhau - một xấp xỉ biên nhỏ đổi lấy tăng tốc gần tuyến tính.

== Các bộ giảm mẫu dự phòng

Ngoài kim tự tháp LOD (đường mặc định hiện tại), hệ thống giữ hai bộ giảm mẫu dự phòng. Chiến lược chọn theo thứ tự #strong[kim tự tháp → LTTB → min/max cố định].

- #strong[HybridDownsampler.] Chọn chiến lược theo #strong[tỉ lệ giảm mẫu] `sourceSize / destinationSize`: nếu tỉ lệ vượt ngưỡng 500 thì mỗi xô phát #strong[đường giữa (min+max)/2] (rẻ và ổn định hơn ở tỉ lệ cực lớn, khi LTTB chọn một-điểm bỏ đi quá nhiều); ngược lại dùng LTTB song song.
- #strong[StationaryMinMaxDownsampler.] Một bộ giảm mẫu min/max mà các xô neo vào một #strong[lưới cố định trong không gian dữ liệu] (`bucket = floor(x / gridWidth)`) thay vì theo dải chỉ số. Vì lưới #strong[đứng yên], các điểm mẫu được chọn không nhảy khi pan/zoom - đầu ra #strong[không nhấp nháy (shimmer-free)]. Với xô theo chỉ số, dịch một điểm thô làm lệch mọi ranh giới xô nên cực trị được chọn cứ chập chờn; neo xô vào x tuyệt đối khiến một gai luôn rơi vào cùng một xô và được chọn nhất quán qua các khung.
