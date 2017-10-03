#ifndef _DTW_H
#define _DTW_H
#include "stdint.h"
#include "VAD.h"
#define dis_err	0xFFFFFFFF
#define dis_max	0xFFFFFFFF

uint32_t dtw(v_ftr_tag *ftr_in, v_ftr_tag *frt_mdl);

#endif


