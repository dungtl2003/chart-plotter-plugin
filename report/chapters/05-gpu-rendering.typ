= Kỹ thuật kết xuất đồ họa trên GPU

Nếu Chương 4 mô tả đường dữ liệu đi vào - từ byte thô tới tập điểm đã giảm mẫu - thì chương này mô tả #strong[đường đi ra]: biến tập điểm đó thành hình ảnh trên màn hình. Toàn bộ hình học được đẩy xuống GPU và vẽ lại theo pixel ở mỗi khung, thay vì tô lại bằng CPU. Điểm nhấn kỹ thuật của chương là kỹ thuật khử răng cưa tự viết dựa trên trường khoảng cách có dấu (SDF), cho nét vẽ sắc nét, độc lập độ phân giải và không phụ thuộc khử răng cưa đa mẫu (MSAA) của phần cứng. Như ở Chương 4, mỗi kỹ thuật tối ưu đều kèm phân tích độ phức tạp để đối chiếu, và chương khép lại bằng một mục tổng hợp chi phí kết xuất một khung. Chương gồm các nội dung chính sau:
- Ánh xạ tọa độ dữ liệu sang màn hình.
- Toán học viewport: zoom và pan.
- Trục số: nice numbers và sinh vạch chia.
- Kết xuất đường: hình học capsule và khử răng cưa SDF.
- Nét đứt, nét chấm và marker.
- Kết xuất biểu đồ cột và biểu đồ tròn.
- Kết xuất trục và chữ bằng atlas texture.
- Tích hợp scene graph và trộn màu.
- Trỏ xem chi tiết và điều tiết cập nhật.

== Ánh xạ tọa độ dữ liệu sang màn hình

Mọi bộ kết xuất đều ánh xạ dữ liệu sang màn hình theo cùng một công thức. Trước hết, tọa độ dữ liệu được chuẩn hóa về đoạn $[0, 1]$ theo phạm vi trục hiện tại, rồi trải lên vùng vẽ (plot area) tính bằng pixel:

$ n_x = (x - x_min) / (x_max - x_min), quad n_y = (y - y_min) / (y_max - y_min) $
$ "screenX" = "plot"_L + n_x dot "plot"_W, quad "screenY" = "plot"_B - n_y dot "plot"_H $

Trục Y bị #strong[lật] (trừ vào cạnh dưới `plot`#sub[B]) vì tọa độ màn hình tăng theo chiều xuống, còn giá trị dữ liệu tăng theo chiều lên. Phép chuẩn hóa có chốt chặn phạm vi bề rộng bằng 0 (trả về $0.5$) để không chia cho 0.

Một quyết định thiết kế quan trọng: hình học được dựng #strong[theo pixel ở mỗi khung], dựa trên viewport hiện tại, chứ không dựng một lần trong không gian dữ liệu rồi để GPU co giãn. Nhờ đó ma trận biến đổi `u_mvp` trên GPU chỉ còn nhiệm vụ tối giản là đưa tọa độ pixel sang không gian cắt (clip space) của OpenGL. Chi phí ánh xạ là $O(1)$ cho mỗi điểm, tức $O("số điểm vẽ")$ cho mỗi khung - mà số điểm vẽ đã được Chương 4 khống chế ở cỡ số pixel ngang, độc lập với tổng số điểm $N$.

== Toán học viewport: zoom và pan

`ViewportController` quản lý hai phạm vi: toàn bộ dữ liệu và cửa sổ đang thấy. Mọi thao tác zoom/pan chỉ biến đổi cửa sổ đang thấy.

=== Chuyển bánh xe chuột thành bước zoom

Hệ điều hành báo mỗi nấc lăn chuột là $120$ đơn vị (quy ước WHEEL_DELTA, ứng với $15 degree$ mỗi nấc). Mã nguồn quy đổi thành số bước hình học $s = 120 \/ 8 \/ 15 = 1$ bước mỗi nấc. Chỉ zoom khi con trỏ nằm trong vùng vẽ. Các đường không-zoom đều gọi `event->ignore()` để sự kiện lăn nổi lên phần tử cha (một `Flickable`/`ScrollView`), nhờ đó một biểu đồ nhúng trong danh sách cuộn vẫn cuộn được danh sách thay vì nuốt mất sự kiện.

=== Zoom neo theo con trỏ

Mỗi bước zoom nhân bề rộng cửa sổ với một hệ số hình học $"factor" = 0.9^s$ ($s > 0$ thu nhỏ cửa sổ - phóng to; $s < 0$ nới rộng). Điểm dưới con trỏ được giữ cố định: gọi $t = "clamp"(x_"chuột" \/ w_"khung", 0, 1)$ là vị trí tương đối của con trỏ, điểm neo trong không gian dữ liệu là $"anchor" = x_min + t dot "range"$, và cửa sổ mới giữ điểm neo ở đúng tỉ lệ màn hình đó:

$ "newMin" = "anchor" - t dot "newRange", quad "newMax" = "anchor" + (1 - t) dot "newRange" $

=== Sàn zoom-in chống suy biến

Mỗi lần phóng to nhân bề rộng với $0.9$, nên phóng liên tục sẽ đẩy bề rộng cửa sổ tiến về $0$ theo cấp số nhân. Một cửa sổ gần bằng 0 không chỉ vô dụng mà còn gây hỏng ở các khâu sau:
+ #strong[Trục suy biến.] Với cửa sổ như $[1.0000000, 1.0000001]$, bước chia "đẹp" (mục sau) làm tròn xuống một giá trị nhỏ tới mức nhiều vạch liền kề hiện #strong[cùng một nhãn].
+ #strong[Bước chia tiến về 0 gây lặp vô hạn.] Khi bề rộng co lại, bước chia tính ra bị tràn số về gần 0; vòng sinh vạch tiến từng bước bằng giá trị đó cho tới khi vượt cận trên, nhưng một bước bằng 0 không bao giờ tiến, khiến vòng lặp không kết thúc.
+ #strong[Nhiễu dưới mức điểm ảnh.] Ở độ lớn epoch-ms (khoảng $1.7 times 10^12$), một cửa sổ hẹp hơn vài mili-giây đã nằm dưới độ phân giải hữu ích của kiểu `double` tại độ lớn đó, khiến tọa độ dao động ở các bit thấp.

Giải pháp là áp một #strong[sàn] cho bề rộng cửa sổ trước khi tính cận mới. Sàn này #strong[phụ thuộc dữ liệu] chứ không phải hằng số cố định, và lấy giá trị lớn hơn giữa hai số hạng:

$ w_min = max("dataSpan" dot 10^(-4), "magnitude" dot 10^(-9)) $

Số hạng thứ nhất ($"dataSpan" dot 10^(-4)$) cho phép phóng to tới khi cửa sổ còn $1\/10000$ toàn dải dữ liệu - đủ sâu cho mọi nhu cầu thực tế mà không sụp về 0. Số hạng thứ hai ($"magnitude" dot 10^(-9)$) là lưới an toàn cho trường hợp giá trị rất lớn nhưng dải lại rất hẹp (ví dụ trục epoch-ms hiển thị một phút dữ liệu): nó giữ cửa sổ trên ngưỡng mà bản thân #strong[giá trị] bắt đầu mất bit. Lấy $max$ của hai số hạng bảo đảm phủ đồng thời cả hai kiểu hỏng. Với $"factor" = 0.9$, phải mất khoảng $log(10^(-4)) \/ log(0.9) approx 87$ nấc lăn liên tục mới chạm sàn, nên người dùng gần như không gặp trong thao tác bình thường, nhưng khi giữ lăn chuột (hoặc với trục có độ lớn giá trị rất lớn) thì sàn này chặn lại mà không gây gián đoạn hiển thị. Chi phí kiểm tra sàn là $O(1)$ mỗi lần zoom.

=== Pan và bám mép động

Kéo trượt chuyển độ dời pixel thành độ dời dữ liệu: $"dataDelta" = (Delta x_"pixel" \/ w_"khung") dot "range"$, rồi dịch cả hai cận đi $-"dataDelta"$, có kẹp để không kéo ra ngoài dữ liệu. Pan bị vô hiệu khi đang ở chế độ tự co giãn (toàn bộ biểu đồ đang hiển thị). Khi dữ liệu mới về, nếu cửa sổ đang ghim ở mép phải cũ thì nó bám theo mép mới (cửa sổ đuổi theo hiện tại); nếu không thì đứng yên để người dùng khảo sát lịch sử - đây là điều khiến biểu đồ dòng "dính lấy hiện tại" trừ khi ta đã cuộn ngược về trước.

== Trục số: nice numbers và sinh vạch chia

Người đọc quen với trục chia theo các mốc $1, 2, 5, 10, 20, 50, ...$ chứ không phải 1,37 hay 2,74. Hàm `niceNumber`@heckbert_nice_numbers_1990 làm tròn một giá trị về "số đẹp" dạng ${1, 2, 5, 10} dot 10^n$: tách $e = lr(floor.l log_10 v floor.r)$ và phần định trị $f = v \/ 10^e in [1, 10)$, ép $f$ về một trong các mốc 1, 2, 5, 10 rồi nhân lại với $10^e$.

Từ đó, quy trình dựng cận đẹp cho một dải thô với số vạch mục tiêu gồm ba bước:

+ Làm tròn #strong[lên] độ rộng dải.
+ Tính bước chia $"step" = "niceNumber"("dải" \/ ("số vạch" - 1))$ (làm tròn gần nhất).
+ Đẩy hai cận ra ngoài về bội số của bước: $min = lr(floor.l x_min \/ "step" floor.r) dot "step"$ và $max = lr(ceil.l x_max \/ "step" ceil.r) dot "step"$.

Các trường hợp suy biến (dải rỗng, vô hạn, bước bằng 0) có nhánh dự phòng để tránh sinh vạch vô hạn/không hợp lệ.

Với trục phân loại (category), vạch chia không đặt tại tâm nhãn mà tại #strong[ranh giới các dải]: nhãn $i$ nằm giữa hai vạch ${-0.5, 0.5, ..., n - 0.5}$ (chế độ căn giữa khoảng). Nhờ đó một cột hay điểm ở vị trí $i$ nằm chính giữa dải của nó. Viewport được điều khiển bằng đúng dải $[-0.5, n - 0.5]$ (không làm tròn đẹp) để zoom lọc vạch trục và cột theo cùng một phạm vi.

== Kết xuất đường: hình học capsule và khử răng cưa SDF

Đây là kỹ thuật lõi của chương. Một đường (line series) thực chất là một #strong[đường gấp khúc (polyline)] nối các điểm đã vẽ, được phân rã thành các đoạn thẳng liên tiếp và mỗi đoạn vẽ độc lập. Mỗi đoạn thẳng được vẽ như một #strong[capsule] (hình viên nhộng: một hình chữ nhật với hai nửa hình tròn ở hai đầu). CPU chỉ phát ra một #strong[tứ giác bao] lỏng cho mỗi đoạn; fragment shader sau đó tính, cho mỗi điểm ảnh, #strong[khoảng cách có dấu tới trục của đoạn] và chỉ giữ điểm ảnh nếu khoảng cách đó không vượt quá nửa bề rộng nét.

#figure(
  image("/assets/images/polyline2D.drawio.png", width: 100%, height: auto),
  caption: [Hình học đường gấp khúc: một đường được phân rã thành các đoạn thẳng liên tiếp, mỗi đoạn là một capsule],
) <fig-polyline>

=== Vì sao "khoảng cách tới đoạn thẳng không quá $r$" là một capsule bo tròn đầu

Xét một đoạn thẳng từ $A$ tới $B$ và hỏi: những điểm $p$ nào nằm trong khoảng cách $r$ tới đoạn đó? Chia trường hợp theo vị trí hình chiếu vuông góc của $p$:
- #strong[Hình chiếu rơi vào trong đoạn] (giữa $A$ và $B$): điểm gần nhất nằm trên trục, nên biên $"dist" = r$ là một cạnh thẳng song song với đoạn, lệch ra $r$. Hai cạnh như vậy (mỗi bên một cạnh) tạo thành #strong[thân hình chữ nhật].
- #strong[Hình chiếu rơi ra ngoài một đầu mút]: điểm gần nhất chính là đầu mút đó, nên "cách đầu mút một khoảng $r$" là một #strong[cung tròn bán kính $r$] tâm tại đầu mút - chính là #strong[đầu bo tròn (round cap)].

Như vậy nét bo tròn không phải thứ ta chủ động vẽ, mà là tập mức chính xác ${p : "dist"(p, "đoạn") <= r}$. Toàn bộ mấu chốt nằm ở một hàm khoảng cách vừa đo tới điểm gần nhất trên đoạn, vừa tự động chuyển từ "vuông góc với thân" sang "xuyên tâm từ đầu mút" đúng chỗ.

#figure(
  image("/assets/images/capsule-sdf.drawio.png", width: 100%, height: auto),
  caption: [Hình học capsule: tập các điểm cách đoạn thẳng $A B$ không quá $r$ gồm thân chữ nhật và hai đầu bo tròn],
) <fig-capsule-sdf>

=== CPU: chỉ phát tứ giác bao

Nhiệm vụ duy nhất của CPU là phát một tứ giác đủ lớn để chứa capsule đó, để mọi điểm ảnh của capsule đều nhận được một fragment (phần ngoài bị shader loại bỏ). Cho đoạn $p_0 arrow p_1$, đặt $"dir"$ là véc-tơ đơn vị dọc đoạn và $n$ là pháp tuyến đơn vị. Tứ giác được nới ra hai chiều: dọc trục một đoạn $r$ để chừa chỗ cho hai đầu bo tròn (nếu không nới, hai cung tròn sẽ bị cắt bởi cạnh phẳng của tứ giác), và vuông góc một đoạn $r$ để chừa chỗ cho thân và dải khử răng cưa. Ở đây $r = "nửa bề rộng" + "khử răng cưa" + 1$.

Số hạng $+1$ cần được giải thích kỹ, vì nó xuất phát trực tiếp từ công thức tô sẽ nêu ở mục sau. Tứ giác bao #strong[bắt buộc phải chứa trọn vùng có độ phủ dương] ($alpha > 0$); phần nào của capsule rơi ra ngoài tứ giác sẽ không sinh fragment và bị #strong[cắt cụt thẳng đơ], phá hỏng chính dải mượt mà khử răng cưa tạo ra. Vùng $alpha > 0$ trải tới khi $"dist" = "aa"$ vượt ra ngoài mép nét, tức xa tâm một đoạn $"nửa bề rộng" + "aa"$. Vấn đề là bề rộng dải $"aa" = max(u\_"antialias", "fwidth"("dist"))$ #strong[không cố định]: ngay cả khi người dùng đặt tham số khử răng cưa bằng 0, số hạng $"fwidth"("dist")$ vẫn kéo dải rộng tối thiểu khoảng một điểm ảnh (xem mục sau). Nếu chỉ nới tứ giác đúng $"nửa bề rộng" + u\_"antialias"$ thì trong trường hợp $u\_"antialias"$ nhỏ hơn một điểm ảnh, phần dải do `fwidth` sinh ra sẽ tràn khỏi tứ giác và bị cắt. Số hạng $+1$ chính là biên dự phòng cho đúng trường hợp xấu nhất đó ($"fwidth" approx 1$ điểm ảnh), bảo đảm tứ giác luôn bao trọn dải khử răng cưa dù người dùng đặt tham số khử răng cưa nhỏ tới đâu.

Điểm tinh tế: cả bốn đỉnh của tứ giác đều #strong[mang theo cặp đầu mút thật $(p_0, p_1)$] làm thuộc tính đỉnh, và các giá trị này #strong[phẳng] (không nội suy khác nhau) trên toàn tứ giác. Nhờ đó mọi fragment đều nhận đúng cặp đầu mút của đoạn và đo khoảng cách tới #strong[trục thật], bất kể nó sinh ra từ góc nào. Một hệ quả có ích: khi $p_0 = p_1$ (đoạn dài 0), hàm khoảng cách rút gọn thành khoảng cách tới một điểm, nên đúng đường vẽ này cho ra một #strong[đĩa tròn] - cách marker và nét chấm được dựng (mục sau).

=== GPU: hàm khoảng cách và tô khử răng cưa

Trên GPU, hàm khoảng cách tới đoạn và phần tô khử răng cưa chỉ gồm vài dòng:

```glsl
float sdSegment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / max(dot(ba, ba), 1e-6), 0.0, 1.0);
    return length(pa - ba * h);   // khoảng cách tới điểm gần nhất trên đoạn
}
// ...
float dist  = sdSegment(v_pos, v_p0, v_p1) - u_halfWidth; // <0 trong nét, =0 tại mép
float aa    = max(u_antialias, fwidth(dist));             // dải >= 1 pixel
float alpha = 1.0 - smoothstep(0.0, aa, dist);           // 1 trong -> 0 ngoài
```

Diễn giải bằng lời để thấy vì sao đầu bo tròn "tự rơi ra":
+ Đại lượng $h$ (tỉ số giữa tích vô hướng `dot(pa, ba)` và `dot(ba, ba)`) là #strong[tham số hình chiếu] của $p$ dọc đoạn, biểu diễn bằng tỉ lệ: $0$ tại $A$, $1$ tại $B$, $0.5$ tại trung điểm.
+ Phép `clamp(h, 0, 1)` chính là #strong[toàn bộ cơ chế bo đầu]. Khi $h in (0, 1)$, điểm gần nhất nằm trên trục nên đo được khoảng cách vuông góc (thân thẳng); khi $h < 0$ hoặc $h > 1$, $h$ bị ép về $0$ hoặc $1$, điểm gần nhất trở thành đúng đầu mút $A$ hoặc $B$, nên đo được khoảng cách xuyên tâm (cung tròn). Chỉ một phép `clamp`, không rẽ nhánh, không cần hình học đầu bo riêng.
+ Trừ đi nửa bề rộng ($"dist" = "sdSegment" - u\_"halfWidth"$) dịch mức 0 từ trục ra mép nét: âm ở trong capsule, bằng 0 tại biên, dương ở ngoài. Tập $"dist" <= 0$ chính là capsule bo tròn.

Phần khử răng cưa dựa trên hai hàm GLSL. Hàm `smoothstep` tạo một dốc mượt $0 arrow 1$ rộng đúng $"aa"$ điểm ảnh vắt qua mép: độ phủ ($alpha$) bằng 1 ở trong, giảm dần về 0 khi ra ngoài. Dải mượt này #strong[chính là] phần khử răng cưa - độ phủ mép được tính #strong[giải tích], không phải lấy mẫu, nên sạch ở mọi mức phóng to. Hàm `fwidth(dist)` cho biết `dist` đổi bao nhiêu giữa hai điểm ảnh kề nhau, tức xấp xỉ một điểm ảnh đo theo đơn vị của `dist`; lấy $"aa" = max(u\_"antialias", "fwidth"("dist"))$ bảo đảm dải khử răng cưa #strong[rộng ít nhất một điểm ảnh] ngay cả khi người dùng đặt khử răng cưa bằng 0 hoặc phóng to rất sâu.

=== So sánh độ phức tạp với khử răng cưa đa mẫu

Kỹ thuật này có hai ưu điểm đo được so với MSAA của phần cứng. Thứ nhất về #strong[hình học]: CPU chỉ phát $O("số đoạn")$ tứ giác, không sinh thêm đỉnh cho đầu bo hay mối nối, và chi phí không phụ thuộc bề rộng nét. Thứ hai về #strong[chất lượng và chi phí điểm ảnh]: độ phủ mép được tính giải tích với chi phí $O(1)$ cho mỗi fragment, trong khi MSAA phải lấy $k$ mẫu con cho mỗi điểm ảnh biên (chi phí nhân lên theo $k$) và vẫn phụ thuộc phần cứng có hỗ trợ hay không. Kết quả là nét vẽ sắc nét, độc lập độ phân giải, và chạy được cả trên thiết bị nhúng không hỗ trợ tốt MSAA.

== Nét đứt, nét chấm và marker

Ba kiểu nét này đều tái sử dụng hình học capsule ở mục trước, chỉ khác nhau ở cách rải các phần tử dọc theo đường. Nền tảng chung của cả ba là cách tham số hóa đường.

=== Đi theo độ dài cung: nền tảng chung

Cả ba kiểu đều đặt các phần tử (đoạn nét, chấm, điểm đánh dấu) dọc đường theo #strong[độ dài cung Euclid] chứ không theo chỉ số đỉnh hay theo hoành độ $x$. Lý do nằm ở tính đều đặn thị giác: một đường gấp khúc gồm nhiều đoạn dài ngắn khác nhau và có góc cua. Nếu rải theo chỉ số đỉnh thì đoạn giữa hai điểm gần nhau sẽ có nét dày sít, đoạn giữa hai điểm xa nhau sẽ có nét thưa thớt; nếu rải theo $x$ thì các đoạn dốc (đổi nhiều theo $y$ trên một khoảng $x$ nhỏ) sẽ bị nén lại. Chỉ khi đo bằng #strong[quãng đường thực trên màn hình] thì khoảng cách giữa các nét hay chấm mới #strong[đều nhau về mặt thị giác] qua mọi góc cua và đoạn dốc. Việc duyệt là một lượt tuyến tính qua các đoạn, tích lũy dần quãng đường đã đi.

=== Nét đứt

Nét đứt lặp lại một chu kỳ gồm một đoạn nét và một khoảng hở, với độ dài chu kỳ $L_"pat" = L_"nét" + L_"hở"$. Thuật toán duyệt đường gấp khúc, tích lũy quãng đường $d$ đã đi từ đầu đường; tại mỗi vị trí, phần dư $"patternPos" = d mod L_"pat"$ cho biết đang ở trong pha "nét" hay pha "hở". Trong mỗi đoạn, thuật toán tiến từng bước nhỏ, mỗi bước dài bằng #strong[giá trị nhỏ hơn] giữa "quãng còn lại của pha hiện tại" và "quãng còn lại của đoạn hiện tại". Nhờ cách chọn bước này, ranh giới nét/hở luôn rơi đúng vị trí ngay cả khi nó nằm giữa một đoạn, và không đoạn con nào bị bỏ sót hay tính lặp. Các đoạn con thuộc pha "nét" được gom thành các #strong[đoạn chạy (run)] gồm từ hai điểm trở lên, rồi mỗi đoạn chạy được kết xuất thành capsule như mục trước.

Một chi tiết bù trừ quan trọng: vì mỗi capsule có hai đầu bo tròn kéo dài thêm về mặt thị giác một lượng $2 dot "nửa bề rộng nét"$, nếu lấy đúng $L_"nét"$ đặt ra thì đoạn nét in ra sẽ #strong[dài hơn] mong muốn. Do đó chiến lược #strong[trừ trước] phần này vào độ dài nét và cộng vào khoảng hở trước khi vẽ, để nét đứt hiển thị đúng tỉ lệ nét/hở đã đặt.

#figure(
  image("/assets/images/dash.drawio.png", width: 100%, height: auto),
  caption: [Hình học nét đứt: chu kỳ nét-hở rải đều theo độ dài cung, mỗi đoạn nét là một capsule],
) <fig-dash>

Về chi phí: mỗi đoạn được duyệt đúng một lần và bị chia thành nhiều nhất là "số ranh giới pha rơi vào đoạn đó" cộng một đoạn con. Cộng trên toàn đường, tổng chi phí là $O("số đoạn" + L \/ L_"pat")$ với $L$ là tổng chiều dài đường trên màn hình - tức tỉ lệ với số đoạn cộng số chu kỳ nét hiển thị, đều ở cỡ số pixel và độc lập với tổng số điểm $N$.

=== Nét chấm

Nét chấm rải một #strong[đĩa tròn] - chính là capsule suy biến với $p_0 = p_1$ đã nêu ở mục 5.4 - sau mỗi khoảng độ dài cung cố định $L_"step" = 2r + "hở"$, với $r$ là bán kính chấm. Tại mỗi mốc, tâm chấm được xác định bằng #strong[nội suy tuyến tính] trên đoạn đang đi qua: $"tâm" = a + (b - a) dot t$, trong đó $a$, $b$ là hai đầu đoạn hiện tại và $t$ là tỉ lệ quãng đường trong đoạn đó. Vì tái dùng đúng đường vẽ capsule, mỗi chấm cũng được khử răng cưa bằng SDF đường tròn mà không cần thêm cơ chế riêng.

#figure(
  image("/assets/images/dot.drawio.png", width: 100%, height: auto),
  caption: [Hình học nét chấm: mỗi chấm là một đĩa tròn (capsule suy biến) rải đều theo độ dài cung],
) <fig-dot>

Về chi phí: cũng là một lượt duyệt tuyến tính cộng số chấm phát ra, tức $O("số đoạn" + L \/ L_"step")$ - độc lập với $N$.

=== Marker

Điểm đánh dấu (marker) tại mỗi điểm dữ liệu là một tứ giác căn theo màn hình, tô bằng #strong[SDF đường tròn] (khoảng cách tới tâm trừ bán kính) với đúng cơ chế khử răng cưa dựa trên `smoothstep`/`fwidth` như nét vẽ. Chi phí là $O("số điểm vẽ")$.

Tổng hợp lại, cả ba kiểu chỉ cần một lượt duyệt tuyến tính qua các đoạn hoặc điểm đã vẽ, và số phần tử phát ra tỉ lệ với chiều dài đường trên màn hình (với nét đứt, nét chấm) hoặc số điểm vẽ (với marker). Tất cả đều ở cỡ số pixel, độc lập với tổng số điểm $N$, nhất quán với kết quả ở mục 5.1.

== Kết xuất biểu đồ cột và biểu đồ tròn

=== Biểu đồ cột

Mỗi cột là một tứ giác hai tam giác, dựng từ #strong[đường cơ sở] (giá trị dữ liệu $y = 0$) lên tới giá trị, tự co theo dải và bám theo zoom. Chiến lược tính một #strong[bề rộng dải] (`bandWidth`) trong đơn vị dữ liệu: đúng bằng $1$ với trục phân loại, hoặc bằng #strong[khoảng cách nhỏ nhất giữa hai giá trị $x$ kề nhau] với trục số. Bộ kết xuất ánh xạ bề rộng đó sang điểm ảnh theo dải trục #strong[hiện tại] ở mỗi khung:

$ "bandPx" = ("bandWidth" \/ "xSpan") dot "plot"_W, quad w_"hiệu dụng" = max("bandPx" dot 0.8, 1) $

Nhờ đó cột #strong[nở ra khi phóng to], co lại vừa vặn khi dày đặc, và không bao giờ biến mất (sàn 1 điểm ảnh). Đường cơ sở được chiếu một lần và mỗi cột trải từ nó lên tới đỉnh. Chiến lược cũng ép đường cơ sở vào dải Y để cột luôn neo tại 0, và lọc bỏ các cột có tâm nằm ngoài dải $x$ đang thấy.

=== Biểu đồ tròn

Biểu đồ tròn là một series #strong[phi Cartesian] (cột nhãn và cột giá trị), không có trục. Bộ lập kế hoạch bố cục gắn nó với hệ tọa độ riêng và áp hai ràng buộc cho dạng đơn giản hiện tại: không trộn lẫn với series XY, và mỗi biểu đồ chỉ một series tròn. Chiến lược cộng tổng các giá trị dương hữu hạn (bỏ giá trị $<= 0$ hoặc `NaN`) và phát một lát cho mỗi dòng với tỉ trọng $"giá trị" \/ "tổng"$; vì #strong[chuẩn hóa theo tổng], các lát luôn phủ kín vòng tròn.

Bộ kết xuất lưu góc theo radian, chiều kim đồng hồ tính từ đỉnh, nên điểm trên vành ứng với góc $a$ là:

$ p(a) = "tâm" + r dot (sin a, -cos a) $

trong đó dấu $-cos$ bù cho việc tọa độ màn hình tăng xuống dưới (để $a = 0$ ở vị trí 12 giờ), tâm là tâm vùng vẽ, bán kính $r = 0.5 dot min(w, h) dot 0.9$. Mỗi lát được chia thành một #strong[quạt tam giác] (một tam giác cho mỗi cung khoảng $2 degree$) với thuộc tính màu cho mỗi đỉnh, nên cả biểu đồ là một lệnh vẽ tam giác duy nhất. Hình học được dựng lại mỗi khung từ vùng vẽ hiện tại, nên biểu đồ tự canh tâm và co giãn khi đổi kích thước cửa sổ. Vì biểu đồ tròn không có chú giải theo series, chú giải được dựng thành một mục cho mỗi lát.

== Kết xuất trục và chữ bằng atlas texture

=== Đường trục và vạch chia

Đường trục và mỗi vạch chia là các tứ giác mỏng vẽ bằng tam giác, tô màu phẳng (không dùng SDF vì chúng thẳng theo trục và mỏng). Độ dài vạch bằng $4$ lần bề rộng trục.

=== Chữ qua atlas texture và bộ nhớ đệm

Chữ trên trục được rasterize (vẽ thành ảnh bitmap) #strong[một lần cho mỗi chuỗi nhãn khác nhau] vào một texture RGBA nhỏ, rồi vẽ như một tứ giác dán texture; không có việc tạo hình chữ trực tiếp ở mỗi khung. Ảnh được vẽ theo tỉ lệ điểm ảnh thiết bị (DPR) để sắc nét trên màn hình mật độ cao (HiDPI). Điểm mấu chốt về hiệu năng là bộ nhớ đệm: nhãn được ghi nhớ theo chuỗi, nên các nhãn lặp lại (và các lần vẽ lại) #strong[gần như không tốn chi phí] - lần rasterize đầu là $O("số điểm ảnh của nhãn")$, các lần sau chỉ là một lần tra cứu $O(1)$ và một lần vẽ tứ giác.

=== Giảm mật độ nhãn chống chồng chữ

Với hàng trăm nhãn phân loại, chữ sẽ chồng lên nhau thành một vệt không đọc được. Giải pháp là một thuật toán #strong[giảm mật độ tham lam]: duyệt các nhãn theo thứ tự dọc trục và chỉ giữ một nhãn nếu nó không đè lên nhãn được giữ gần nhất (hai nhãn căn giữa đè nhau khi khoảng cách tâm nhỏ hơn nửa tổng bề rộng cộng khoảng đệm). Cách này cho một tập con thưa đều và đọc được, với chi phí $O("số nhãn")$ - một lượt duyệt tuyến tính.

== Tích hợp scene graph và trộn màu

`ChartRenderNode` là một `QSGRenderNode`@qt_qsgrendernode - nó chạy #strong[lệnh OpenGL thô bên trong scene graph] của Qt Quick. Nút này khai báo những trạng thái GL mà nó chạm tới (viewport, scissor, màu, trộn) để Qt khôi phục lại sau khi vẽ; và tính ma trận `mvp` từ phép chiếu do Qt cấp - vì các bộ kết xuất đã làm việc theo pixel, ma trận này chỉ còn việc đưa pixel sang không gian cắt.

Về trộn màu, hệ thống dùng #strong[alpha nhân trước (premultiplied alpha)]@wiki_alpha_compositing với hàm trộn tương ứng. Mọi shader đều xuất màu ở dạng nhân trước. Cách này tránh được viền tối ở mép khử răng cưa mà kiểu trộn alpha thẳng thường tạo ra: khi độ phủ mép nhỏ, màu đã được nhân sẵn với độ phủ nên không kéo lẫn màu nền tối vào biên.

== Trỏ xem chi tiết và điều tiết cập nhật

=== Dò trúng điểm khi trỏ chuột

Cơ chế trỏ xem chi tiết (hover) chia làm hai phần: dò trúng điểm ở tầng C++ và lớp phủ hiển thị tùy chọn ở tầng QML. Ở mỗi lần dựng lại, `ChartView` giữ một bản sao các #strong[điểm đã vẽ] (có thể đã giảm mẫu) của mỗi series đang hiện, nên dò trúng luôn khớp với thứ đang thực sự trên màn hình. Khi trỏ chuột, nó tìm điểm đã vẽ gần con trỏ nhất trong một bán kính (mặc định $24$ điểm ảnh), so theo #strong[bình phương khoảng cách] để tránh phép căn bậc hai, rồi công bố một thuộc tính mô tả điểm đó cho lớp phủ QML vẽ chỉ báo và hộp thông tin. Chi phí dò trúng là $O("số điểm đã vẽ")$, mà số điểm đã vẽ ở cỡ số pixel, nên độc lập với tổng số điểm. Phép dò được #strong[đánh giá lại sau mỗi lần dựng lại] tại vị trí chuột gần nhất, để chỉ báo vẫn dính vào điểm sau khi zoom hay dòng cập nhật ngay cả khi chuột đứng yên.

=== Gộp và điều tiết theo tần số khung

Nhiều nguồn sự kiện (ảnh chụp dòng, lăn chuột, kéo, đổi kích thước, bật/tắt chú giải) có thể yêu cầu vẽ lại nhanh hơn nhiều so với tần số làm tươi màn hình. `ChartView` #strong[gộp (coalesce)] các yêu cầu này và giới hạn ở một tần số khung (FPS) đặt được. Khoảng cách tối thiểu giữa hai khung là $"interval" = 1000 \/ "clamp"("fps", 1, 240)$ mili-giây: nếu đủ thời gian đã trôi qua thì vẽ ngay, nếu chưa thì hẹn một lần vẽ tại đúng mốc thời gian. Một cờ "đang chờ" bảo đảm mỗi lúc chỉ có một lần cập nhật xếp hàng. Nhờ vậy, một cụm $N$ sự kiện trong cùng một khung được gộp thành #strong[đúng một] lần dựng lại và một khung GL.

Một chi tiết quan trọng: các series #strong[đang ẩn vẫn được dựng] (`build()`) ở mỗi khung. Nếu bỏ qua chúng, phần dữ liệu chưa xử lý sẽ phình vô hạn trong lúc nạp (bộ tính biên phải quét lại một dải ngày càng lớn, thành chi phí bậc hai theo cả quá trình nạp), và khi hiện lại sẽ phải bù đồng bộ một lần rất lớn. Do đó `build()` luôn chạy để giữ bộ nhớ đệm điểm cập nhật; chỉ phần #strong[phát ra] (dữ liệu kết xuất, mục hover, tự co giãn Y) là được bỏ qua với series ẩn.

== Tổng hợp chi phí kết xuất một khung

Gộp lại toàn bộ, chi phí kết xuất một khung được phân tách theo từng khâu, với $P$ là số điểm ảnh của vùng vẽ (xấp xỉ số điểm và số fragment thực vẽ) và $N$ là tổng số điểm dữ liệu:
- #strong[Dựng hình học trên CPU] (ánh xạ tọa độ, phát tứ giác capsule/cột/quạt): $O(P)$ - tỉ lệ với số phần tử vẽ, mà số này đã được Chương 4 khống chế ở cỡ số pixel.
- #strong[Tô trên GPU] (khử răng cưa SDF, trộn màu): $O(1)$ mỗi fragment, tức $O(P)$ mỗi khung; không lấy mẫu bội như MSAA.
- #strong[Chữ trục]: $O(1)$ nhờ bộ nhớ đệm nhãn (chỉ lần đầu tốn $O("số điểm ảnh của nhãn")$), giảm mật độ $O("số nhãn")$.
- #strong[Dò trúng điểm khi hover]: $O(P)$.
- #strong[Gộp sự kiện]: một cụm $N_"sự kiện"$ yêu cầu trong một khung thu về #strong[một] lần dựng lại.

Điểm mấu chốt là toàn bộ đường đi ra phụ thuộc vào $P$ (số điểm ảnh trên màn hình) chứ #strong[không phụ thuộc vào tổng số điểm dữ liệu $N$]. Kết hợp với kết quả của Chương 4 - nơi đường dữ liệu vào cũng đạt $O(P)$ cho mỗi khung tương tác - cả hệ thống có chi phí mỗi khung ở cỡ số điểm ảnh, độc lập với $N$. Đây là cơ sở định lượng đầy đủ cho tuyên bố xuyên suốt: biểu đồ giữ được độ mượt như nhau dù dữ liệu là một triệu hay một trăm triệu điểm.
