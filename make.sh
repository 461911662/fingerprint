#!/bin/bash
cd ./src
##g++ -Wl,-rpath -Wl,'$ORIGIN' -g -o ../demosimplest codeimg.cpp common_toupcam.cpp Error.cpp main.cpp rtp.cpp socket.cpp toupcamcfg.cpp wificfg.cpp toupcam_log.cpp -I/home/cht/work/project/fingerprint_xiao/fingerprint/include/ -L/home/cht/work/project/fingerprint_xiao/lib -ltoupcam -lavformat -lavcodec -lswscale -lswresample -lavutil -lm -lz -llzma -pthread -ljpeg -lx264
aarch64-linux-gnu-g++ -Wl,-rpath -Wl,'$ORIGIN' -g -o ../demosimplest codeimg.cpp common_toupcam.cpp Error.cpp main.cpp rtp.cpp socket.cpp toupcamcfg.cpp wificfg.cpp toupcam_log.cpp -I/home/cht/test/lib/lib/include/ -L/home/cht/test/lib/lib/lib -ltoupcam -lavformat -lavcodec -lswscale -lswresample -lavutil -lm -lz -pthread -ljpeg -lx264 -lrockchip_mpp #-llzma
cd -
