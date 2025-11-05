# TinyOL-HITL Paper Build System

PANDOC = pandoc
PDFOPTS = --pdf-engine=xelatex --number-sections --variable=geometry:margin=1in
BIBOPTS = --bibliography references.bib --csl ieee.csl --citeproc

.PHONY: all pdf docx latex clean watch

all: pdf

pdf: paper.md references.bib metadata.yaml
	$(PANDOC) metadata.yaml paper.md \
		--from markdown \
		--to pdf \
		--output build/paper.pdf \
		$(BIBOPTS) $(PDFOPTS)
	@echo "✓ PDF generated: build/paper.pdf"

docx: paper.md references.bib metadata.yaml
	$(PANDOC) metadata.yaml paper.md \
		--from markdown \
		--to docx \
		--output build/paper.docx \
		$(BIBOPTS)
	@echo "✓ DOCX generated: build/paper.docx"

latex: paper.md references.bib metadata.yaml
	$(PANDOC) metadata.yaml paper.md \
		--from markdown \
		--to latex \
		--output build/paper.tex \
		$(BIBOPTS) \
		--standalone
	@echo "✓ LaTeX generated: build/paper.tex"

# For IEEE conference submission (requires template)
ieee: paper.md references.bib metadata.yaml templates/ieee.latex
	$(PANDOC) metadata.yaml paper.md \
		--from markdown \
		--to pdf \
		--output build/paper_ieee.pdf \
		$(BIBOPTS) \
		--template templates/ieee.latex \
		--pdf-engine=xelatex
	@echo "✓ IEEE format: build/paper_ieee.pdf"

# Check for uncited percentages (common error)
check:
	@echo "Checking for uncited quantitative claims..."
	@grep -n "%" paper.md | grep -v "@" || echo "✓ All percentages cited"

clean:
	rm -rf build/*

# Auto-rebuild on file changes (requires inotify-tools)
watch:
	@echo "Watching for changes... (Ctrl+C to stop)"
	@while inotifywait -e modify paper.md references.bib 2>/dev/null; do \
		make pdf; \
	done