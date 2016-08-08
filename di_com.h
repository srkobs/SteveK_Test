/*
 * di_com.h
 *
 *  Created on: Feb 28, 2014
 *      Author: Dave Murphy
 *
 * © 2014. Confidential and proprietary information of Georgia-Pacific Consumer Products LP. All rights reserved.
 */

#ifndef DI_COM_H_
#define DI_COM_H_

#define SOF_EOF 0x7e
#define ESC 0x7d

//The following defines constants that are specified for the framing of Dispenser Interface
#define MAX_FRAME_BYTES (MIN_FRAME_BYTES + MAXDATABYTES)
#define CMD_INDEX   1 //Index in frame that Command is in
#define CDATA_START 2 //Index in frame that Command Data starts
#define PDATA_START 3 //Index in frame that parameter data starts
#define MIN_FRAME_BYTES 5//Size in bytes of a frame where there is no command data.
                        //This is the smallest possible frame consisting of:
                        //SOF, Command, CRC, EOF = 5bytes
#define MIN_CRCD_FRAME_BYTES (MIN_FRAME_BYTES - 4)
#define MAX_CTS_ATTEMPTS 3 //This is the number of loops it will go through while waiting
						   //for CTS over interface before giving up

//Defines the number of clock cycles in 1ms by using the clock speed that is defined in di.h
#if(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_16Mhz)
#define NUM_CYCLES_1MS 16000
#endif	//(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_16Mhz)
#if(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_12Mhz)
#define NUM_CYCLES_1MS 12000
#endif	//(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_12Mhz)
#if(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_8Mhz)
#define NUM_CYCLES_1MS 8000
#endif	//(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_8Mhz)
#if(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_1Mhz)
#define NUM_CYCLES_1MS 1000
#endif	//(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_1Mhz)
#ifndef NUM_CYCLES_1MS
#error	Define number of cycles to get 1 ms
#endif //NUM_CYCLES_1MS
typedef enum
{
	RXTX_SOF,
	RXTX_NORMAL,
	RXTX_ESCAPE,
	RXTX_EOF
}RXTX_STATE;

uint8_t nextindex(uint8_t b, uint8_t i);
uint8_t lastindex(uint8_t b, uint8_t i);
void HandleDiRx(void);
void buildframe(uint8_t length, di_commandKey_t command);
void event(di_command_t * command);
uint16_t crc_ccitt_crc(void);
void crc_ccitt_update(uint8_t x);
void crc_ccitt_init(void);
void readCommMode();
void initUARTCommMode();
extern uint16_t outcon[16];
extern uint16_t incon[7];
extern uint8_t rxbuf[MAX_FRAME_BYTES + MAX_FRAME_BYTES];
extern uint8_t txbuf[MAX_FRAME_BYTES];
extern uint8_t rxindex;
void printf(char *,...);//TBDDWMdebug
#endif /* DI_COM_H_ */
