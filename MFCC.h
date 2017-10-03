
#ifndef _MFCC_H
#define _MFCC_H




#include <stdio.h>
#include "stdint.h"
#include "VAD.h"
#define hp_ratio	95/100			
#define fft_point	1024			
#define frq_max		(fft_point/2)	
#define hamm_top	10000			
#define	tri_top		1000			
#define tri_num		24				
#define mfcc_num	12				

#define vv_tim_max	1200	//?????????? ms
#define vv_frm_max	((vv_tim_max-frame_time)/(frame_time-frame_mov_t)+1)	//??????????
void cr4_fft_1024(void *pssOUT, void *pssIN, uint16_t Nbin);
#pragma pack(1)
typedef struct
{
	uint16_t save_sign;						//???? ????flash?????????
	uint16_t frm_num;						//??
	int16_t mfcc_dat[vv_frm_max*mfcc_num];	//MFCC????
}v_ftr_tag;								//???????
#pragma pack()

void get_mfcc(valid_tag *valid, v_ftr_tag *v_ftr, atap_tag *atap_arg);

#endif
