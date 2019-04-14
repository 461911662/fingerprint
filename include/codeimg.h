#ifndef _CODEIMG_H_
#define _CODEIMG_H_

#include "../include/x264.h"
#include "../include/rtp.h"

//#include "../include/libavutil/frame.h"

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
#define RGB24_DEPTH  (3)


extern X264Encoder x264Encoder;
extern int nWidth, nHeight, giJpgSize;
extern unsigned char *gpucJpgDest;//[1024*1022];
extern struct rtp_pack *rtp;
extern struct rtp_pack_head head;
extern int sendnum;

extern void encode_yuv(unsigned char *g_pImageData);
extern int encode_jpeg(unsigned char *pucImageData);
extern void initX264Encoder(X264Encoder &px264Encoder, const char *filePath);


#endif
