TEX_FILES = \
	main.tex \
	src/*.tex
CLASS_FILES = elegantbook.cls
BIB_FILES = reference.bib
IMAGES = image/*.png generated/*/*.pdf
OUTPUT_DIR = .build

.PHONY: all
all: main.pdf

main.pdf: $(TEX_FILES) $(CLASS_FILES) $(IMAGES) $(BIB_FILES)
	mkdir -p $(OUTPUT_DIR)
	xelatex -output-directory=$(OUTPUT_DIR) $<            || (make clean && exit 5)
	biber --output-directory=$(OUTPUT_DIR) $(basename $<) || (make clean && exit 6)
	xelatex -output-directory=$(OUTPUT_DIR) $<            || (make clean && exit 7)
	xelatex -output-directory=$(OUTPUT_DIR) $<            || (make clean && exit 8)
	mv $(OUTPUT_DIR)/$@ .

.PHONY: clean
clean:
	rm -rf $(OUTPUT_DIR)/*
