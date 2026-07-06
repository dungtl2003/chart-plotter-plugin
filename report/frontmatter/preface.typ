#heading(numbering: none)[Lời nói đầu]

Nhu cầu trực quan hóa dữ liệu chuỗi thời gian quy mô lớn - từ tín hiệu đo lường từ xa, dữ liệu cảm biến cho tới log vận hành - ngày càng trở nên cấp thiết tại các đơn vị kỹ thuật cao. Tuy nhiên, các thư viện vẽ biểu đồ sẵn có thường hoặc nghẽn ở quy mô hàng triệu điểm, hoặc kéo theo phụ thuộc và ràng buộc giấy phép nặng nề. Đề tài "Xây dựng plugin vẽ biểu đồ sử dụng C++ và Qt Framework" hướng tới một lời giải gọn nhẹ, hiệu năng cao và tự chứa cho bài toán đó.

Báo cáo được chia làm sáu chương cụ thể như sau:

#let ch(no, title, desc) = block[
  #set par(first-line-indent: 0pt)
  #grid(
    columns: (auto, 1fr),
    gutter: 1em,
    [#strong[CHƯƠNG #no.]],
    [
      #strong[#title]

      #desc
    ],
  )
]

#ch("1", "Tổng quan đề tài và giá trị mang lại")[
  Trình bày bối cảnh, bài toán và các thách thức kỹ thuật; phân tích hạn chế của các giải pháp hiện có và làm rõ giá trị, tính mới của sản phẩm; xác định mục tiêu và phạm vi đề tài.
]

#ch("2", "Cơ sở công nghệ và kỹ thuật nền tảng")[
  Giới thiệu Qt và QML, kiến trúc Qt Quick Scene Graph tích hợp OpenGL, các mẫu thiết kế được áp dụng và nền tảng lý thuyết về giảm mẫu cùng khử răng cưa bằng trường khoảng cách có dấu.
]

#ch("3", "Phân tích yêu cầu và thiết kế kiến trúc tổng thể hệ thống")[
  Phân tích yêu cầu chức năng, phi chức năng và trình bày thiết kế kiến trúc: luồng dữ liệu một chiều, mô hình đa luồng, điểm nối mở rộng và giao diện lập trình phía QML.
]

#ch("4", "Kỹ thuật xử lý dữ liệu và tối ưu hiệu năng")[
  Đi sâu vào phần backend: nạp dữ liệu tốc độ cao, lưu trữ theo cột, bộ nhớ đệm điểm tăng tiến và các thuật toán giảm mẫu cho tập dữ liệu hàng triệu điểm.
]

#ch("5", "Kỹ thuật kết xuất đồ họa trên GPU")[
  Trình bày ánh xạ tọa độ, kết xuất nét bằng trường khoảng cách có dấu, sinh trục, tích hợp scene graph và các thao tác tương tác trên biểu đồ.
]

#ch("6", "Triển khai, thử nghiệm và đánh giá")[
  Mô tả môi trường triển khai, bộ ví dụ minh họa, kết quả đo hiệu năng và đánh giá mức độ đáp ứng so với các mục tiêu đã đề ra.
]
