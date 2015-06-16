mkdir build #2> /dev/null
#if it's already unpacked then ignore error.
unzip -n lib/pbjson.zip -d lib/
unzip -n lib/gmock-1.7.0.zip -d lib/