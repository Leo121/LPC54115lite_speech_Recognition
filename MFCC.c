/*******   MFCC.C    *******/

#include "arm_math.h"
#include "arm_const_structs.h"

#include <math.h>

#include "MFCC.H"
#include "MFCC_Arg.h"
#include <float.h>
#include "VAD.h"

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_dmic.h"
#include <stdlib.h>
#include <string.h>
#include "app_led.h"
#include "app_key.h"
#include "app_dmic.h"

#include "pin_mux.h"
#include <stdbool.h>
#include "stdint.h"



uint32_t fft_out[fft_point];	
uint32_t	fft_in[fft_point];	

/*	
	cr4_fft_1024输入参数是有符号数

	cr4_fft_1024输入参数包括实数和虚数
	但语音数据只包括实数部分 虚数用0填充
	fft点数超出输入数据长度时 超过部分用0填充
	
	cr4_fft_1024输出数据包括实数和虚数
	应该取其绝对值 即平方和的根
*/
uint32_t* fft(int16_t* dat_buf, uint16_t buf_len)
{
	uint16_t i;
	int32_t real,imag;
	
	if(buf_len>fft_point)
	{
		return (void*)0;
	}
	
	for(i=0;i<buf_len;i++)
	{
		fft_in[i]=*(uint16_t*)(dat_buf+i);//虚部高位 实部低位
		//PRINTF("fft_in[%d]=%d\r\n",i,(int16_t)fft_in[i]);
	}
	for(;i<fft_point;i++)
	{
		fft_in[i]=0;//超出部分用0填充
	}
	
	cr4_fft_1024(fft_out,fft_in,fft_point);
	
	for(i=0;i<frq_max;i++)
	{
		real=(int16_t)(fft_out[i]);
		imag=(int16_t)((fft_out[i])>>16);

		//PRINTF("%d %d",real,imag);

		real=real*real+imag*imag;
		//PRINTF(" %d",real);
		fft_out[i]=sqrtf((float)real)*10;
		//PRINTF(" %d\r\n",fft_out[i]);
	}
	return fft_out;
}

void cr4_fft_1024(void *pssOUT, void *pssIN, uint16_t Nbin)
{
  arm_cmplx_mag_f32(pssIN, pssOUT, Nbin);

}
/*
	MFCC：Mel频率倒谱系数
	
	参数：
	valid	有效语音段起点终点
	
	返回值：
	v_ftr	MFCC值，帧数
	
	Mel=2595*lg(1+f/700)
	1000Hz以下按线性刻度 1000Hz以上按对数刻度
	三角型滤波器中心频率 在Mel频率刻度上等间距排列
	预加重:6dB/倍频程 一阶高通滤波器  H(z)=1-uz^(-1) y(n)=x(n)-ux(n-1) u=0.94~0.97

	MFCC 步骤：
	1.对语音信号预加重、分帧、加汉明窗处理，然后进行短时傅里叶变换，得出频谱
	2.取频谱平方，得能量谱。并用24个Mel带通滤波器进行滤波，输出Mel功率谱
	3.对每个滤波器的输出值取对数，得到相应频带的对数功率谱。然后对24个对数功率进行
	  反离散余弦变换得到12个MFCC系数
*/

void get_mfcc(valid_tag *valid, v_ftr_tag *v_ftr, atap_tag *atap_arg)
{
	uint16_t *vc_dat;
	uint16_t h,i;
	uint32_t *frq_spct;			//??
	int16_t	vc_temp[frame_len];	//?????
	int32_t	temp;
	
	uint32_t	pow_spct[tri_num];	//????????????
	uint16_t frm_con;
	int16_t *mfcc_p;
	int8_t	*dct_p;
	int32_t mid;
	uint16_t v_frm_num;
	
	//PRINTF("start=%d end=%d",(uint32_t)(valid->start),(uint32_t)(valid->end));
	v_frm_num=(((uint32_t)(valid->end)-(uint32_t)(valid->start))/2-frame_len)/(frame_len-frame_mov)+1;
	if(v_frm_num>vv_frm_max)
	{
		PRINTF("frm_num=%d ",v_frm_num);
		v_ftr->frm_num=0;
	}
	else
	{
		mid=(int32_t)atap_arg->mid_val;
		mfcc_p=v_ftr->mfcc_dat;
		frm_con=0;
		for(vc_dat=(uint16_t*)(valid->start);vc_dat<=((uint16_t*)(valid->end-frame_len));vc_dat+=(frame_len-frame_mov))
		{
			for(i=0;i<frame_len;i++)
			{
				//???
				//PRINTF("vc_dat[%d]=%d ",i,*(vc_dat+i)-mid);
				temp=((int32_t)(*(vc_dat+i))-mid)-((int32_t)(*(vc_dat+i-1))-mid)*hp_ratio; 
				//PRINTF("vc_hp[%d]=%d ",i,temp);
				//???? ???10?
				vc_temp[i]=(int16_t)(temp*hamm[i]/(hamm_top/10));
				//PRINTF("vc_hm[%d]=%d\r\n",i,vc_temp[i]);
			}
			
			frq_spct=fft(vc_temp,frame_len);
			
			for(i=0;i<frq_max;i++)
			{
				//PRINTF("frq_spct[%d]=%d ",i,frq_spct[i]);
				frq_spct[i]*=frq_spct[i];//???
				//PRINTF("E_spct[%d]=%d\r\n",i,frq_spct[i]);
			}
			
			//??????
			pow_spct[0]=0;
			for(i=0;i<tri_cen[1];i++)
			{
				pow_spct[0]+=(frq_spct[i]*tri_even[i]/(tri_top/10));
			}
			for(h=2;h<tri_num;h+=2)
			{
				pow_spct[h]=0;
				for(i=tri_cen[h-1];i<tri_cen[h+1];i++)
				{
					pow_spct[h]+=(frq_spct[i]*tri_even[i]/(tri_top/10));
				}
			}
			
			for(h=1;h<(tri_num-2);h+=2)
			{
				pow_spct[h]=0;
				for(i=tri_cen[h-1];i<tri_cen[h+1];i++)
				{
					pow_spct[h]+=(frq_spct[i]*tri_odd[i]/(tri_top/10));
				}
			}
			pow_spct[tri_num-1]=0;
			for(i=tri_cen[tri_num-2];i<(fft_point/2);i++)
			{
				pow_spct[tri_num-1]+=(frq_spct[i]*tri_odd[i]/(tri_top/10));
			}
			
			//??????????
			for(h=0;h<tri_num;h++)
			{
				//PRINTF("pow_spct[%d]=%d ",h,pow_spct[h]);
				pow_spct[h]=(uint32_t)(log(pow_spct[h])*100);//???? ?100 ????????
				//PRINTF("%d\r\n",pow_spct[h]);
			}
			
			//???????
			dct_p=(int8_t *)dct_arg;
			for(h=0;h<mfcc_num;h++)
			{
				mfcc_p[h]=0;
				for(i=0;i<tri_num;i++)
				{
					mfcc_p[h]+=(((int32_t)pow_spct[i])*((int32_t)dct_p[i])/100);
				}
				//PRINTF("%d,",mfcc_p[h]);
				dct_p+=tri_num;
			}
			//PRINTF("\r\n");
			mfcc_p+=mfcc_num;
			frm_con++;
		}
		
		v_ftr->frm_num=frm_con;
	}
}

	
