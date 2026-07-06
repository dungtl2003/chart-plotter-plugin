#import "metadata.typ": logo-path

#import "@preview/codly:1.3.0": *
#import "@preview/codly-languages:0.1.1": *

#let font-family = "SVN-Times New Roman"
#let p-spacing = 1.1em
#let base-font-size = 14pt
#let indent-size = 1cm
#let overlay(img, color) = layout(bounds => {
  let size = measure(img, ..bounds)
  img
  place(top + left, block(..size, fill: color))
})
#let is-appendix = state("is-appendix", false)
#let appendix(body) = {
  is-appendix.update(true)
  set heading(numbering: "1", supplement: [Appendix])
  counter(heading).update(0)
  body
}
#let project-margin = (left: 2cm, right: 2cm, top: 2cm, bottom: 2cm)

#let project(
  project-name: "",
  body,
) = {
  set page(
    paper: "a4",
    margin: project-margin,
    numbering: "i",
    number-align: center,
    // background: place(
    //   center + horizon,
    //   overlay(image(logo-path, width: 50%), white.transparentize(12%)),
    // ),
  )

  set text(font: font-family, size: base-font-size, lang: "vi", region: "VN")

  set par(
    justify: true,
    first-line-indent: (amount: indent-size, all: true),
    leading: 0.7em,
    spacing: p-spacing,
  )

  set list(indent: indent-size, body-indent: 0.6em)
  set enum(indent: indent-size, body-indent: 0.6em)
  show list: set par(first-line-indent: 0pt)
  show enum: set par(first-line-indent: 0pt)
  set terms(indent: indent-size)
  show terms: set par(first-line-indent: 0pt)
  show terms.item: it => {
    it.term
    terms.separator
    it.description
    linebreak()
  }

  show figure.caption: set align(center)
  show figure.caption: set text(style: "italic")
  show figure.caption: set block(sticky: true)
  set figure.caption(separator: [ ])
  show figure.where(kind: table): set figure.caption(position: top)
  show figure.where(kind: table): set block(breakable: true)
  show figure.where(kind: image): set figure.caption(position: bottom)
  set figure(numbering: num => {
    let h-counter = counter(heading).get()

    let chapter = if h-counter.len() > 0 and h-counter.first() > 0 {
      h-counter.first()
    } else {
      1
    }

    [#chapter.#num.]
  })

  set outline(indent: auto)
  set outline.entry(fill: repeat[ . ])
  show outline.entry.where(level: 1): it => context {
    if it.element.func() != heading {
      return it
    }

    // Check if the heading at this location is inside the appendix
    let in-appendix = is-appendix.at(it.element.location())

    let num = if it.element.numbering != none {
      let n = numbering(it.element.numbering, ..counter(heading).at(it.element.location()))
      [#n]
    } else { none }

    // Determine the correct prefix based on the state
    let prefix = if in-appendix { "Phụ lục" } else { "Chương" }

    let body = if num != none {
      upper(text(weight: "bold", [#prefix #num #it.body()]))
    } else {
      upper(text(weight: "bold", it.body()))
    }

    link(
      it.element.location(),
      it.indented(
        none,
        [
          #body
          #box(width: 1fr, it.fill)
          #text(it.page())
        ],
      ),
    )
  }

  set heading(numbering: "1.1.")

  show heading.where(level: 1): it => {
    pagebreak(weak: true)

    counter(figure.where(kind: image)).update(0)
    counter(figure.where(kind: table)).update(0)

    set align(center)
    set text(weight: "bold", size: base-font-size)
    set par(first-line-indent: 0pt)

    let in-appendix = is-appendix.get()

    if in-appendix {
      if it.numbering != none {
        let num = counter(heading).display(it.numbering)
        [Phụ lục #num #upper(it.body)]
      } else {
        upper(it.body)
      }
    } else {
      if it.numbering != none {
        let num = counter(heading).display(it.numbering)
        [CHƯƠNG #num #upper(it.body)]
      } else {
        upper(it.body)
      }
    }
  }

  show heading.where(level: 2): it => {
    block(sticky: true)[
      #set text(weight: "bold", size: base-font-size)
      #set par(first-line-indent: 0pt)
      #counter(heading).display() #it.body
    ]
  }

  show heading.where(level: 3): it => {
    block(sticky: true)[
      #set text(weight: "bold", size: base-font-size)
      #set par(first-line-indent: 0pt)
      #counter(heading).display() #it.body
    ]
  }

  show heading.where(level: 4): it => {
    block(sticky: true)[
      #set text(weight: "bold", size: base-font-size)
      #set par(first-line-indent: 0pt)
      #counter(heading).display() #it.body
    ]
  }

  show: codly-init.with()
  codly(
    languages: codly-languages,
    // number-format: none,
    stroke: 1pt + black, // Border around the block
    // radius: 4pt, // Rounded corners
    radius: 0pt, // Rounded corners
    inset: (top: 0.2em, bottom: 0.2em, left: 0.5em, right: 0.5em),
    zebra-fill: none, // Disable alternating line colors if preferred
    display-icon: false,
    display-name: false,
  )

  body
}
