#import "/metadata.typ": author, logo-path, mentor, project-name

#let font-family = "SVN-Times New Roman"

#let cover-page(margin: (left: 2cm, right: 2cm, top: 2cm, bottom: 2cm)) = {
  set page(
    paper: "a4",
    margin: margin,
    numbering: none,
    background: none,
  )

  set text(font: font-family, size: 14pt, lang: "vi")
  set par(leading: 0.6em, justify: false)

  place(layout(size => {
    let w = size.width
    let h = size.height
    rect(width: 100%, height: 100%, stroke: 0.5pt)
    place(center + horizon, rect(width: 100% - 2mm, height: 100% - 2mm, stroke: 3pt))
  }))

  align(center)[
    #v(0.5cm)

    #text(weight: "bold", size: 14pt)[TẬP ĐOÀN CÔNG NGHIỆP - VIỄN THÔNG QUÂN ĐỘI]
    #v(-0.5em)
    #line(length: 50%, stroke: 0.5pt)

    #v(1cm)

    #if logo-path != none {
      image(logo-path, width: 9.5cm)
    } else {
      box(height: 3.5cm, align(center + horizon)[
        #circle(radius: 1.6cm, fill: white, stroke: 1.5pt + red)
        #place(center + horizon, text(fill: red, weight: "bold")[LOGO])
      ])
    }

    #v(1cm)

    BÁO CÁO MINI-PROJECT

    #v(0.3cm)
    #block(width: 80%)[
      #text(size: 16pt, weight: "bold")[
        #upper[#project-name]
      ]
    ]

    #v(1.2cm)

    Chương trình Viettel Digital Talent 2026 \
    Lĩnh vực: System Programming

    #v(3cm)

    #align(left)[
      #pad(left: 1.2cm)[

        #text(style: "italic")[Sinh viên thực hiện:] \
        #v(-0.2cm)
        #pad(left: 2cm)[
          *Họ và tên: #author.name* \
          *Email: #author.email*
        ]

        #v(0.8em)

        #text(style: "italic")[Người hướng dẫn:] \
        #v(-0.2cm)
        #pad(left: 2cm)[
          *Họ và tên: #mentor.name* \
          *Đơn vị: #mentor.unit*
        ]
      ]
    ]

    #v(1fr)

    *Hà Nội, 2026*
    #v(0.5cm)
  ]
  pagebreak()
}
