mkdir RespeQt.iconset
sips -z 16 16     RespeQt.png --out RespeQt.iconset/icon_16x16.png
sips -z 32 32     RespeQt.png --out RespeQt.iconset/icon_16x16@2x.png
sips -z 32 32     RespeQt.png --out RespeQt.iconset/icon_32x32.png
sips -z 64 64     RespeQt.png --out RespeQt.iconset/icon_32x32@2x.png
sips -z 128 128   RespeQt.png --out RespeQt.iconset/icon_128x128.png
sips -z 256 256   RespeQt.png --out RespeQt.iconset/icon_128x128@2x.png
sips -z 256 256   RespeQt.png --out RespeQt.iconset/icon_256x256.png
sips -z 512 512   RespeQt.png --out RespeQt.iconset/icon_256x256@2x.png
sips -z 512 512   RespeQt.png --out RespeQt.iconset/icon_512x512.png
cp RespeQt.png RespeQt.iconset/icon_512x512@2x.png
iconutil -c icns RespeQt.iconset
rm -R RespeQt.iconset
