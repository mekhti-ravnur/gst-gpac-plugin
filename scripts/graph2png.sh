#!/bin/bash

# Change to the directory containing the .dot files
if [ -z "$1" ]; then
  echo "Usage: $0 <directory>"
  exit 1
fi
CWD=$(pwd)
cd "$1" || exit 1

if [ -z "$(ls *.dot 2>/dev/null)" ]; then
  echo "No .dot files found in the current directory."
  exit 1
fi

# Remove all .png files
if [ -n "$(ls *.png 2>/dev/null)" ]; then
  rm *.png
fi

# Convert all .dot files to .png
for file in *.dot; do
  # Remove date prefix from the file name
  new_file=$(echo "$file" | sed 's/^[0-9]\.[0-9]\{2\}\.[0-9]\{2\}\.[0-9]\{9\}-//')
  dot -Tpng "$file" -o "${new_file%.dot}.png"
done

# Remove all .dot files
rm *.dot

# Change back to the original directory
cd "$CWD" || exit 1
