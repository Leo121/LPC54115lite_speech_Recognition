#ifndef _VAD_H
#define _VAD_H
#include "stdint.h"

#define fs			8000					//ADC������ Hz
#define	voice_len	2000					//¼��ʱ�䳤�� ��λms
#define	VcBuf_Len	((fs/1000)*voice_len)	//�������������� ��λ���� ÿ��������16λ
#define atap_len_t	300						//������������Ӧʱ�䳤�� ms
#define atap_len	((fs/1000)*atap_len_t)	//������������Ӧ����

#define	max_vc_con	3	//VAD��������������
#define frame_time	20						// ÿ֡ʱ�䳤�� ��λms
#define frame_mov_t	10						// ֡��
#define frame_len	(frame_time*fs/1000)	// ֡��	
#define frame_mov	(frame_mov_t*fs/1000)	// ֡�ƣ�����֡��������	

typedef struct
{
	uint32_t mid_val;	//��������ֵ �൱���з��ŵ�0ֵ ���ڶ�ʱ�����ʼ���
	uint16_t	n_thl;		//������ֵ�����ڶ�ʱ�����ʼ���
	uint16_t z_thl;		//��ʱ��������ֵ����������ֵ����Ϊ������ɶΡ�
	uint32_t s_thl;		//��ʱ�ۼӺ���ֵ����������ֵ����Ϊ������ɶΡ�
}atap_tag;			//����Ӧ����

typedef struct
{
	uint16_t *start;	//��ʼ��
	uint16_t *end;	//������
}valid_tag;	//��Ч������

void noise_atap(const uint16_t* noise,uint16_t n_len,atap_tag* atap);
void VAD(const uint16_t *vc, uint16_t buf_len, valid_tag *valid_voice, atap_tag *atap_arg);

#endif
