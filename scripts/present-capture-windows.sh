#!/bin/bash
screen=1

mkdir -p present-tmp

pages=""
for page in `seq $1`
do
    sleep 4
    echo "Capturing number $page in 1s..."
    sleep 1
    file="present-tmp/$page.pdf"
    convert screenshot:[$screen] -trim +repage $file
    pages="$pages $file"
    echo "$page done!"
done

echo "Done! Merging pdf..."
gs -dBATCH -dNOPAUSE -q -sDEVICE=pdfwrite -sOutputFile=present.pdf $pages

rm -rf present-tmp
