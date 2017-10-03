/*

 */

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_dmic.h"
#include <stdlib.h>
#include <string.h>
#include "app_led.h"
#include "app_key.h"
#include "app_dmic.h"
#include "fsl_dmic_dma.h"
#include "pin_mux.h"
#include <stdbool.h>

#include "MFCC.h"
#include "VAD.h"
#include "DTW.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define FIFO_DEPTH 15U

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
* Variables
******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/


#define SECTOR_ADDR 0
#define PAGE_SIZE 256
#define SECTOR_SIZE 65536
#define FLASH_SPI_SSEL 3
#define EXAMPLE_SPI_MASTER SPI2
#define EXAMPLE_SPI_MASTER_CLK_SRC kCLOCK_Flexcomm2
#define EXAMPLE_SPI_MASTER_CLK_FREQ CLOCK_GetFreq(kCLOCK_Flexcomm2)
#define BUFFER_SIZE 64

#define APP_DMIC_CHANNEL kDMIC_Channel0

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void delay(void);



enum SysSta {S_ON,S_PLAY,S_REC,S_TEST};
enum SysSta g_SysSta;

//16K=16384 8k=8192 16k=16384 64K=65536 128K=131072 512k=524288 1M=1048576
#define RD_SIZE 1024
#define WR_SIZE 256
#define REC_SIZE 16384


uint16_t g_spchBuf[REC_SIZE]={0};
__align(4) uint8_t g_RDBuf[RD_SIZE] __attribute__((aligned(4))) = {0};
__align(4) uint8_t g_WRBuf[WR_SIZE] __attribute__((aligned(4))) = {0};
__align(4) uint8_t g_RecBuf0[RD_SIZE] __attribute__((aligned(4))) = {0};
__align(4) uint8_t g_RecBuf1[RD_SIZE] __attribute__((aligned(4))) = {0};

bool g_BufSet=0;
bool g_RecSel=0;
bool g_RecFlag=0;
bool g_RecTest=0;

uint32_t g_RecAddr;
uint16_t g_RecVol=0x0010;
uint32_t g_AddrWRBuf;
uint32_t g_AddrFlash;

extern volatile bool g_Transfer_Done;

extern uint16_t g_rxBuffer[32];
extern dmic_dma_handle_t g_dmicDmaHandle;
extern dmic_transfer_t receiveXfer;
extern char g_buffer[BUFFER_SIZE];
extern atap_tag	atap_arg;
extern valid_tag	valid_voice[max_vc_con];
extern v_ftr_tag	ftr;
extern v_ftr_tag ftr_mdl;

#define save_ok		0
#define VAD_fail	1
#define MFCC_fail	2
#define Flash_fail	3
/*******************************************************************************
 * Code
 ******************************************************************************/
void delay(void)
{
    volatile uint32_t i = 0;
    for (i = 0; i < 100000; ++i)
    {
        __asm("NOP"); /* delay */
    }
}



int main(void)
{
	


	
	uint32_t i;
	
	uint8_t *res;
	uint32_t dis;

    dmic_channel_config_t dmic_channel_cfg;

    /* Board pin, clock, debug console init */
    CLOCK_EnableClock(kCLOCK_InputMux);
    CLOCK_EnableClock(kCLOCK_Gpio0);
    CLOCK_EnableClock(kCLOCK_Gpio1);

    /* USART0 clock */
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    

    BOARD_InitPins();
    BOARD_BootClockFROHF96M();
    BOARD_InitDebugConsole();

		led_init();
		key_init();
		PRINTF("Hello,NXP!\r\n");
    PRINTF("Configure DMIC\r\n");

   dmic_init(); //初始化dmic，并且利用dmic_hwvad回调函数实现有效的语音数据采集，并且透过DMA传给RAM
	
	// dmic_dma_init();//初始化DMA
		 


again:    PRINTF("=================================================!\r\n");
    while (1)
    {
			
				if( (key_value(1) == 0) )
				{
					g_RecTest=!g_RecTest;
					//led_on(2);
					PRINTF("Start to record speech file!\r\n");
					while(key_value(1) == 0);
					//led_off(2);
					g_SysSta = S_PLAY;
				g_AddrWRBuf=0;
					g_AddrFlash=0;
					g_Transfer_Done = false;
					DMIC_TransferReceiveDMA(DMIC0, &g_dmicDmaHandle, &receiveXfer, APP_DMIC_CHANNEL);		
					break;
				}
		
				if( (key_value(0) == 0) )
				{
					//led_on(2);
					PRINTF("Start to Record mdl file!\r\n");
					while(key_value(0) == 0);
					//led_off(2);
					g_RecTest = 1;
					g_SysSta = S_REC;
				


					
					g_AddrWRBuf=0;
					g_AddrFlash=0;
					g_Transfer_Done = false;
					DMIC_TransferReceiveDMA(DMIC0, &g_dmicDmaHandle, &receiveXfer, APP_DMIC_CHANNEL);		
					break;
				}
		
	}
		
	while (1)
		{

		
			if(g_SysSta == S_PLAY)
			{
				while (1)
					{

							/* Wait for DMA transfer finish */
							while (g_Transfer_Done == false)
							{
							}

							for(i=0;i<32;i++)
							{
		
								g_spchBuf[g_AddrWRBuf+i]=g_rxBuffer[i]; //录制语音到缓存数组
							}
							g_AddrWRBuf=g_AddrWRBuf+32;
							//g_AddrWRBuf = g_AddrWRBuf +64;
							g_AddrFlash = g_AddrFlash +32;
							
							g_Transfer_Done = false;
							DMIC_TransferReceiveDMA(DMIC0, &g_dmicDmaHandle, &receiveXfer, APP_DMIC_CHANNEL);
								if(g_AddrFlash>=REC_SIZE)
							{
								PRINTF("spch is finished!\r\n");
								PRINTF("begin to process VAD=>MFCC!");
									
										res=spch_recg(g_spchBuf, &dis); //语音识别
										if(dis!=dis_err)//识别成功
										{
											PRINTF("speech command is valid\r\n");
											PRINTF("led%d  is on ",*res-48);
											led_on(*res-48);
										}
											else
										{
											PRINTF("Unknow speech command return\r\n!");
										}
//										
														goto again;

							}			
							
				
			}
		}
			else if(g_SysSta == S_REC)
			{							
					while (1)
					{

							/* Wait for DMA transfer finish */
							while (g_Transfer_Done == false)
							{
							}

							for(i=0;i<32;i++)
							{

								g_spchBuf[g_AddrWRBuf+i]=g_rxBuffer[i];//语音模板数据缓存
							}
							g_AddrWRBuf=g_AddrWRBuf+32;
							//g_AddrWRBuf = g_AddrWRBuf +64;
							g_AddrFlash = g_AddrFlash +32;
							
							g_Transfer_Done = false;
							DMIC_TransferReceiveDMA(DMIC0, &g_dmicDmaHandle, &receiveXfer, APP_DMIC_CHANNEL);

			//==============================================================================

							
							if(g_AddrFlash>=REC_SIZE)
							{
								PRINTF("Mdl is finished!\r\n");
								PRINTF("begin to process VAD=>MFCC!\r\n");
									noise_atap(g_spchBuf,atap_len,&atap_arg); //噪音处理
										VAD(g_spchBuf,REC_SIZE,valid_voice, &atap_arg);//有效语音数据处理
											if(valid_voice[0].end==((void *)0))
											{
												printf("VAD_mdl_fail \r\n");
												return VAD_fail;
											}
							
										get_mfcc(&(valid_voice[0]),&ftr_mdl,&atap_arg);//mfcc处理，获取特征值
										//get_mfcc(&(g_mdlBuffer[0]),&ftr,&atap_arg);
										if(ftr_mdl.frm_num==0)
										{
										//	*mtch_dis=dis_err;
											printf("MFCC mdl fail \r\n");
											return MFCC_fail;
										}
											printf("MFCC mdl is successful! \r\n");	
//											led_off(0);led_off(1);led_off(2);led_off(3);
//														led_off(4);led_off(5);led_off(6);led_off(7);
														goto again;

							}			
					}							
			}
		}		
	}

