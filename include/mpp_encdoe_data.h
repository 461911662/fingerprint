#ifndef _MPP_ENCODE_DATA_H_
#define _MPP_ENCODE_DATA_H_

#include <stdint.h>
#include "common_toupcam.h"
#include "toupcam.h"
using namespace std;

#define MPP_ALIGN(x, a)         (((x)+(a)-1)&~((a)-1))

struct MPP_ENC_DATA
{
	// global flow control flag
	uint32_t frm_eos;
	uint32_t pkt_eos;
	uint32_t frame_count;
	uint64_t stream_size;

	// base flow context
	MppCtx ctx;
	MppApi *mpi;
	MppEncPrepCfg prep_cfg;
	MppEncRcCfg rc_cfg;
	MppEncCodecCfg codec_cfg;

	// input / output
	MppBuffer frm_buf;
	MppEncSeiMode sei_mode;

	uint32_t width;
	uint32_t height;
	uint32_t hor_stride;
	uint32_t ver_stride;
	MppFrameFormat fmt;
	MppCodingType type;
	uint32_t num_frames;

	// resources
	size_t frame_size;

	int32_t gop;
	int32_t fps;
	int32_t bps;

	FILE *fp_output;
    unsigned char *pImageData;
};


static MPP_ENC_DATA mpp_enc_data;

class MppEncodeData
{
public:
    MppEncodeData();
    MppEncodeData(int fd, uint32_t width, uint32_t height, uint32_t *pInputData);
    ~MppEncodeData();
    start();

private:
    uint32_t width;
    uint32_t height;
    uint32_t *pInputData;
    int size;
    int fd;
    
private:
    void init_mpp(uint32_t width, uint32_t height);
    void rtp_send();
    bool prcoess_image();
};

MppEncodeData::start()
{
}

MppEncodeData::rtp_send()
{
}

MppEncodeData::prcoess_image()
{
    MPP_RET ret = MPP_OK;
    MppFrame frame = NULL;
    MppPacket packet = NULL;
    
    if(mpp_enc_data.type == MPP_VIDEO_CodingAVC)
    {
        MppPacket packet = NULL;
        ret = mpp_enc_data.mpi->control(mpp_enc_data.ctx, MPP_ENC_GET_EXTRA_INFO, &packet);
        if(ret)
        {
            printf("mpi control enc get extra info failed\n");
            goto MPP_INIT_OUT;
        }

        /* get and write sps/pps for H.264 */
        if(packet)
        {
            void *ptr = mpp_packet_get_pos(packet);
            size_t len = mpp_packet_get_length(packet);
            if(mpp_enc_data.pImageData)
                memcpy(mpp_enc_data.pImageData, ptr, len);
            packet = NULL;
        }
    }

    void *buf = mpp_buffer_get_ptr(mpp_enc_data.frm_buf);
    //TODO: improve performance here?
    memcpy(buf, this.pInputData, this.size);

    ret = mpp_frame_init(&frame);
    if(ret)
    {
        printf("mpp_frame_init failed\n");
        return true;
    }

    mpp_frame_set_width(frame, mpp_enc_data.width);
    mpp_frame_set_height(frame, mpp_enc_data.height);
    mpp_frame_set_hor_stride(frame, mpp_enc_data.hor_stride);
    mpp_frame_set_ver_stride(frame, mpp_enc_data.ver_stride);
    mpp_frame_set_fmt(frame, mpp_enc_data.fmt);
    mpp_frame_set_buffer(frame, mpp_enc_data.frm_buf);
    mpp_frame_set_eos(frame, mpp_enc_data.frm_eos);        

    ret = mpp_enc_data.mpi->encode_put_frame(mpp_enc_data.ctx, frame);
    if (ret)
    {
    	printf("mpp encode put frame failed\n");
    	return true;
    }        
        
    ret = mpp_enc_data.mpi->encode_get_packet(mpp_enc_data.ctx, &packet);
    if (ret)
    {
    	printf("mpp encode get packet failed\n");
    	return true;
    }

    if (packet)
    {
    	// write packet to file here
    	void *ptr   = mpp_packet_get_pos(packet);
    	size_t len  = mpp_packet_get_length(packet);

    	mpp_enc_data.pkt_eos = mpp_packet_get_eos(packet);

    	//if (mpp_enc_data.fp_output)
    	//	fwrite(ptr, 1, len, mpp_enc_data.fp_output);
    	mpp_packet_deinit(&packet);

    	printf("encoded frame %d size %d\n", mpp_enc_data.frame_count, len);
    	mpp_enc_data.stream_size += len;
    	mpp_enc_data.frame_count++;

    	if (mpp_enc_data.pkt_eos)
    	{
    		printf("found last packet\n");
    	}
    }

    if (mpp_enc_data.num_frames && mpp_enc_data.frame_count >= mpp_enc_data.num_frames)
    {
    	printf("encode max %d frames", mpp_enc_data.frame_count);
    	return false;
    }

    if (mpp_enc_data.frm_eos && mpp_enc_data.pkt_eos)
    	return false;

    return true;

    
}



MppEncodeData::init_mpp()
{
        MPP_RET ret = MPP_OK;
        memset(&mpp_enc_data, 0, sizeof(mpp_enc_data));
    
        mpp_enc_data.width = this.width;
        mpp_enc_data.height = this.height;
        mpp_enc_data.hor_stride = MPP_ALIGN(mpp_enc_data.width, 16);
        mpp_enc_data.ver_stride = MPP_ALIGN(mpp_enc_data.height, 16);
        mpp_enc_data.fmt = MPP_FMT_BGR888;
        mpp_enc_data.frame_size = mpp_enc_data.hor_stride * mpp_enc_data.ver_stride * 2;
        mpp_enc_data.type = MPP_VIDEO_CodingAVC;
        mpp_enc_data.num_frames = 2000;
    
        ret = mpp_buffer_get(NULL, &mpp_enc_data.frm_buf, mpp_enc_data.frame_size);
        if(ret)
        {
            printf("failed to get buffer for input frame ret %d\n", ret);
            goto MPP_INIT_OUT;
        }
    
        ret = mpp_create(&mpp_enc_data.ctx, &mpp_enc_data.mpi);
        if(ret)
        {
            printf("mpp_create failed ret %d\n", ret);
            goto MPP_INIT_OUT;
        }
    
        mpp_enc_data.fps = 20;
        mpp_enc_data.gop = 60;
        mpp_enc_data.bps = mpp_enc_data.width * mpp_enc_data.height / 8 *mpp_enc_data.fps;
    
        mpp_enc_data.prep_cfg.change = MPP_ENC_PREP_CFG_CHANGE_INPUT |
            MPP_ENC_PREP_CFG_CHANGE_ROTATION |
            MPP_ENC_PREP_CFG_CHANGE_FORMAT;
        mpp_enc_data.prep_cfg.width = mpp_enc_data.width;
        mpp_enc_data.prep_cfg.height = mpp_enc_data.height;
        mpp_enc_data.prep_cfg.hor_stride = mpp_enc_data.hor_stride;
        mpp_enc_data.prep_cfg.ver_stride = mpp_enc_data.ver_stride;
        mpp_enc_data.prep_cfg.format = mpp_enc_data.fmt;
        mpp_enc_data.prep_cfg.rotation = MPP_ENC_ROT_0;
        ret =mpp_enc_data.mpi->control(mpp_enc_data.ctx, MPP_ENC_SET_PREP_CFG, &mpp_enc_data.prep_cfg);
        if(ret)
        {
            printf("mpi control enc set prep cfg failed ret %d\n", ret);
            goto MPP_INIT_OUT;
        }
    
        mpp_enc_data.rc_cfg.change = MPP_ENC_RC_CFG_CHANGE_ALL;
        mpp_enc_data.rc_cfg.rc_mode = MPP_ENC_RC_MODE_CBR;
        mpp_enc_data.rc_cfg.quality = MPP_ENC_RC_QUALITY_MEDIUM;
    
        if(mpp_enc_data.rc_cfg.rc_mode == MPP_ENC_RC_MODE_CBR)
        {
            /* constant bitrate has very small bps range of 1/16 bps */
            mpp_enc_data.rc_cfg.bps_target = mpp_enc_data.bps;
            mpp_enc_data.rc_cfg.bps_max = mpp_enc_data.bps * 17 / 16;
            mpp_enc_data.rc_cfg.bps_min = mpp_enc_data.bps * 15 / 16；
        }
        else if(mpp_enc_data.rc_cfg.rc_mode == MPP_ENC_RC_MODE_VBR)
        {
            if(mpp_enc_data.rc_cfg.quality == MPP_ENC_RC_QUALITY_CQP)
            {
                /* constant QP does not have bps */
                mpp_enc_data.rc_cfg.bps_target = -1;
                mpp_enc_data.rc_cfg.bps_max = -1;
                mpp_enc_data.rc_cfg.bps_min = -1;
            }
            else
            {
                /* variable bitrate has large bps range */
                mpp_enc_data.rc_cfg.bps_target = mpp_enc_data.bps;
                mpp_enc_data.rc_cfg.bps_max = mpp_enc_data.bps * 17 / 16；
                mpp_enc_data.rc_cfg.bps_min = mpp_enc_data.bps * 1 / 16;            
            }
        }
    
        /* fix input / output frame rate */
        mpp_enc_data.rc_cfg.fps_in_flex = 0;
        mpp_enc_data.rc_cfg.fps_in_num = mpp_enc_data.fps;
        mpp_enc_data.rc_cfg.fps_in_denorm = 1;
        mpp_enc_data.rc_cfg.fps_out_flex = 0;
        mpp_enc_data.rc_cfg.fps_out_num = mpp_enc_data.fps;
        mpp_enc_data.rc_cfg.fps_out_denorm = 1;
        mpp_enc_data.rc_cfg.gop = mpp_enc_data.gop;
        mpp_enc_data.rc_cfg.skip_cnt = 0;
        ret = mpp_enc_data.mpi->control(mpp_enc_data.ctx, MPP_ENC_SET_RC_CFG, &mpp_enc_data.rc_cfg);
        if(ret)
        {
            printf("mpi control enc set rc cfg failed ret %d\n", ret);
            goto MPP_INIT_OUT;
        }
    
        mpp_enc_data.codec_cfg.coding = mpp_enc_data.type;
        switch(mpp_enc_data.codec_cfg.coding)
        {
            case MPP_VIDEO_CodingAVC:
            {
                mpp_enc_data.codec_cfg.h264.change = MPP_ENC_H264_CFG_CHANGE_PROFILE |
                    MPP_ENC_H264_CFG_CHANGE_ENTROPY |
                    MPP_ENC_H264_CFG_CHANGE_TRANS_8x8;
                /*
                 * H.264 profile_idc parameter
                 * 66  - Baseline profile
                 * 77  - Main profile
                 * 100 - High profile
                 */
                mpp_enc_data.codec_cfg.h264.profile = 100;
                /*
                 * H.264 level_idc parameter
                 * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
                 * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
                 * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
                 * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
                 * 50 / 51 / 52         - 4K@30fps
                 */
                mpp_enc_data.codec_cfg.h264.level  = 31;
                mpp_enc_data.codec_cfg.h264.entropy_coding_mode = 1;
                mpp_enc_data.codec_cfg.h264.cabac_init_idc = 0;
                mpp_enc_data.codec_cfg.h264.transform8x8_mode = 1;
            }
            break;
            default:
            {
                printf("support encoder coding type %d\n", mpp_enc_data.codec_cfg.coding);
            }
            break;
        }
        ret = mpp_enc_data.mpi->control(mpp_enc_data.ctx, MPP_ENC_SET_CODEC_CFG, &mpp_enc_data.codec_cfg);
        if(ret)
        {
            printf("mpi control enc set codec cfg failed ret %d\n", ret);
            goto MPP_INIT_OUT;
        }
    
        /* optional */
        mpp_enc_data.sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
        ret = mpp_enc_data.mpi->control(mpp_enc_data.ctx, MPP_ENC_SET_SEI_CFG, &mpp_enc_data.sei_mode);
        if(ret)
        {
            printf("mpi control enc set sei cfg failed ret %d\n", ret);
            goto MPP_INIT_OUT;
        }
        
    MPP_INIT_OUT:
        if(mpp_enc_data.ctx)
        {
            mpp_destroy(mpp_enc_data.ctx);
            mpp_enc_data.ctx = NULL;
        }
    
        if(mpp_enc_data.frm_buf)
        {
            mpp_buffer_put(mpp_enc_data.frm_buf);
            mpp_enc_data.frm_buf = NULL;
        }
    
        printf("init mpp failed!\n");
}

MppEncodeData::MppEncodeData(int fd, uint32_t width, uint32_t height, uint32_t pInputData)
{
    this.height = height;
    this.width = width;
    this.fd = fd;
    this.pInputData = pInputData;
    this.size = TDIBWIDTHBYTES(width*24)*height
    cout<<"mpp encode data entry." <<endl;
}

MppEncodeData::~MppEncodeData()
{
	MPP_RET ret = MPP_OK;
	ret = mpp_enc_data.mpi->reset(mpp_enc_data.ctx);
	if (ret)
	{
		printf("mpi->reset failed\n");
	}

	if (mpp_enc_data.ctx)
	{
		mpp_destroy(mpp_enc_data.ctx);
		mpp_enc_data.ctx = NULL;
	}

	if (mpp_enc_data.frm_buf)
	{
		mpp_buffer_put(mpp_enc_data.frm_buf);
		mpp_enc_data.frm_buf = NULL;
	}
    cout<< "mpp encode data exit." <<endl;
}


#endif
