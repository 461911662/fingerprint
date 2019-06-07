#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/x264.h"
#include "../include/socket.h"
#include "../include/codeimg.h"
#include "../include/jpeglib.h"
#include "../include/mpp_encode_data.h"
#include "../include/common_toupcam.h"

extern "C"
{
#include "../include/libavutil/frame.h"
#include "../include/libswscale/swscale.h"
#include "../include/libavcodec/avcodec.h"
};

extern MPP_ENC_DATA_S *g_pstmpp_enc_data;

#define VENC_FPS 30 /* 控制帧率 */
#define M_BITRATE (2*1024*1024)  /* 控制码流 */
#define JPEG_COMPRESS_QUILITY    (100)

void encode_yuv(unsigned char *g_pImageData);
int encode_jpeg(unsigned char *pucImageData);
void encoderImg(X264Encoder &x264Encoder,AVFrame *frame);
void initX264Encoder(X264Encoder &x264Encoder, const char *filePath);
static void convertFrameToX264Img(x264_image_t *x264InImg,AVFrame *pFrameYUV);


void initX264Encoder(X264Encoder &px264Encoder, const char *filePath)
{
#ifndef INITX264ENCODER_DEFVAL
    x264Encoder.m_x264Fp = fopen(filePath, "wb");

    x264Encoder.m_pX264Param = (x264_param_t *)malloc(sizeof(x264_param_t));
    if(NULL == x264Encoder.m_pX264Param)
    {
        toupcam_log_f(LOG_ERROR, "%s", strerror(errno));
        exit(1);
    }
	x264_param_default(x264Encoder.m_pX264Param);

    /* 0.编码延时 */
    x264_param_default_preset(x264Encoder.m_pX264Param, "veryfast" , "zerolatency" );

    /* 1.初始化profile */
    x264_param_apply_profile(x264Encoder.m_pX264Param, "baseline");

    /* 2.threads设置 */
    x264Encoder.m_pX264Param->i_threads = X264_THREADS_AUTO;/* X264_SYNC_LOOKAHEAD_AUTO;  取空缓冲区继续使用不死锁的保证 */

    /* 3.视频选项大小 */
    x264Encoder.m_pX264Param->i_width = g_pstTouPcam->inWidth; /* 要编码的图像宽度. */
    x264Encoder.m_pX264Param->i_height = g_pstTouPcam->inHeight; /* 要编码的图像高度 */

    /* 4.设置编码复杂度 */
    x264Encoder.m_pX264Param->i_level_idc = 30;

    /* 5.图像质量控制 */
    x264Encoder.m_pX264Param->rc.f_rf_constant = 25;
    x264Encoder.m_pX264Param->rc.f_rf_constant_max = 45;

    /* 码率控制 */
    /*
    * CQP 恒定质量
    * CRF 恒定码率
    * ABR 平均码率
    */
    x264Encoder.m_pX264Param->rc.i_rc_method = X264_RC_ABR;
    x264Encoder.m_pX264Param->rc.i_vbv_max_bitrate = (int)((M_BITRATE*1.2)/1000); /* 平均码率模式，最大瞬时码率，默认0(与-B设置相同) */
    x264Encoder.m_pX264Param->rc.i_bitrate = (int)M_BITRATE/1000;

    /* 6.rtp时,使用i帧添加sps,pps */
    x264Encoder.m_pX264Param->b_repeat_headers = 1; /* 重复SPS/PPS放到关键帧前面 */

    /* 7.i帧间隔 */
    x264Encoder.m_pX264Param->b_vfr_input = 0; /* 0时只使用fps控制帧率 */
    int m_frameRate = VENC_FPS;
    x264Encoder.m_pX264Param->i_fps_num = m_frameRate; /* 帧率分子 */
    x264Encoder.m_pX264Param->i_fps_den = 1; /* 帧率分母 */
	x264Encoder.m_pX264Param->i_timebase_den = x264Encoder.m_pX264Param->i_fps_num;
	x264Encoder.m_pX264Param->i_timebase_num = x264Encoder.m_pX264Param->i_fps_den;    
    x264Encoder.m_pX264Param->i_keyint_max = m_frameRate/2; /* 1秒刷新一个i帧 */

    x264Encoder.m_pX264Param->b_intra_refresh = 0; /* i->p */
    x264Encoder.m_pX264Param->b_annexb = 1;
    x264Encoder.m_pX264Param->b_cabac = 1;

    /* 9.设置编码格式 */
    x264Encoder.m_pX264Param->i_csp = X264_CSP_I420; /* X264_CSP_I420 */
    x264Encoder.m_pX264Param->i_log_level = X264_LOG_INFO; /* X264_LOG_DEBUG */
#else
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
#endif

    x264Encoder.m_x264iNal = 0;
    x264Encoder.m_pX264Nals = NULL;
    x264Encoder.m_pX264Pic_in = (x264_picture_t *)malloc(sizeof(x264_picture_t));
    if (x264Encoder.m_pX264Pic_in == NULL)
    {
        toupcam_log_f(LOG_ERROR, "%s", strerror(errno));
        exit(1);
    }
    else
    {
        memset(x264Encoder.m_pX264Pic_in, 0, sizeof(x264_picture_t));
    }
    x264_picture_alloc(x264Encoder.m_pX264Pic_in, X264_CSP_I420, x264Encoder.m_pX264Param->i_width, x264Encoder.m_pX264Param->i_height);
    x264Encoder.m_pX264Pic_in->i_type = X264_TYPE_AUTO;

    x264Encoder.m_pX264Pic_out = (x264_picture_t *)malloc(sizeof(x264_picture_t));
    if (x264Encoder.m_pX264Pic_out == NULL)
    {
        toupcam_log_f(LOG_ERROR, "%s", strerror(errno));
        exit(1);
    }
    else
    {
        memset(x264Encoder.m_pX264Pic_out, 0, sizeof(x264_picture_t));
    }
    x264_picture_init(x264Encoder.m_pX264Pic_out);

    x264Encoder.m_pX264Handle = x264_encoder_open(x264Encoder.m_pX264Param);
    if(NULL == x264Encoder.m_pX264Handle)
    {
        toupcam_log_f(LOG_ERROR, "x264encoder_open Error.");
        exit(1);
    }
}

MPP_RET mpp_ctx_init(MPP_ENC_DATA_S **data)
{
    MPP_ENC_DATA_S *pstmpp = NULL;
    MPP_RET ret = MPP_OK;

    if (NULL == data) 
    {
        printf("invalid input data %p cmd %p\n", data);
        return MPP_ERR_NULL_PTR;
    }

    pstmpp = (MPP_ENC_DATA_S *)calloc(sizeof(MPP_ENC_DATA_S), 1);
    if (NULL == pstmpp) 
    {
        printf("create MpiEncTestData failed\n");
        ret = MPP_ERR_MALLOC;
        goto RET;
    }

    pstmpp->width = g_pstTouPcam->inWidth;
    pstmpp->height = g_pstTouPcam->inHeight;
    pstmpp->hor_stride = MPP_ALIGN(pstmpp->width, 16);
    pstmpp->ver_stride = MPP_ALIGN(pstmpp->height, 16);
    pstmpp->fmt = MPP_FMT_YUV420P;
    pstmpp->type = MPP_VIDEO_CodingAVC;
    pstmpp->frame_size = pstmpp->hor_stride * pstmpp->ver_stride * 3 / 2;    
    pstmpp->fp_output = fopen("./camera.h264", "w+b");
RET:
    *data = pstmpp;
    return ret;
}

MPP_RET mpp_setup(MPP_ENC_DATA_S *p)
{
    MPP_RET ret;
    MppApi *mpi;
    MppCtx ctx;
    MppEncCodecCfg *codec_cfg;
    MppEncPrepCfg *prep_cfg;
    MppEncRcCfg *rc_cfg;

    if (NULL == p)
    {
        return MPP_ERR_NULL_PTR;
    }

    mpi = p->mpi;
    ctx = p->ctx;
    codec_cfg = &p->codec_cfg;
    prep_cfg = &p->prep_cfg;
    rc_cfg = &p->rc_cfg;

    /* setup default parameter */
    p->fps = 15;
    p->gop = 10;
    p->bps = p->width * p->height / 8 * p->fps;

    prep_cfg->change        = MPP_ENC_PREP_CFG_CHANGE_INPUT |
                              MPP_ENC_PREP_CFG_CHANGE_ROTATION |
                              MPP_ENC_PREP_CFG_CHANGE_FORMAT;
    prep_cfg->width         = p->width;
    prep_cfg->height        = p->height;
    prep_cfg->hor_stride    = p->hor_stride;
    prep_cfg->ver_stride    = p->ver_stride;
    prep_cfg->format        = p->fmt;
    prep_cfg->rotation      = MPP_ENC_ROT_0;
    ret = mpi->control(ctx, MPP_ENC_SET_PREP_CFG, prep_cfg);
    if (ret) 
    {
        printf("mpi control enc set prep cfg failed ret %d\n", ret);
        goto RET;
    }

    rc_cfg->change  = MPP_ENC_RC_CFG_CHANGE_ALL;
    //rc_cfg->rc_mode = MPP_ENC_RC_MODE_CBR;
    //rc_cfg->quality = MPP_ENC_RC_QUALITY_MEDIUM;
    rc_cfg->rc_mode = MPP_ENC_RC_MODE_VBR;
    rc_cfg->quality = MPP_ENC_RC_QUALITY_CQP;

    if (rc_cfg->rc_mode == MPP_ENC_RC_MODE_CBR) 
    {
        /* constant bitrate has very small bps range of 1/16 bps */
        rc_cfg->bps_target   = p->bps;
        rc_cfg->bps_max      = p->bps * 17 / 16;
        rc_cfg->bps_min      = p->bps * 15 / 16;
    } 
    else if (rc_cfg->rc_mode ==  MPP_ENC_RC_MODE_VBR) 
    {
        if (rc_cfg->quality == MPP_ENC_RC_QUALITY_CQP) 
        {
            /* constant QP does not have bps */
            //rc_cfg->bps_target   = -1;
            //rc_cfg->bps_max      = -1;
            //rc_cfg->bps_min      = -1;
            rc_cfg->bps_target   = p->bps;
            rc_cfg->bps_max      = p->bps * 17 / 16;
            rc_cfg->bps_min      = p->bps * 1 / 16;
        } 
        else 
        {
            /* variable bitrate has large bps range */
            rc_cfg->bps_target   = p->bps;
            rc_cfg->bps_max      = p->bps * 17 / 16;
            rc_cfg->bps_min      = p->bps * 1 / 16;
        }
    }

    /* fix input / output frame rate */
    rc_cfg->fps_in_flex      = 0;
    rc_cfg->fps_in_num       = p->fps;
    rc_cfg->fps_in_denorm    = 1;
    rc_cfg->fps_out_flex     = 0;
    rc_cfg->fps_out_num      = p->fps;
    rc_cfg->fps_out_denorm   = 1;

    rc_cfg->gop              = p->gop;
    rc_cfg->skip_cnt         = 0;

    printf("mpi_enc_test bps %d fps %d gop %d\n",
            rc_cfg->bps_target, rc_cfg->fps_out_num, rc_cfg->gop);
    ret = mpi->control(ctx, MPP_ENC_SET_RC_CFG, rc_cfg);
    if (ret) 
    {
        printf("mpi control enc set rc cfg failed ret %d\n", ret);
        goto RET;
    }

    codec_cfg->coding = p->type;
    switch (codec_cfg->coding) 
    {
        case MPP_VIDEO_CodingAVC : 
        {
            codec_cfg->h264.change = MPP_ENC_H264_CFG_CHANGE_PROFILE |
                                     MPP_ENC_H264_CFG_CHANGE_ENTROPY |
                                     MPP_ENC_H264_CFG_CHANGE_TRANS_8x8;
            /*
             * H.264 profile_idc parameter
             * 66  - Baseline profile
             * 77  - Main profile
             * 100 - High profile
             */
            codec_cfg->h264.profile  = 66;
            /*
             * H.264 level_idc parameter
             * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
             * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
             * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
             * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
             * 50 / 51 / 52         - 4K@30fps
             */
            codec_cfg->h264.level    = 30;
            codec_cfg->h264.entropy_coding_mode  = 1;
            codec_cfg->h264.cabac_init_idc  = 0;
            codec_cfg->h264.transform8x8_mode = 1;
        } 
        break;
        case MPP_VIDEO_CodingMJPEG : 
            {
                codec_cfg->jpeg.change  = MPP_ENC_JPEG_CFG_CHANGE_QP;
                codec_cfg->jpeg.quant   = 10;
            } 
            break;
        case MPP_VIDEO_CodingVP8 : 
            {
            } 
            break;
        case MPP_VIDEO_CodingHEVC : 
            {
                codec_cfg->h265.change = MPP_ENC_H265_CFG_INTRA_QP_CHANGE;
                codec_cfg->h265.intra_qp = 26;
            } 
            break;
        default : 
            {
                printf("support encoder coding type %d\n", codec_cfg->coding);
            } 
            break;
    }
    ret = mpi->control(ctx, MPP_ENC_SET_CODEC_CFG, codec_cfg);
    if (ret) 
    {
        printf("mpi control enc set codec cfg failed ret %d\n", ret);
        goto RET;
    }

    /* optional */
    p->sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
    ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &p->sei_mode);
    if (ret) 
    {
        printf("mpi control enc set sei cfg failed ret %d\n", ret);
        goto RET;
    }

RET:
    return ret;
}

MPP_RET mpp_ctx_deinit(MPP_ENC_DATA_S **data)
{
    MPP_RET ret = MPP_OK;
    MPP_ENC_DATA_S *p = NULL;

    if (!data) 
    {
        printf("invalid input data %p\n", data);
        return MPP_ERR_NULL_PTR;
    }
    
    p = *data;

    if(NULL != p)
    {
        return ret;
    }
    
    ret = p->mpi->reset(p->ctx);
    if (ret) 
    {
        printf("mpi->reset failed\n");
    }

    if (p->ctx) 
    {
        mpp_destroy(p->ctx);
        p->ctx = NULL;
    }

    if (p->frm_buf) 
    {
        mpp_buffer_put(p->frm_buf);
        p->frm_buf = NULL;
    }

    free(p);
    p = NULL;
    *data = NULL;
    return MPP_OK;
}



void init_mpp()
{
    MPP_RET ret = MPP_OK;
    
    /* ctx init */
    ret = mpp_ctx_init(&g_pstmpp_enc_data);
    if(ret)
    {
        printf("test data init failed ret %d\n", ret);
        goto INIT_MPP_OUT;
    }
    
    ret = mpp_buffer_get(NULL, &g_pstmpp_enc_data->frm_buf, g_pstmpp_enc_data->frame_size);
    if (ret) 
    {
        printf("failed to get buffer for input frame ret %d\n", ret);
        goto INIT_MPP_OUT;
    }

    printf("mpi_enc_test encoder test start w %d h %d type %d\n", 
        g_pstmpp_enc_data->width, g_pstmpp_enc_data->height, g_pstmpp_enc_data->type);

    ret = mpp_create(&g_pstmpp_enc_data->ctx, &g_pstmpp_enc_data->mpi);
    if (ret) 
    {
        printf("mpp_create failed ret %d\n", ret);
        goto INIT_MPP_OUT;
    }

    ret = mpp_init(g_pstmpp_enc_data->ctx, MPP_CTX_ENC, g_pstmpp_enc_data->type);
    if (ret) 
    {
        printf("mpp_init failed ret %d\n", ret);
        goto INIT_MPP_OUT;
    }

    ret = mpp_setup(g_pstmpp_enc_data);
    if (ret) 
    {
        printf("mpp setup failed ret %d\n", ret);
        goto INIT_MPP_OUT;
    }
    return;
    
INIT_MPP_OUT:

    ret = g_pstmpp_enc_data->mpi->reset(g_pstmpp_enc_data->ctx);
    if (ret) 
    {
        printf("mpi->reset failed\n");
    }

    if (g_pstmpp_enc_data->ctx) 
    {
        mpp_destroy(g_pstmpp_enc_data->ctx);
        g_pstmpp_enc_data->ctx = NULL;
    }

    if (g_pstmpp_enc_data->frm_buf) 
    {
        mpp_buffer_put(g_pstmpp_enc_data->frm_buf);
        g_pstmpp_enc_data->frm_buf = NULL;
    }
}

static unsigned char GetGray(unsigned char *pImageData, int inheight, int jnwidth, int nheight, int nwidth)
{
    if(NULL == pImageData || nheight < 0 || nwidth < 0 || inheight < 0 || jnwidth < 0)
    {
        toupcam_log_f(LOG_ERROR, "input ptr is null");
        return 0;
    }

    unsigned char gray = 0;
    unsigned char *pucImageData = pImageData+nwidth*3*nheight+jnwidth;
    /* 方法1: Gray = (R*30 + G*59 + B*11 + 50) / 100 */
    /* 方法2: Gray = (R*38 + G*75 + B*15) >> 7 */
    gray = (*pucImageData*38 + *(pucImageData+1)*75 + *(pucImageData+2)*15)>>7;
    /*
    gray = (*pucImageData + *(pucImageData+1) + *(pucImageData+2))/3;
    */
    return gray;
}

static unsigned char linetran(unsigned char *pImageData, int nheight, int nwidth, double fa, double fb)
{
    if(NULL == pImageData || nheight < 0 || nwidth < 0)
    {
        toupcam_log_f(LOG_ERROR, "input ptr is null");
        return 0;
    }

    int target = 0;
    unsigned char gray = 0;
    unsigned char *pucImageData = NULL;

    for (int i = 0; i < nheight; ++i)
    {
        for (int j = 0; j < nwidth*3; j+=3)
        {
            gray = GetGray(pImageData, i, j, nheight, nwidth);
            target = fa*gray + fb;
            if(target < 0)
            {
                target = 0;
            }
            else if(target > 255)
            {
                target = 255;
            }
            pucImageData = pImageData + i*nwidth*3 + j;
            *pucImageData++ = target;
            *pucImageData++ = target;
            *pucImageData = target;
        }
    }    
    return 0;
}

/*
* 图像二值化处理
*/
int Binarization(unsigned char *pImageData, int height, int width, int bit)
{
    int iRet = ERROR_SUCCESS;
    int i, j;
    if(NULL == pImageData || height < 0 || width < 0)
    {
        toupcam_log_f(LOG_ERROR, "input ptr is null");
        return -1;
    }

    if(bit != 24)
    {
        toupcam_log_f(LOG_ERROR, "bit is not 24");
        return -1;
    }

#ifdef STUDY_RGB_GRAY
    /* study RGB R=G=B=gray */
    for (i = 0; i < height; ++i)
    {
        for (j = 0; j < width; ++j)
        {
            if(*((unsigned char*)pImageData+i*width*3+j) == *((unsigned char*)pImageData+i*width*3+j+1) 
                && *((unsigned char*)pImageData+i*width*3+j) == *((unsigned char*)pImageData+i*width*3+j+2))
            {
                toupcam_log_f(LOG_ERROR, "R G B is equal");
            }
            else
            {
                toupcam_log_f(LOG_ERROR, "R G B is not equal");
            }
        }
    }
#endif

#ifdef STUDY_RGB_THRESHOLD_BINARIZATION
    /* 阈值二值化 */
    for (i = 0; i < height; ++i)
    {
        for (j = 0; j < width*3; ++j)
        {
            if(127 > *((unsigned char *)pImageData+i*width*3+j))
            {
                *((unsigned char *)pImageData+i*width*3+j) = 0;
            }
            else
            {
                *((unsigned char *)pImageData+i*width*3+j) = 255;
            }
        }
    }
#endif

#ifdef STUDY_RGB_AVERAGE_BINARIZATION
    /* 平均值二值化 */
    unsigned long long ullaverage = 0;
    for (i = 0; i < height; ++i)
    {
        for (j = 0; j < width*3; j+=3)
        {
            ullaverage += *((unsigned char *)pImageData+i*width*3+j);
        }
    }
    ullaverage = (unsigned char)(ullaverage/(height*width));
    for (i = 0; i < height; ++i)
    {
        for (j = 0; j < width*3; ++j)
        {
            if(ullaverage > *((unsigned char *)pImageData+i*width*3+j))
            {
                *((unsigned char *)pImageData+i*width*3+j) = 0;
            }
            else
            {
                *((unsigned char *)pImageData+i*width*3+j) = 255;
            }
        }
    }    
#endif
    

    return 0;
}

int image_arithmetic_handle_rgb(unsigned char *pImageData, int height, int width, int bit)
{
    int iRet = ERROR_SUCCESS;
    int i, j;
    if(NULL == pImageData || height < 0 || width < 0)
    {
        toupcam_log_f(LOG_ERROR, "input ptr is null");
        return -1;
    }

    if(bit != 24)
    {
        toupcam_log_f(LOG_ERROR, "bit is not 24");
        return -1;
    }

#ifdef STUDY_HISTOGRAM_
    unsigned char n[255] = {0};
    unsigned char c[255] = {0};
    unsigned char p[255] = {0};
    unsigned char min,max;

    i = height;
    j = width * 3;
    min = max = pImageData[0];
    /* 累计灰度值 */
    for(i = 0; i < height; i++)
    {
        for(j = 0; j < width*3; j++)
        {
            if(min > pImageData[i+j])
            {
                min = pImageData[i+j];
            }
            if(max < pImageData[i+j])
            {
                max = pImageData[i+j];
            }
            n[pImageData[i+j]]++;
        }
    }
    for(i = 0; i < 255; i++)
    {
        p[i] = n[i] / (width*height*3);
    }
    
    /* 累计值归一化 */
    for(i = 0; i < 255; i++)
    {
        for(j = 0; j <= i; j++)
        {
            c[i] += p[j];
        }
    }

    /* 均衡化处理 */
    for(i = 0; i < height; i++)
    {
        for(j = 0; j < width*3; j++)
        {
            pImageData[i+j] = c[pImageData[i+j]] * (max - min) + min;
        }
    }
#endif

    iRet = Binarization(pImageData, height, width, bit);
    if(ERROR_SUCCESS != iRet)
    {
        toupcam_log_f(LOG_ERROR, "image Binarization failed");
        return ERROR_FAILED;
    }

    return ERROR_SUCCESS;
    
    unsigned char gray = 0;
    unsigned char *pucImageData = NULL;
    /* 灰度值归一化 */
    /* 线性灰度变换 */
    linetran(pImageData, height, width, -1, 255);
    /*
    for (i = 0; i < height; ++i)
    {
        for (j = 0; j < width; ++j)
        {
            gray = GetGray(pImageData, i, j);
            pucImageData = pImageData + i*j*3+ j*3;
            *pucImageData++ = gray;
            *pucImageData++ = gray;
            *pucImageData = gray;
        }
    }
    */

    return 0;
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

static void repeatencode2h264(char *ptr, void *rsppsp, size_t *len, size_t len0)
{
    if(NULL == ptr)
    {
        printf("%s: ptr is null.\n", __func__);
        return ;
    }
    char *pcfinish = NULL;
    char flag = 0;
    char *pctr = ptr;
    char *pctr2 = NULL;

    /* init x264Encode array */
    x264_nal_t *px264Nal = NULL;
    x264_nal_t *x264Nal = NULL;
    size_t ilen = *len;
    
    memset(&x264Encoder, 0, sizeof(x264Encoder));
    /* 分解h264裸数据 */
    while(ilen>4 && ilen--)
    {
        if(*((int*)pctr) == 0x01000000)  /* start code */
        {
            pctr2 = pctr;
            x264Encoder.m_x264iNal++;
            toupcam_dbg_f(LOG_DEBUG, "x264iNal:%d", x264Encoder.m_x264iNal);
            x264Nal = (x264_nal_t *)realloc(x264Nal, sizeof(x264_nal_t)*x264Encoder.m_x264iNal);
            px264Nal = x264Nal;
            px264Nal += x264Encoder.m_x264iNal - 1;
            px264Nal->i_type = *(pctr+4) & 0x1f;
            px264Nal->i_payload = ilen;
            px264Nal->p_payload = (uint8_t*)pctr;

#if 0
            px264Nal = (x264_nal_t *)malloc(sizeof(x264_nal_t));
            px264Nal->i_type = *(pctr+4) & 0x1f;
            px264Nal->i_payload = ilen;
            px264Nal->p_payload = (uint8_t*)pctr;
#endif
        }
        pctr++;
    }

    x264Encoder.m_pX264Nals = x264Nal;

    uint8_t *p_payload = NULL;

    for (int i = 0; i < x264Encoder.m_x264iNal; ++i)
    {
        /* if(0x05 == x264Encoder.m_pX264Nals[i].i_type) */ 
        if(x264Encoder.m_pX264Nals[i].i_type) /* I帧 添加h264头 */
        {
            p_payload = (uint8_t *)malloc(len0+x264Encoder.m_pX264Nals[i].i_payload);
            memcpy(p_payload, rsppsp, len0);
            memcpy(p_payload+len0, x264Encoder.m_pX264Nals[i].p_payload, x264Encoder.m_pX264Nals[i].i_payload);
            x264Encoder.m_pX264Nals[i].p_payload = p_payload;
            *len += len0;
            px264Nal->i_payload += len0;
        }

        if(0x05 == x264Encoder.m_pX264Nals[i].i_type)
        {
            toupcam_dbg_f(LOG_DEBUG, "I payload:%d\n", px264Nal->i_payload);            
        }
        /*
        else if(0x05 == x264Encoder.m_pX264Nals[i].i_type)
        {
            toupcam_log_f(LOG_ERROR, "P payload:%d\n", px264Nal->i_payload);
        }
        else if(0x05 == x264Encoder.m_pX264Nals[i].i_type)
        {
            toupcam_log_f(LOG_ERROR, "B payload:%d\n", px264Nal->i_payload);
        }
        */
        else
        {
            toupcam_dbg_f(LOG_DEBUG, "other payload:%d\n", px264Nal->i_payload);
        }
    }

    return ;
}

static unsigned int rtp_transmit(void *ptr, void *rsppsp, size_t len, size_t len0)
{
    if(NULL == ptr)
    {
        printf("%s: ptr is null.\n", __func__);
        return ERROR_FAILED;
    }
    int iRet = ERROR_SUCCESS;
    size_t ilen = len;

    /* encode h264 again */
    repeatencode2h264((char *)ptr, rsppsp, &ilen, len0);

    struct rtp_data *pdata;
	pthread_mutex_lock(&g_PthreadMutexUDP);
	frame_size = 0;
	for (int i = 0; i < x264Encoder.m_x264iNal; ++i)
	{
        pdata=(struct rtp_data *)malloc(sizeof(struct rtp_data));
    	memset(pdata,0,sizeof(struct rtp_data));

        pdata->datalen=x264Encoder.m_pX264Nals[i].i_payload;
        unsigned char *rtpdata = (unsigned char *)malloc(pdata->datalen);
        memcpy(rtpdata, x264Encoder.m_pX264Nals[i].p_payload, pdata->datalen);
        frame_size += pdata->datalen;
        pdata->buff=rtpdata;
    	pdata->bufrelen=0;
		/* printf("buff len : %d\r\n", pdata->datalen); */

		/* 数据封包 */
    	while((rtp=rtp_pack(pdata,&head))!=NULL)
    	{
        	/* 数据发送 */
        	rtp_send(rtp,sock);
            if(rtp)
            {
        	    free(rtp);
                rtp=NULL;
            }
        	/* 序列号增加 */
        	head.sernum++;
			/* printf("Send Num : %d\r\n", sendnum++); */
    	}
        
        if(pdata->buff)
        {
            free(pdata->buff);
            pdata->buff = NULL;
        }
        if(pdata)
        {
		    free(pdata);
            pdata = NULL;
        }
        if(x264Encoder.m_pX264Nals[i].p_payload)
        {
            free(x264Encoder.m_pX264Nals[i].p_payload);
            x264Encoder.m_pX264Nals[i].p_payload = NULL;
        }
	}

    if(x264Encoder.m_pX264Nals)
    {
        free(x264Encoder.m_pX264Nals);
        x264Encoder.m_pX264Nals = NULL;
    }
    usleep(800);

    /* 时间戳增加 */
    head.timtamp+=800;
    /* usleep(1*1000*1000); */
    pthread_mutex_unlock(&g_PthreadMutexUDP);

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

	pthread_mutex_lock(&g_PthreadMutexUDP);
	frame_size = 0;
    struct rtp_data *pdata;
	for (int i = 0; i < x264Encoder.m_x264iNal; ++i)
	{
        pdata=(struct rtp_data *)malloc(sizeof(struct rtp_data));
    	memset(pdata,0,sizeof(struct rtp_data));

        pdata->datalen=x264Encoder.m_pX264Nals[i].i_payload;
		frame_size += pdata->datalen;
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
    pthread_mutex_unlock(&g_PthreadMutexUDP);
}

static void convertFrameToX264Img(x264_image_t *x264InImg,AVFrame *pFrameYUV)
{
	//RGB方式
	x264InImg->plane[0] = pFrameYUV->data[0];
	x264InImg->plane[1] = pFrameYUV->data[1];
	x264InImg->plane[2] = pFrameYUV->data[2];
}


bool  grb24toyuv420p(unsigned char *RgbBuf, unsigned int nWidth, unsigned int nHeight, unsigned char *yuvBuf,unsigned long *len)  
{  
    int i, j;  
    unsigned char*bufY, *bufU, *bufV, *bufRGB,*bufYuv;  
    memset(yuvBuf,0,(unsigned int )*len);  
    bufY = yuvBuf;  
    bufV = yuvBuf + nWidth * nHeight;  
    bufU = bufV + (nWidth * nHeight* 1/4);  
    *len = 0;   
    unsigned char y, u, v, r, g, b,testu,testv;  
    unsigned int ylen = nWidth * nHeight;  
    unsigned int ulen = (nWidth * nHeight)/4;  
    unsigned int vlen = (nWidth * nHeight)/4; 

    for (j = 0; j<nHeight;j++)  
    {  
        /* bufRGB = RgbBuf + nWidth * (nHeight - 1 - j) * 3 ; */
        if(0 == j)
        {
            bufRGB = RgbBuf + nWidth * j * 3; /* 解决倒立 */
        }
        else
        {
            bufRGB = RgbBuf + nWidth * j * 3 -1; /* 解决倒立 */
        }
        for (i = 0;i<nWidth;i++)  
        {  
            /*
            * 左右反转
            * if(0 == i)
            * {
            *     bufRGB += nWidth * 3 - 1;
            * }
            */
            r = *(bufRGB++);  
            g = *(bufRGB++);  
            b = *(bufRGB++);
            /*
            * 左右反转
            * b = *(bufRGB--);  
            * g = *(bufRGB--);  
            * r = *(bufRGB--);
            */
            y = (unsigned char)( ( 66 * r + 129 * g +  25 * b + 128) >> 8) + 16  ;            
            u = (unsigned char)( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128 ;            
            v = (unsigned char)( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128 ;  
            /* *(bufY++) = max( 0, min(y, 255 )); */
            *(bufY++) = (y<255?y:255)<0?0:(y<255?y:255);
            if (j%2==0&&i%2 ==0)  
            {  
                if (u>255)  
                {  
                    u=255;  
                }  
                if (u<0)  
                {  
                    u = 0;  
                }  
                *(bufU++) =u;  
                /* 存u分量 */
            }  
            else  
            {  
                /* 存v分量 */
                if (i%2==0)  
                {  
                    if (v>255)  
                    {  
                        v = 255;  
                    }  
                    if (v<0)  
                    {  
                        v = 0;  
                    }  
                    *(bufV++) =v;  
                }  
            }  
        }  
    }  
    *len = nWidth * nHeight+(nWidth * nHeight)/2;  
    return true;  
}   

MPP_RET mpp_encode(MPP_ENC_DATA_S *p, unsigned char *yuv, unsigned long len)
{
    MPP_RET ret;
    MppApi *mpi;
    MppCtx ctx;
    MppFrame frame = NULL;
    MppPacket packet = NULL;
    void *buf = NULL;

    if (NULL == p)
    {
        return MPP_ERR_NULL_PTR;
    }

    static int flag = 0;
    static size_t len0 = 0;
    static char *pch264spspps = NULL;

    mpi = p->mpi;
    ctx = p->ctx;

    if (p->type == MPP_VIDEO_CodingAVC && 0 == flag) 
    {
        MppPacket packet = NULL;
        ret = mpi->control(ctx, MPP_ENC_GET_EXTRA_INFO, &packet);
        if (ret) 
        {
            printf("mpi control enc get extra info failed\n");
            goto RET;
        }

        /* get and write sps/pps for H.264 */
        if (packet) 
        {
            void *ptr   = mpp_packet_get_pos(packet);
            len0  = mpp_packet_get_length(packet);
            pch264spspps = (char *)malloc(len0);
            memcpy(pch264spspps, ptr, len0);
            if (p->fp_output)
            {
                //fwrite(ptr, 1, len0, p->fp_output);
                //fclose(p->fp_output);
                //printf("write finished.\n");
            }

            packet = NULL;
        }
        flag = 1;
    }

    if(NULL == pch264spspps)
    {
        printf("crash pch264spspps!!!\n");
        return MPP_ERR_MALLOC;
    }

    buf =mpp_buffer_get_ptr(p->frm_buf);

    memcpy(buf, yuv, len);

    ret = mpp_frame_init(&frame);
    if (ret) 
    {
        printf("mpp_frame_init failed\n");
        goto RET;
    }

    mpp_frame_set_width(frame, p->width);
    mpp_frame_set_height(frame, p->height);
    mpp_frame_set_hor_stride(frame, p->hor_stride);
    mpp_frame_set_ver_stride(frame, p->ver_stride);
    mpp_frame_set_fmt(frame, p->fmt);
    mpp_frame_set_buffer(frame, p->frm_buf);

    //usleep(100);
    ret = mpi->encode_put_frame(ctx, frame);
    if (ret) 
    {
        printf("mpp encode put frame failed\n");
        goto RET;
    }

    ret = mpi->encode_get_packet(ctx, &packet);
    if (ret) 
    {
        printf("mpp encode get packet failed\n");
        goto RET;
    }

    if (packet) 
    {
        // write packet to file here
        void *ptr   = mpp_packet_get_pos(packet);
        size_t len  = mpp_packet_get_length(packet);

        if(0 == p->frame_count%10)
        {
            //fwrite(pch264spspps, 1, 45, p->fp_output);
            rtp_transmit(ptr, pch264spspps, len, len0);
        }
        else
        {
            rtp_transmit(ptr, NULL, len, 0);
        }

        if (p->fp_output)
            //fwrite(ptr, 1, len, p->fp_output);
        mpp_packet_deinit(&packet);

        //printf("encoded frame %d size %d\n", p->frame_count, len);
        p->stream_size += len;
        p->frame_count++;
    }

RET: 
    return ret;

}


void encode2hardware(unsigned char *g_pImageData)
{
    if(NULL == g_pstTouPcam || NULL == g_pImageData || NULL == sock)
        return;

    MPP_RET ret = MPP_OK;
    unsigned long len = 0;
    unsigned char *pucyuvBuf = (unsigned char *)malloc(g_pstTouPcam->inHeight*g_pstTouPcam->inWidth*2);
    if(NULL == pucyuvBuf)
    {
        perror("encode2hardware");
        return;
    }
    
    //grb24toyuv420p(g_pImageData, g_pstTouPcam->inWidth, g_pstTouPcam->inHeight, pucyuvBuf, &len);
    //printf("yuv420 len:%d\n", len);

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
    int iy_size = width * height;
    memset(pucyuvBuf, 0, g_pstTouPcam->inHeight*g_pstTouPcam->inWidth*2);
    memcpy(pucyuvBuf, pFrameYUV->data[0], iy_size);
    memcpy(pucyuvBuf+iy_size, pFrameYUV->data[1], iy_size/4);
    memcpy(pucyuvBuf+iy_size+iy_size/4, pFrameYUV->data[2], iy_size/4);
	av_frame_free(&pFrameYUV);
    free(out_buffer);
    len = iy_size + iy_size/2;
    pFrameYUV = NULL;
    out_buffer = NULL;

#if 0        
    FILE *fp = fopen("./mcamera.yuv", "ab+");
    if(NULL == fp)
    {
        perror("mcamera.yuv");
        fclose(fp);
        return ;
    }
    fwrite(pucyuvBuf, len, 1, fp);
    fclose(fp);
#endif

    ret = mpp_encode(g_pstmpp_enc_data, pucyuvBuf, len);
    if (ret) 
    {
        printf("mpp_encode failed ret %d\n", ret);
    }

    free(pucyuvBuf);
    pucyuvBuf = NULL;

    return;
}


