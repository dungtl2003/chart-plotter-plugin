#import "template.typ": project, project-margin
#import "/frontmatter/cover.typ": cover-page
#import "metadata.typ": doc-title, project-name, version

#show: project.with(
  project-name: project-name,
)
#metadata(doc-title) <doc_title>

#cover-page(margin: project-margin)

#set page(numbering: "i")
#counter(page).update(1)

#include "/frontmatter/outline.typ"
#include "/frontmatter/symbols.typ"
#include "/frontmatter/lists.typ"
// #include "/frontmatter/thanks.typ"
#include "/frontmatter/preface.typ"

#set page(numbering: "1")
#counter(page).update(1)

#include "/chapters/01-overview-and-value.typ"
#include "/chapters/02-technology-foundations.typ"
#include "/chapters/03-architecture-design.typ"
#include "/chapters/04-data-processing-and-performance.typ"
#include "/chapters/05-gpu-rendering.typ"
#include "/chapters/06-implementation-and-evaluation.typ"

#include "/backmatter/conclusion.typ"
#include "/backmatter/refs.typ"
// #include "/backmatter/appendix.typ"
