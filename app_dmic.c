/*
 
 */
#include "board.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"

#include "fsl_common.h"
#include "fsl_iocon.h"
#include "fsl_dmic.h"
#include "fsl_dma.h"
#include "fsl_dmic_dma.h"

#include <stdbool.h>

#include "app_led.h"
#include "app_key.h"
#include "app_dmic.h"

#include "MFCC.h"
#include "VAD.h"
#include "DTW.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
//#define FIFO_DEPTH 15U

#define DMAREQ_DMIC0 16U
#define DMAREQ_DMIC1 17U
#define APP_DMAREQ_CHANNEL DMAREQ_DMIC0
#define APP_DMIC_CHANNEL kDMIC_Channel0
#define APP_DMIC_CHANNEL_ENABLE DMIC_CHANEN_EN_CH0(1)
#define FIFO_DEPTH 15U
#define BUFFER_LENGTH 32U
//#define BUFFER_LENGTH 16000U

//#define RD_SIZE 16384
//#define WR_SIZE 256

//uint8_t g_WRBuf[WR_SIZE] = {0};
//uint8_t g_RDBuf[RD_SIZE] = {0};
//uint32_t g_AddrWRBuf;
//uint32_t g_AddrFlash;

//#define REC_SIZE 524288

#define save_ok		0
#define VAD_fail	1
#define MFCC_fail	2
#define Flash_fail	3

/* DMIC user callback */
void DMIC_UserCallback(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData);
//uint8_t* spch_recg(uint16_t *v_dat, uint32_t *mtch_dis);

uint16_t g_rxBuffer[BUFFER_LENGTH] = {0};


dmic_dma_handle_t g_dmicDmaHandle;
dma_handle_t g_dmicRxDmaHandle;
volatile bool g_Transfer_Done = false;
volatile uint16_t min_comm;
volatile bool record_module_flag=false;
volatile bool record_flag=false;

dmic_transfer_t receiveXfer;
atap_tag	atap_arg;
valid_tag	valid_voice[max_vc_con];
v_ftr_tag	ftr;
v_ftr_tag ftr_mdl;
typedef struct
{
	uint8_t str[3];
}comm_tag;

comm_tag commstr[]={"0 ","1 ","2 ","3 ","4 ","5 ","6 ","7 ","8 ","9 "};


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
* @brief Call back for DMIC0 Interrupt
*/
void DMIC0_Callback(void)
{
    /* In this example this interrupt is disabled */
}
void DMIC_UserCallback(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData)
{
    userData = userData;
    if (status == kStatus_DMIC_Idle)
    {
        g_Transfer_Done = true;
    }
}

/*!
* @brief Call back for DMIC0 HWVAD Interrupt
*/
void DMIC0_HWVAD_Callback(void)
{
	uint8_t *res;
	uint32_t dis;
    volatile int i;

    led_toggle(3);
    /* reset hwvad internal interrupt */
    DMIC_CtrlClrIntrHwvad(DMIC0, true);
    /* wait for HWVAD to settle */
		 PRINTF("Just woke up!\r\n");
		PRINTF("test dma\r\n");
		 
	
    receiveXfer.dataSize = 2 * BUFFER_LENGTH;
	 receiveXfer.data = g_rxBuffer;
			
	  DMIC_TransferReceiveDMA(DMIC0, &g_dmicDmaHandle, &receiveXfer, APP_DMIC_CHANNEL);
	
	while(1)
	{
	
		for(i=0;i<32;i++)
				{
//					g_WRBuf[g_AddrWRBuf+i*2+0]=( (g_rxBuffer[i]>>8) &0x00ff );
//					g_WRBuf[g_AddrWRBuf+i*2+1]=( g_rxBuffer[i] &0x00ff );					
				}
	
	
	}
//for (i = 0; i < sizeof(g_WRBuf); i++)
//    {
//        PRINTF("%d\t,", g_WRBuf[i]);
//			if(i%4==0)
//				 PRINTF("\r\n");
//    }
		PRINTF("\r\n");
//		if(key_value(0)==0)
//			{
//				PRINTF("Please record module !");
//				record_flag =false;
				//record_module_flag=true;
//			}

//			if(key_value(1)==0)
//			{
	//			record_flag=true;
				//record_module_flag=false;
//			
//			}
			if(record_module_flag)
			{
				//noise_atap(g_rxBuffer,atap_len,&atap_arg);
				VAD(g_rxBuffer,BUFFER_LENGTH,valid_voice, &atap_arg);
					if(valid_voice[0].end==((void *)0))
					{
						printf("VAD_mdl_fail \r\n");
						return ;//VAD_fail;
					}
	
				get_mfcc(&(valid_voice[0]),&ftr_mdl,&atap_arg);
				//get_mfcc(&(g_mdlBuffer[0]),&ftr,&atap_arg);
				if(ftr_mdl.frm_num==0)
				{
				//	*mtch_dis=dis_err;
					printf("MFCC mdl fail \r\n");
					return ;//(void *)0;
				}
				record_module_flag=false;
				printf("pls input command args:\r\n");
				//min_comm=getchar();
			}
//			if(record_flag)
//			{
//				res=spch_recg(g_rxBuffer,&dis);
//				if(dis!=dis_err)
//				{
//					printf("LED %d  is on",*res);
//					led_on(*res);
//				
//				}
//			
//				record_flag=false;
//			}
			
    /* Wait for DMA transfer finish */
     while (g_Transfer_Done == false)
    {
    }
    
    /*HWVAD Normal operation */
    DMIC_CtrlClrIntrHwvad(DMIC0, false);
	
		led_toggle(3);
}

uint8_t dmic_dma_init(void)
{
	DMIC_SetOperationMode(DMIC0, kDMIC_OperationModeDma);
	
	dmic_channel_config_t dmic_channel_cfg;

    dmic_transfer_t receiveXfer;
	 uint32_t i;

 PRINTF("\r\nConfigure DMA\r\n");

    DMA_Init(DMA0);

    DMA_EnableChannel(DMA0, APP_DMAREQ_CHANNEL);

    /* Request dma channels from DMA manager. */
    DMA_CreateHandle(&g_dmicRxDmaHandle, DMA0, APP_DMAREQ_CHANNEL);

    /* Create DMIC DMA handle. */
    DMIC_TransferCreateHandleDMA(DMIC0, &g_dmicDmaHandle, DMIC_UserCallback, NULL, &g_dmicRxDmaHandle);
    receiveXfer.dataSize = 2 * BUFFER_LENGTH;
    receiveXfer.data = g_rxBuffer;
    PRINTF("\r\nBuffer Data before transfer \r\n");
//    for (i = 0; i < BUFFER_LENGTH; i++)
//    {
//        PRINTF("%d\t", g_rxBuffer[i]);
//			if(i%4==0)
//				 PRINTF("\r\n");
//    }
    DMIC_TransferReceiveDMA(DMIC0, &g_dmicDmaHandle, &receiveXfer, APP_DMIC_CHANNEL);

    /* Wait for DMA transfer finish */
    while (g_Transfer_Done == false)
    {
    }

    PRINTF("\r\nTransfer completed\r\n");
    PRINTF("\r\nBuffer Data after transfer \r\n");
//    for (i = 0; i < BUFFER_LENGTH; i++)
//    {
//        PRINTF("%d\t", g_rxBuffer[i]);
//			if(i%4==0)
//				 PRINTF("\r\n");
//    }
return 1;
}

uint8_t dmic_init(void)
{	
	uint8_t ret = 0;
	uint32_t i = 0;
	dmic_channel_config_t dmic_channel_cfg;
	
	/* PDM interface */
	IOCON_PinMuxSet(IOCON, 1, 15, IOCON_FUNC1 | IOCON_DIGITAL_EN);  /* PDM CLK  0 */
	IOCON_PinMuxSet(IOCON, 1, 16, IOCON_FUNC1 | IOCON_DIGITAL_EN);  /* PDM DATA 0 */
	
	/* DMIC uses 12MHz FRO clock */
  CLOCK_AttachClk(kFRO12M_to_DMIC);
	/*12MHz divided by 15 = 800KHz PDM clock */
	CLOCK_SetClkDiv(kCLOCK_DivDmicClk, 14, false);
	dmic_channel_cfg.divhfclk = kDMIC_PdmDiv1;
	dmic_channel_cfg.osr = 25U;
	//dmic_channel_cfg.gainshft = 3U;
	dmic_channel_cfg.gainshft = 0U;
	dmic_channel_cfg.preac2coef = kDMIC_CompValueZero;
	dmic_channel_cfg.preac4coef = kDMIC_CompValueZero;
	dmic_channel_cfg.dc_cut_level = kDMIC_DcCut155;
	dmic_channel_cfg.post_dc_gain_reduce = 1U;
	dmic_channel_cfg.saturate16bit = 1U;
	dmic_channel_cfg.sample_rate = kDMIC_PhyFullSpeed;
	DMIC_Init(DMIC0);
	
	DMIC_ConfigIO(DMIC0, kDMIC_PdmStereo);
	DMIC_Use2fs(DMIC0, true);
	
	DMIC_SetOperationMode(DMIC0, kDMIC_OperationModeDma);
	DMIC_ConfigChannel(DMIC0, APP_DMIC_CHANNEL, kDMIC_Left, &dmic_channel_cfg);

	DMIC_FifoChannel(DMIC0, APP_DMIC_CHANNEL, FIFO_DEPTH, true, true);

	DMIC_EnableChannnel(DMIC0, APP_DMIC_CHANNEL_ENABLE);
	
	
		PRINTF("Configure DMA\r\n");
		
    DMA_Init(DMA0);

    DMA_EnableChannel(DMA0, APP_DMAREQ_CHANNEL);

    /* Request dma channels from DMA manager. */
    DMA_CreateHandle(&g_dmicRxDmaHandle, DMA0, APP_DMAREQ_CHANNEL);

    /* Create DMIC DMA handle. */
    DMIC_TransferCreateHandleDMA(DMIC0, &g_dmicDmaHandle, DMIC_UserCallback, NULL, &g_dmicRxDmaHandle);
    receiveXfer.dataSize = 2 * BUFFER_LENGTH;
    receiveXfer.data = g_rxBuffer;
    PRINTF("Buffer Data before transfer \r\n");
    for (i = 0; i < BUFFER_LENGTH; i++)
    {
        PRINTF("%d\r\n", g_rxBuffer[i]);
    }
    DMIC_TransferReceiveDMA(DMIC0, &g_dmicDmaHandle, &receiveXfer, APP_DMIC_CHANNEL);

    /* Wait for DMA transfer finish */
    while (g_Transfer_Done == false)
    {
    }

    PRINTF("Transfer completed\r\n");
    PRINTF("Buffer Data after transfer \r\n");
    for (i = 0; i < BUFFER_LENGTH; i++)
    {
        PRINTF("%d\r\n", g_rxBuffer[i]);
    }	
	
	/*
	DMIC_SetOperationMode(DMIC0, kDMIC_OperationModeInterrupt);
	DMIC_ConfigChannel(DMIC0, kDMIC_Channel0, kDMIC_Left, &dmic_channel_cfg);
	DMIC_ConfigChannel(DMIC0, kDMIC_Channel1, kDMIC_Right, &dmic_channel_cfg);
	DMIC_FifoChannel(DMIC0, kDMIC_Channel0, FIFO_DEPTH, true, true);
	DMIC_FifoChannel(DMIC0, kDMIC_Channel1, FIFO_DEPTH, true, true);
	
	//Gain of the noise estimator 
	DMIC_SetGainNoiseEstHwvad(DMIC0, 0x02U);
	//Gain of the signal estimator 
	DMIC_SetGainSignalEstHwvad(DMIC0, 0x01U);
	// 00 = first filter by-pass, 01 = hpf_shifter=1, 10 = hpf_shifter=4 
	DMIC_SetFilterCtrlHwvad(DMIC0, 0x01U);
	//input right-shift of (GAIN x 2 -10) bits (from -10bits (0000) to +14bits (1100)) 
	DMIC_SetInputGainHwvad(DMIC0, 0x04U);
	DisableDeepSleepIRQ(HWVAD0_IRQn);
	DMIC_HwvadEnableIntCallback(DMIC0, DMIC0_HWVAD_Callback);
	DMIC_EnableChannnel(DMIC0, (DMIC_CHANEN_EN_CH0(1) | DMIC_CHANEN_EN_CH1(1)));
	// reset hwvad internal interrupt 
	DMIC_CtrlClrIntrHwvad(DMIC0, true);
	// To clear first spurious interrupt 
	for (i = 0; i < 0xFFFFU; i++)
	{
	}
	//HWVAD Normal operation 
	DMIC_CtrlClrIntrHwvad(DMIC0, false);
	NVIC_ClearPendingIRQ(HWVAD0_IRQn);
	EnableDeepSleepIRQ(HWVAD0_IRQn);
*/
	
	
	

	
	ret = 1;
	
	return ret;
}
uint8_t* spch_recg(uint16_t *v_dat, uint32_t *mtch_dis)
{
	uint16_t i;
	uint32_t ftr_addr;
	uint32_t min_dis;
	uint16_t min_comm; //语音模板的返回值
	uint32_t cur_dis;
	//v_ftr_tag *ftr_mdl_tmp;
	
	noise_atap(v_dat, atap_len, &atap_arg);
	
	VAD(v_dat, VcBuf_Len, valid_voice, &atap_arg);
	if(valid_voice[0].end==((void *)0))
	{
		*mtch_dis=dis_err;
		PRINTF("VAD fail ");
		return (void *)0;
	}
	
	get_mfcc(&(valid_voice[0]),&ftr,&atap_arg);
	if(ftr.frm_num==0)
	{
		*mtch_dis=dis_err;
		PRINTF("MFCC fail ");
		return (void *)0;
	}
	
	//i=0;
	min_comm=3; //本例我用了语音模板数字3，识别成功就会返回此数值
	min_dis=dis_max;
//	for(ftr_addr=ftr_start_addr; ftr_addr<ftr_end_addr; ftr_addr+=size_per_ftr)
//	{
//		ftr_mdl=(v_ftr_tag*)ftr_addr;
		//printf("save_mask=%d ",ftr_mdl->save_sign);
		//cur_dis=((ftr_mdl->save_sign)==save_mask)?dtw(&ftr,ftr_mdl):dis_err;
		
		cur_dis=dtw(&ftr,&ftr_mdl);
		//printf("cur_dis=%d ",cur_dis);
		if(cur_dis<min_dis)
		{
			min_dis=cur_dis;
			//min_comm=i;
		}
//		i++;
//	}
	//min_comm/=4;
	//printf("recg end ");
	*mtch_dis=min_dis;
	return (commstr[min_comm].str);
}

// end file
