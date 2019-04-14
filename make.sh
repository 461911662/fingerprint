#!/bin/bash
cd ./src
#g++ -Wl,-rpath -Wl,'$ORIGIN' -L. -g -o demosimplest main.cpp common_toupcam.cpp rtp.cpp Error.cpp socket.c toupcamcfg.c wificfg.c codeimg.c -I. -I/home/cht/ffmpegUBT/include/ -L/home/cht/ffmpegUBT/lib `pkg-config opencv --libs` `pkg-config opencv --cflags` -L-ltoupcam -lavformat -lavcodec -lswscale -lswresample -lavutil -lm -lz -llzma -pthread -lx264 -ljpeg
g++ -Wl,-rpath -Wl,'$ORIGIN' -g -o ../demosimplest codeimg.cpp common_toupcam.cpp Error.cpp main.cpp rtp.cpp socket.cpp toupcamcfg.cpp wificfg.cpp -I/home/cht/work/project/fingerprint_xiao/fingerprint/include/ -L/home/cht/work/project/fingerprint_xiao/fingerprint/lib/ -ltoupcam -lavformat -lavcodec -lswscale -lswresample -lavutil -lm -lz -llzma -pthread -ljpeg -lx264
cd -
