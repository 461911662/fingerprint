#ifndef _MPP_ENCODE_DATA_H_
#define _MPP_ENCODE_DATA_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "rockchip/rk_mpi.h"
#include "codeimg.h"
#include "common_toupcam.h"
#include "toupcam.h"

#define MPP_ALIGN(x, a)         (((x)+(a)-1)&~((a)-1))

typedef struct MPP_ENC_DATA
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
}MPP_ENC_DATA_S;

#endif
