#!/usr/bin/env bash
python scripts/cpplint.py --extensions="hpp,cpp" --filter="-legal/copyright" $( find . -name "*.cpp" -or -name "*.hpp" | grep -v -e "build/" -e "lib/" )
