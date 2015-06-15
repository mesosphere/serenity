mkdir build
#if it's already unpacked then ignore error.
unzip lib/pbjson -d lib/ 2>/dev/null
unzip lib/gmock-1.7.0.zip -d lib/ 2>/dev/null
exit 0
