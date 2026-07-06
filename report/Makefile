SOURCE = main.typ
OUT_DIR = output
BUILD_DIR = build
DIAGRAMS_SRC = diagrams
DIAGRAMS_OUT = $(BUILD_DIR)/diagrams
FONT_DIR = assets/fonts

all: clean 
	@mkdir -p $(OUT_DIR)
	@TITLE=$$(typst query --font-path $(FONT_DIR) --ignore-system-fonts $(SOURCE) "<doc_title>" --field value --one | tr -d '"'); \
	typst compile --font-path $(FONT_DIR) $(SOURCE) "$(OUT_DIR)/$$TITLE.pdf"

compile: clean-output
	@mkdir -p $(OUT_DIR)
	@TITLE=$$(typst query --font-path $(FONT_DIR) --ignore-system-fonts $(SOURCE) "<doc_title>" --field value --one | tr -d '"'); \
	typst compile --font-path $(FONT_DIR) $(SOURCE) "$(OUT_DIR)/$$TITLE.pdf"

clean: clean-build clean-output

clean-build:
	rm -rf $(BUILD_DIR)

clean-output:
	rm -rf $(OUT_DIR)
	find . -name "*.pdf" -type f -delete # tinymist produces many as well.

check-custom-font:
	typst fonts --font-path $(FONT_DIR) --variants --ignore-system-fonts --ignore-embedded-fonts

fresh: clean all

.PHONY: all clean fresh check-custom-font compile clean-build clean-output
