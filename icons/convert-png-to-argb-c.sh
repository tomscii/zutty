#!/bin/sh
# Uses ImageMagick to convert PNG images to 32 packed cardinal ARGB
# as an array in C code for use with EWMH _NET_WM_ICON property

convert_png () {
  identify $1 > /dev/null
  if [ $? -eq 0 ]
  then
    identify -format '%w, %h,\n' $1
    convert $1 -color-matrix '0 0 1 0, 0 1 0 0, 1 0 0 0, 0 0 0 1' RGBA:- | hexdump -v -e '1/4 "0x%08x,\n"'
  fi
}

if [ "$#" -eq 0 ]
then
  exit 1
fi

echo 'const unsigned long zutty_icons[] = {'
while [ ! -z "$1" ]
do
  convert_png $1
  shift
done
echo '};'
