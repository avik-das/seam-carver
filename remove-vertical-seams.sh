#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "USAGE: $0 <input-filename> <num-vertical-seams-to-remove>"
    exit 1
fi

echo "removing $2 vertical seams from $1"

make

rm -rf out
mkdir out

./seam-carver $1 out $2

echo ---
echo 'Creating seams animation'

# The libx264 encoder requires an even number for each of the width and height
# coordinates. Read the original dimensions, then scale them up to the next
# even number.
read w h < <(identify -format "%w %h" "$1")
w=$(echo "($w + 1)" '/ 2 * 2' | bc)
h=$(echo "($h + 1)" '/ 2 * 2' | bc)

# Options:
#   - 10 frames per second
#   - Read seam images as the input.
#   - Use libx264 for the encoding
#   - Pad the smaller images to the full video width, coloring in the remainder
#     of the frame black (default value). The smaller image goes at the
#     top-left of the frame.
#   - H.264 Constant Rate Factor. Sets the output quality. The value has been
#     chosen experimentally.
#   - Pixel format. Chosen so the video plays in Firefox.
ffmpeg \
    -r 10 \
    -i out/img-seam-%04d.jpg \
    -vcodec libx264 \
    -vf "pad=$w:$h:0:0" \
    -crf 25 \
    -pix_fmt yuv420p \
    out/animation-seams.mp4
