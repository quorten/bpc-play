@echo off
if %1_==-e_ (
goto extract
) else (
if %1_==-b_ (
goto build
) else (
goto ussage
)
)

:extract
icoextract up.cur
rename "and0.bmp" "AND of up.bmp"
rename "xor0.bmp" "XOR of up.bmp"
icoextract down.cur
rename "and0.bmp" "AND of down.bmp"
rename "xor0.bmp" "XOR of down.bmp"
icoextract left.cur
rename "and0.bmp" "AND of left.bmp"
rename "xor0.bmp" "XOR of left.bmp"
icoextract right.cur
rename "and0.bmp" "AND of right.bmp"
rename "xor0.bmp" "XOR of right.bmp"
icoextract up-left.cur
rename "and0.bmp" "AND of up-left.bmp"
rename "xor0.bmp" "XOR of up-left.bmp"
icoextract up-right.cur
rename "and0.bmp" "AND of up-right.bmp"
rename "xor0.bmp" "XOR of up-right.bmp"
icoextract down-left.cur
rename "and0.bmp" "AND of down-left.bmp"
rename "xor0.bmp" "XOR of down-left.bmp"
icoextract down-right.cur
rename "and0.bmp" "AND of down-right.bmp"
rename "xor0.bmp" "XOR of down-right.bmp"
icoextract vertframe.cur
rename "and0.bmp" "AND of vertframe.bmp"
rename "xor0.bmp" "XOR of vertframe.bmp"
icoextract horzframe.cur
rename "and0.bmp" "AND of horzframe.bmp"
rename "xor0.bmp" "XOR of horzframe.bmp"
icoextract isearch.cur
rename "and0.bmp" "AND of isearch.bmp"
rename "xor0.bmp" "XOR of isearch.bmp"
icoextract r-isearch.cur
rename "and0.bmp" "AND of r-isearch.bmp"
rename "xor0.bmp" "XOR of r-isearch.bmp"
icoextract center.cur
rename "and0.bmp" "AND of center.bmp"
rename "xor0.bmp" "XOR of center.bmp"
icoextract vertcenter.cur
rename "and0.bmp" "AND of vertcenter.bmp"
rename "xor0.bmp" "XOR of vertcenter.bmp"
icoextract horzcenter.cur
rename "and0.bmp" "AND of horzcenter.bmp"
rename "xor0.bmp" "XOR of horzcenter.bmp"
del *.cur
goto EOF

:build
rename "AND of up.bmp" "and0.bmp"
rename "XOR of up.bmp" "xor0.bmp"
icobuild 7 12
rename "output.cur" "up.cur"
del and0.bmp xor0.bmp
rename "AND of down.bmp" "and0.bmp"
rename "XOR of down.bmp" "xor0.bmp"
icobuild 7 2
rename "output.cur" "down.cur"
del and0.bmp xor0.bmp
rename "AND of left.bmp" "and0.bmp"
rename "XOR of left.bmp" "xor0.bmp"
icobuild 12 7
rename "output.cur" "left.cur"
del and0.bmp xor0.bmp
rename "AND of right.bmp" "and0.bmp"
rename "XOR of right.bmp" "xor0.bmp"
icobuild 2 7
rename "output.cur" "right.cur"
del and0.bmp xor0.bmp
rename "AND of up-left.bmp" "and0.bmp"
rename "XOR of up-left.bmp" "xor0.bmp"
icobuild 9 9
rename "output.cur" "up-left.cur"
del and0.bmp xor0.bmp
rename "AND of up-right.bmp" "and0.bmp"
rename "XOR of up-right.bmp" "xor0.bmp"
icobuild 2 9
rename "output.cur" "up-right.cur"
del and0.bmp xor0.bmp
rename "AND of down-left.bmp" "and0.bmp"
rename "XOR of down-left.bmp" "xor0.bmp"
icobuild 9 2
rename "output.cur" "down-left.cur"
del and0.bmp xor0.bmp
rename "AND of down-right.bmp" "and0.bmp"
rename "XOR of down-right.bmp" "xor0.bmp"
icobuild 2 2
rename "output.cur" "down-right.cur"
del and0.bmp xor0.bmp
rename "AND of vertframe.bmp" "and0.bmp"
rename "XOR of vertframe.bmp" "xor0.bmp"
icobuild 8 8
rename "output.cur" "vertframe.cur"
del and0.bmp xor0.bmp
rename "AND of horzframe.bmp" "and0.bmp"
rename "XOR of horzframe.bmp" "xor0.bmp"
icobuild 8 8
rename "output.cur" "horzframe.cur"
del and0.bmp xor0.bmp
rename "AND of isearch.bmp" "and0.bmp"
rename "XOR of isearch.bmp" "xor0.bmp"
icobuild 6 22
rename "output.cur" "isearch.cur"
del and0.bmp xor0.bmp
rename "AND of r-isearch.bmp" "and0.bmp"
rename "XOR of r-isearch.bmp" "xor0.bmp"
icobuild 6 0
rename "output.cur" "r-isearch.cur"
del and0.bmp xor0.bmp
rename "AND of center.bmp" "and0.bmp"
rename "XOR of center.bmp" "xor0.bmp"
icobuild 12 12
rename "output.cur" "center.cur"
del and0.bmp xor0.bmp
rename "AND of vertcenter.bmp" "and0.bmp"
rename "XOR of vertcenter.bmp" "xor0.bmp"
icobuild 7 12
rename "output.cur" "vertcenter.cur"
del and0.bmp xor0.bmp
rename "AND of horzcenter.bmp" "and0.bmp"
rename "XOR of horzcenter.bmp" "xor0.bmp"
icobuild 12 7
rename "output.cur" "horzcenter.cur"
del and0.bmp xor0.bmp
del hotspots.txt
goto EOF

:ussage
echo You may use one of the following switches:
echo -e      Extract the bitmaps from the cursors.
echo -b      Build the cursors from the bitmaps.
echo.

:EOF
