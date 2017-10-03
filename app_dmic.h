
#ifndef _APP_DMIC_H_
#define _APP_DMIC_H_


extern uint8_t dmic_init(void);
extern uint8_t dmic_dma_init(void);
extern uint8_t* spch_recg(uint16_t *v_dat, uint32_t *mtch_dis);
#endif
