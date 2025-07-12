#!/bin/bash

for filename in code/*.*; do
    [ -e "$filename" ] || continue # skip pathological case
    [ ${filename: -4} != ".tex" ] || continue # skip .tex files
    
    echo pygmentize -f latex $filename
    echo "{\footnotesize" > "$filename.tex"
    pygmentize -f latex $filename >> "$filename.tex"
    echo "}" >> "$filename.tex"
    sed -i -e 's/^/ /' "$filename.tex"
done
