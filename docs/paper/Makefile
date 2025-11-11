# IEEE LaTeX Build System
LATEX = pdflatex
BIBTEX = bibtex
TARGET = paper
SRC = $(TARGET).tex
BIB = references.bib
PDF = $(TARGET).pdf

.PHONY: all pdf clean watch quick

# Full build with bibliography
all: pdf

pdf: $(SRC) $(BIB)
	$(LATEX) $(TARGET)
	$(BIBTEX) $(TARGET)
	$(LATEX) $(TARGET)
	$(LATEX) $(TARGET)
	@echo "✓ PDF generated: $(PDF)"

# Quick compile (no bib update)
quick: $(SRC)
	$(LATEX) $(TARGET)
	@echo "✓ Quick PDF generated: $(PDF)"

# Clean auxiliary files
clean:
	rm -f *.aux *.bbl *.blg *.log *.out *.toc *.lof *.lot *.fls *.fdb_latexmk *.synctex.gz
	@echo "✓ Cleaned auxiliary files"

# Clean everything including PDF
distclean: clean
	rm -f $(PDF)
	@echo "✓ Cleaned all generated files"

# Auto-rebuild on changes (requires inotify-tools)
watch:
	@echo "Watching for changes... (Ctrl+C to stop)"
	@while inotifywait -e modify $(SRC) $(BIB) 2>/dev/null; do \
		make quick; \
	done

# Count words in document (approximate)
wordcount:
	@texcount -inc -incbib $(SRC)

# Check for common LaTeX errors
check:
	@echo "Checking for common issues..."
	@grep -n "\\cite{}" $(SRC) && echo "⚠ Empty citations found" || echo "✓ No empty citations"
	@grep -n "???" $(TARGET).log 2>/dev/null && echo "⚠ Undefined references" || echo "✓ All references defined"