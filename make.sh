#!/bin/bash
cd ./src
#g++ -Wl,-rpath -Wl,'$ORIGIN' -L. -g -o demosimplest main.cpp common_toupcam.cpp rtp.cpp Error.cpp socket.c toupcamcfg.c wificfg.c -I. -I/home/cht/ffmpegUBT/include/ -L/home/cht/ffmpegUBT/lib `pkg-config opencv --libs` `pkg-config opencv --cflags` -L-ltoupcam -lavformat -lavcodec -lswscale -lswresample -lavutil -lm -lz -llzma -pthread -lx264 -ljpeg
g++ -Wl,-rpath -Wl,'$ORIGIN' -L. -g -o demosimplest main.cpp common_toupcam.cpp rtp.cpp Error.cpp socket.c toupcamcfg.c wificfg.c -I. -I/home/cht/work/project/fingerprint_xiao/include/ -L/home/cht/work/project/fingerprint_xiao/lib `pkg-config opencv --libs` `pkg-config opencv --cflags` -L-ltoupcam -lavformat -lavcodec -lswscale -lswresample -lavutil -lm -lz -llzma -pthread -lx264 -ljpeg
cd -
