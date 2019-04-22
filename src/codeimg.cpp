#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/x264.h"
#include "../include/codeimg.h"
#include "../include/jpeglib.h"
#include "../include/common_toupcam.h"

extern "C"
{
#include "../include/libavutil/frame.h"
#include "../include/libswscale/swscale.h"
#include "../include/libavcodec/avcodec.h"
};

#define VENC_FPS 30
#define JPEG_COMPRESS_QUILITY    (100)

void encode_yuv(unsigned char *g_pImageData);
int encode_jpeg(unsigned char *pucImageData);
void encoderImg(X264Encoder &x264Encoder,AVFrame *frame);
void initX264Encoder(X264Encoder &x264Encoder, const char *filePath);
static void convertFrameToX264Img(x264_image_t *x264InImg,AVFrame *pFrameYUV);


void initX264Encoder(X264Encoder &px264Encoder, const char *filePath)
{
	x264Encoder.m_x264Fp = fopen(filePath, "wb");
	x264Encoder.m_pX264Param = (x264_param_t *)malloc(sizeof(x264_param_t));
	//assert(x264Encoder.m_pX264Param);
	x264_param_default(x264Encoder.m_pX264Param);
	x264_param_default_preset(x264Encoder.m_pX264Param, "veryfast", "zerolatency");
	x264_param_apply_profile(x264Encoder.m_pX264Param, "baseline");
	x264Encoder.m_pX264Param->i_threads = X264_THREADS_AUTO;//X264_SYNC_LOOKAHEAD_AUTO; // 取空缓冲区继续使用不死锁的保证

	// 视频选项
	x264Encoder.m_pX264Param->i_width = g_pstTouPcam->inWidth; // 要编码的图像宽度.
	x264Encoder.m_pX264Param->i_height = g_pstTouPcam->inHeight; // 要编码的图像高度

	// 帧率
	x264Encoder.m_pX264Param->b_vfr_input = 0;//0时只使用fps控制帧率
	int m_frameRate = VENC_FPS;
	x264Encoder.m_pX264Param->i_fps_num = m_frameRate; // 帧率分子
	x264Encoder.m_pX264Param->i_fps_den = 1; // 帧率分母
	x264Encoder.m_pX264Param->i_timebase_den = x264Encoder.m_pX264Param->i_fps_num;
	x264Encoder.m_pX264Param->i_timebase_num = x264Encoder.m_pX264Param->i_fps_den;
	x264Encoder.m_pX264Param->b_intra_refresh = 0;
	x264Encoder.m_pX264Param->b_annexb = 1;
	//m_pX264Param->b_repeat_headers = 0;
	x264Encoder.m_pX264Param->i_keyint_max = m_frameRate;

	x264Encoder.m_pX264Param->i_csp = X264_CSP_I420;//X264_CSP_I420;// 
	x264Encoder.m_pX264Param->i_log_level = X264_LOG_INFO;//X264_LOG_DEBUG;

	x264Encoder.m_x264iNal = 0;
	x264Encoder.m_pX264Nals = NULL;
	x264Encoder.m_pX264Pic_in = (x264_picture_t *)malloc(sizeof(x264_picture_t));
	if (x264Encoder.m_pX264Pic_in == NULL)
		exit(1);
	else
		memset(x264Encoder.m_pX264Pic_in, 0, sizeof(x264_picture_t));
	//x264_picture_alloc(m_pX264Pic_in, X264_CSP_I420, m_pX264Param->i_width, m_pX264Param->i_height);
	x264_picture_alloc(x264Encoder.m_pX264Pic_in, X264_CSP_I420, x264Encoder.m_pX264Param->i_width, x264Encoder.m_pX264Param->i_height);
	x264Encoder.m_pX264Pic_in->i_type = X264_TYPE_AUTO;

	x264Encoder.m_pX264Pic_out = (x264_picture_t *)malloc(sizeof(x264_picture_t));
	if (x264Encoder.m_pX264Pic_out == NULL)
		exit(1);
	else
		memset(x264Encoder.m_pX264Pic_out, 0, sizeof(x264_picture_t));
	x264_picture_init(x264Encoder.m_pX264Pic_out);
	x264Encoder.m_pX264Handle = x264_encoder_open(x264Encoder.m_pX264Param);
	//assert(x264Encoder.m_pX264Handle);
}


void encode_yuv(unsigned char *g_pImageData)
{
    int width = nWidth, height = nHeight;
    enum AVPixelFormat src_pix_fmt = AV_PIX_FMT_BGR24, dst_pix_fmt = AV_PIX_FMT_YUV420P;

    AVFrame *pFrameYUV = (AVFrame *)av_frame_alloc();
    uint8_t *out_buffer = new uint8_t[avpicture_get_size(dst_pix_fmt, width, height)];
    
    /* avpicture_fill是给pFrameYUV初始化一些字段，并且给填充data和linesize */
    avpicture_fill((AVPicture *)pFrameYUV, out_buffer, dst_pix_fmt, width, height);

    AVFrame *rgbFrame = (AVFrame *)av_frame_alloc();

    avpicture_fill((AVPicture *)rgbFrame, g_pImageData, src_pix_fmt, width, height);

    SwsContext *sws_ctx = sws_getContext(
        width, height, src_pix_fmt,
        width, height, dst_pix_fmt,
        SWS_BILINEAR, NULL, NULL, NULL);

    sws_scale(sws_ctx, rgbFrame->data, rgbFrame->linesize, 0, height, pFrameYUV->data, pFrameYUV->linesize);
    sws_freeContext(sws_ctx);
	av_frame_free(&rgbFrame);
   
    encoderImg(x264Encoder, pFrameYUV);

	av_frame_free(&pFrameYUV);
    free(out_buffer);
    pFrameYUV = NULL;
    out_buffer = NULL;

    /* printf("encode_yuv\r\n"); */
}


int encode_jpeg(unsigned char *pucImageData)
{
    if(NULL == pucImageData)
    {
        printf("%s: pucImageData is null.\n", __func__);
        return ERROR_FAILED;
    }

    int iRet = ERROR_SUCCESS;

    pthread_mutex_lock(&g_PthreadMutexJpgDest);



    struct jpeg_compress_struct stcinfo;
    struct jpeg_error_mgr stjerr;
    FILE *pfile = NULL;
    JSAMPROW row_pointer[1];
    int row_stride = nWidth;
    int step = 0;
    int w_step = 0;
    
    /* step 1 */
    stcinfo.err = jpeg_std_error(&stjerr);
    jpeg_create_compress(&stcinfo);


    /* step 2 */
    giJpgSize = 0;
    jpeg_mem_dest(&stcinfo, &g_pucJpgDest, (unsigned long *)&giJpgSize);


    /* step 3 */
    stcinfo.image_width = g_pstTouPcam->inMaxWidth;
    stcinfo.image_height = g_pstTouPcam->inMaxHeight;
    stcinfo.input_components = RGB24_DEPTH;
    stcinfo.in_color_space = JCS_RGB;

    /* step 4 */
    jpeg_set_defaults(&stcinfo);
    jpeg_set_quality(&stcinfo, JPEG_COMPRESS_QUILITY, TRUE );
    jpeg_start_compress(&stcinfo, TRUE);
    row_stride = g_pstTouPcam->inMaxWidth;


    while (stcinfo.next_scanline < stcinfo.image_height)
    {
        row_pointer[0] = & pucImageData[stcinfo.next_scanline * row_stride*RGB24_DEPTH];
        (void) jpeg_write_scanlines(&stcinfo, row_pointer, 1);
    }


    /* step 5 */
    jpeg_finish_compress(&stcinfo);
    jpeg_destroy_compress(&stcinfo);


    if(g_pStaticImageDataFlag)
    {
        printf("too many snapshot picture flag.\n");
    }
    g_pStaticImageDataFlag = 1;
    pthread_mutex_unlock(&g_PthreadMutexJpgDest);
 
    return iRet;
}

static unsigned int rtp_transmit(unsigned char *pucJpgDest)
{
    if(NULL == pucJpgDest)
    {
        printf("%s: pucJpgDest is null.\n", __func__);
        return ERROR_FAILED;
    }
    int iRet = ERROR_SUCCESS;
    
    
    
    return iRet;
}


void encoderImg(X264Encoder &x264Encoder,AVFrame *frame)
{
	/* 转换图像格式 */
	convertFrameToX264Img(&x264Encoder.m_pX264Pic_in->img,frame);

	x264Encoder.m_pX264Pic_in->i_pts++;
    /* printf("1\r\n"); */
	int ret = x264_encoder_encode(x264Encoder.m_pX264Handle, &x264Encoder.m_pX264Nals, &x264Encoder.m_x264iNal, x264Encoder.m_pX264Pic_in, x264Encoder.m_pX264Pic_out);
    /* printf("2\r\n"); */
	if (ret< 0){
		printf("Error.\n");
		return;
	}

    struct rtp_data *pdata;
	for (int i = 0; i < x264Encoder.m_x264iNal; ++i)
	{
        pdata=(struct rtp_data *)malloc(sizeof(struct rtp_data));
    	memset(pdata,0,sizeof(struct rtp_data));

        pdata->datalen=x264Encoder.m_pX264Nals[i].i_payload;
        unsigned char *rtpdata = (unsigned char *)malloc(pdata->datalen);
        memcpy(rtpdata, x264Encoder.m_pX264Nals[i].p_payload, pdata->datalen);
        pdata->buff=rtpdata;
    	pdata->bufrelen=0;
		//printf("buff len : %d\r\n", pdata->datalen);

		/* 数据封包 */
    	while((rtp=rtp_pack(pdata,&head))!=NULL)
    	{
        	/* 数据发送 */
        	rtp_send(rtp,sock);
        	free(rtp);
        	rtp=NULL;
        	/* 序列号增加 */
        	head.sernum++;
        	/* usleep(500); */
			/* printf("Send Num : %d\r\n", sendnum++); */
    	}
		free(pdata);
		pdata=NULL;
	}
    /* 时间戳增加 */
    head.timtamp+=800;
    /* usleep(1*1000*1000); */
}

static void convertFrameToX264Img(x264_image_t *x264InImg,AVFrame *pFrameYUV)
{
	//RGB方式
	x264InImg->plane[0] = pFrameYUV->data[0];
	x264InImg->plane[1] = pFrameYUV->data[1];
	x264InImg->plane[2] = pFrameYUV->data[2];
}


void encode2hardware((unsigned char *)g_pImageData)
{
    
    return;
}


