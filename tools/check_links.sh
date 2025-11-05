#!/bin/bash
# Check for broken links in markdown files

set -e

echo "=== Checking Markdown Links ==="
echo

find . -name "*.md" -type f | while read file; do
  echo "Checking: $file"
  # Extract markdown links
  grep -oP '\[.*?\]\(\K[^)]+' "$file" 2>/dev/null | while read link; do
    if [[ $link =~ ^http ]]; then
      # Skip external links
      continue
    else
      # Check local file exists
      target=$(dirname "$file")/$link
      if [ ! -f "$target" ]; then
        echo "  ✗ Broken: $link"
      fi
    fi
  done
done

echo