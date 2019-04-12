#ifndef _CODEIMG_H_
#define _CODEIMG_H_

extern "C"
{
#include "../include/x264.h"
#include "../include/libavcodec/avcodec.h"
#include "../include/libavformat/avformat.h"
#include "../include/libswscale/swscale.h"
#include "../include/libavutil/imgutils.h"
}


typedef struct __X264_ENCODER__
{

	x264_t* m_pX264Handle;
	x264_param_t* m_pX264Param;

	x264_picture_t* m_pX264Pic_out;
	x264_picture_t* m_pX264Pic_in;
	x264_nal_t* m_pX264Nals;
	int m_x264iNal;
	FILE *m_x264Fp;
}X264Encoder;


extern X264Encoder x264Encoder;
extern void encode_yuv(unsigned char *g_pImageData);
extern int encode_jpeg(unsigned char *pucImageData);
extern void encoderImg(X264Encoder &x264Encoder,AVFrame *frame);
extern void initX264Encoder(X264Encoder &x264Encoder, const char *filePath);


#endif
