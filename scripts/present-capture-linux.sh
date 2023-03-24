#!/bin/bash

echo "Pick a window..."
id=$(xwininfo | grep "Window id:" | grep --only-matching "0x[0-9A-Fa-f]*")

mkdir -p present-tmp

pages=""
for page in `seq $1`
do
    sleep 4
    echo "Capturing number $page in 1s..."
    sleep 1
    file="present-tmp/$page.pdf"
    import -window $id "$file"
    #convert screenshot:[$screen] -trim +repage $file
    pages="$pages $file"
    echo "$page done!"
done

echo "Done! Merging pdf..."
gs -dBATCH -dNOPAUSE -q -sDEVICE=pdfwrite -sOutputFile=present.pdf $pages

rm -rf present-tmp
