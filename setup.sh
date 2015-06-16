mkdir build #2> /dev/null
#if it's already unpacked then ignore error.
unzip -u -n lib/pbjson.zip -d lib/
unzip -u -n -f lib/gmock-1.7.0.zip -d lib/