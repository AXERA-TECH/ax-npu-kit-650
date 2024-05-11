#!/bin/bash
mkdir -p third-party
cd third-party
if [ ! -d libopencv-4.5.5-aarch64 ]; then
  wget -c https://github.com/ZHEQIUSHUI/assets/releases/download/ax650/libopencv-4.5.5-aarch64.zip
  unzip libopencv-4.5.5-aarch64.zip
fi
cd ..