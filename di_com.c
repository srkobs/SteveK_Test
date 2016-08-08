/*
 * di_com.c
 *
 *  Created on: Feb 28, 2014
 *      Author: Dave Murphy
 *
 *
 * © 2014. Confidential and proprietary information of Georgia-Pacific Consumer Products LP. All rights reserved.
 */
#include <stdint.h>
#include <msp430.h>
#include "stdarg.h"
#include "di.h"
#ifdef DISP_ENMOTION_CLASSIC
#include "../input.h"
#include "../UserSettings.h"
#endif //DISP_ENMOTION_CLASSIC
#if defined(DISP_ENMOTION_SOAP_CAP) || defined(DISP_SAFEHAVEN_SOAP)
#include "../typeDef.h"
#endif //DISP_ENMOTION_SOAP_CAP

static uint16_t crc;
uint8_t txbuf[MAX_FRAME_BYTES]={0};
uint8_t rxindex= 0;
uint8_t rxbuf[MAX_FRAME_BYTES + MAX_FRAME_BYTES]={0}; //rx buffer is doubled to account for worst case Escapes
static void sendtxbuf( uint8_t length);
#ifdef GPIO_EVENTS_ENABLED
static void sendByte(uint8_t byte );
static ConfigGPIOAsOutput(GPIO_T gpio);

#endif //GPIO_EVENTS_ENABLED
#ifndef DISP_EAGLE
//TODO: Add Meaningful Comment
void sendByte(uint8_t byte )
{
	//TODO: Add timeout function. In SPI Mode, device is slave.
	//		When no master is connected, this becomes an infinite loop.
    while (!(IFG2&UCA0TXIFG)) ;			// USCI_A0 TX buffer ready?

    // Add byte to transmit buffer. Processor will take care of the rest.
    UCA0TXBUF = byte;
}
#else
void sendByte(uint8_t byte)
{
	while(!(UCA1IFG&UCTXIFG));			// USCI_A1 TX buffer ready?

	UCA1TXBUF = byte;
}
#endif	//DISP_EAGLE

//TODO: Add Meaningful Comment
void HandleDiRx(void)
{
    uint16_t temp;
    uint8_t ptemp;
    uint8_t numSeqEscapes = 0;
    uint8_t sof = sizeof(rxbuf);
    uint8_t eof = sizeof(rxbuf);
    static uint8_t cindex = 0;

    temp = cindex;
    ptemp = lastindex(1,cindex);

    while(temp != rxindex)  //lets check all the bytes that have been produced
    {
    	//Seek out start of frame
        if (rxbuf[temp]== SOF_EOF && sof == sizeof(rxbuf))
        {
            sof = temp;
        }
        //Seek out end of frame, and make sure it is not escaped
        else if ((rxbuf[temp]== SOF_EOF && rxbuf[ptemp] != ESC) ||
                 (rxbuf[temp]== SOF_EOF && rxbuf[ptemp] == ESC && (numSeqEscapes & 0x01) == 0))
        {
        	//Potential end of frame found
            eof = temp;
            cindex = temp;
            //If crc passes, performDiCommand returns true which means we can
            //update the pointers past the last eof.
            if (performDiCommand(sof, eof) != FALSE)
            {
                cindex = nextindex(1,temp);
            }
            else //bad frame, so start looking for new frame again starting at the eof
                 //of the bad frame
            {
                sof = eof;
            }
        }
        //Sum up any escape characters that are sequential. This will help determine
        //if an EOF is real or just an escaped character and not the actual EOF. The actual EOF
        //is not escaped, and therefore an even number of sequential ESC characters must come before it.
        //E.G. if a frame ends with the crc value being 7D7E, the frame would be escaped as follows:
        // ... 7D 7D 7D 7E 7E where the second 7E is the true EOF, and the first is just the escaped crc.
        // For this example when the first 7E (the false EOF) is evaluated, numSeqEscapes will be 3, an odd
        // number and therefore not a real EOF.
        if (rxbuf[temp] == ESC)
        {
            numSeqEscapes++;
        }
        else
        {
            numSeqEscapes = 0;
        }

        ptemp = temp;
        temp = nextindex(1,temp);

    }
}

//TODO: Add Meaningful Comment
uint8_t nextindex(uint8_t b, uint8_t i )
{
	while(b)
	{
        i++;
        b--;
        if (i >= sizeof(rxbuf))
        {
            i=0;
        }
	}
    return i;
}

//TODO: Add Meaningful Comment
uint8_t lastindex(uint8_t b, uint8_t i )
{
	while(b)
	{
        i--;
        b--;
        if (i >= sizeof(rxbuf))
        {
            i=sizeof(rxbuf) - 1;
        }
	}
    return i;
}

//TODO: Add Meaningful Comment
void crc_ccitt_init(void)
{
    crc = 0xFFFF;
}

//TODO: Add Meaningful Comment
//TODO: Determine if we can fix this CRC routine. Per this article,
//		http://srecord.sourceforge.net/crc16-ccitt.html, it is the
//		"bad" CRC.
//		Setting initial value to 0xFFFF, "123456789" ASCII string
//		calculates to
void crc_ccitt_update(uint8_t x)
{
    uint16_t crc_new = (uint8_t)(crc >> 8) | (crc << 8);
    crc_new ^= x;
    crc_new ^= (uint8_t)(crc_new & 0xff) >> 4;
    crc_new ^= crc_new << 12;
    crc_new ^= (crc_new & 0xff) << 5;
    crc = crc_new;
}

//TODO: Add Meaningful Comment
uint16_t crc_ccitt_crc(void)
{
    return crc;
}


//TODO: Add Meaningful Comment
void event(di_command_t * command)
{
	inputEventData = command->paramData[0];
	triggerEvent(command->paramKey);
}




//TODO: Add Meaningful Comment
void sendtxbuf( uint8_t length)
{
	RXTX_STATE txState = RXTX_SOF;
	uint16_t i =0;
	uint16_t numAttempts = 0;

	if (inputEventConfigs[IE_CLEAR_TO_SEND - START_IE] & ECFG_ENABLED_ON_GPIO)
	{
		while(!(getGPIO((GPIO_T)((inputEventConfigs[IE_CLEAR_TO_SEND - START_IE] & ECFG_GPIO_BITS)>>2))) &&
				numAttempts <= MAX_CTS_ATTEMPTS)
		{
			numAttempts++;
			__delay_cycles(NUM_CYCLES_1MS);
			#ifdef USE_WDT
			WDTCTL = FEED_WDT;
			#endif	//USE_WDT
		}
	}

	//Only transmit just prior to the EOF to handle any necessary escapes
	while(i < length - 1)
	{
		switch (txState)
		{
			case RXTX_SOF:
				sendByte(txbuf[i]);
				i++;
				txState = RXTX_NORMAL;
				break;

			case RXTX_NORMAL:
				if (txbuf[i] == SOF_EOF || txbuf[i] == ESC)
				{
					sendByte(ESC);
					txState = RXTX_ESCAPE;
					//don't increment i for escape char
				}
				else //char doesn't need escape
				{
					sendByte(txbuf[i]);
					i++;
				}
				break;

			case RXTX_ESCAPE:
				sendByte(txbuf[i]);
				i++;
				txState = RXTX_NORMAL;
				break;
		}
	}

	//Now do EOF
	sendByte (SOF_EOF);
}

//TODO: Add Meaningful Comment
void buildframe(uint8_t commandDataNumBytes, di_commandKey_t command)
{
	//Check if Request to Send handshaking is enabled
	if (outputEventConfigs[OE_REQUEST_TO_SEND - START_OE] & ECFG_ENABLED_ON_GPIO)
	{
		//Assert RTS
		setGPIO((GPIO_T)((outputEventConfigs[OE_REQUEST_TO_SEND - START_OE] & ECFG_GPIO_BITS)>>2), TRUE);
	}

    uint16_t i = 1;
	crc_ccitt_init();
	txbuf[0] = SOF_EOF;
	txbuf[CMD_INDEX] = command;

    while(i <= MIN_CRCD_FRAME_BYTES + commandDataNumBytes)
    {
    	crc_ccitt_update(txbuf[i]);
    	i++;
    }
	txbuf[i] = crc>>8;
	txbuf[i+1]= crc & 0x00FF;
	txbuf[i+2]= SOF_EOF;
	sendtxbuf(MIN_FRAME_BYTES + commandDataNumBytes);

	//Remove Request to Send if necessary
	if (outputEventConfigs[OE_REQUEST_TO_SEND - START_OE] & ECFG_ENABLED_ON_GPIO)
	{
		//De-assert RTS
		setGPIO((GPIO_T)((outputEventConfigs[OE_REQUEST_TO_SEND - START_OE] & ECFG_GPIO_BITS)>>2), FALSE);
	}
}

typedef enum
{
	COM_UART,
	COM_SPI3,
	COM_I2C,
	COM_SPI4
}comMode_t;
/*------void readCommMode(void)--------------------------------------------------------
     This function sets the communication profile based on the state of GPIO4 and GPIO5
        on port 3.1 and 3.2
            COM CONFIG
            GPIO4 , GPIO5

            00 = UART
            01 = SPI3
            10 = I2C
            11 = SPI4

    Any user application I/0 requirements should be set here as well for GPIO 0-7
------void prefini(void)--------------------------------------------------------*/
void readCommMode()
{
//	static comMode_t COMMode;
	static comMode_t COMMode = COM_UART;	// default to UART
	//Order GPIO4, GPIO5
#ifdef DISP_EAGLE
	COMMode = (comMode_t)(((P2IN & BIT2) << 1) | ((P2IN & BIT6) >> 1));
#else
	COMMode = (comMode_t)(((P2IN & BIT0) << 1) | ((P2IN & BIT1) >> 1));
#endif	//DISP_EAGLE
#ifdef DISP_UNIFIED_CHASSIS
	COMMode = COM_UART;
#endif	//DISP_UNIFIED_CHASSIS

#ifdef DISP_ATHENA_SOAP
	COMMode = COM_UART;
#endif	//DISP_ATHENA_SOAP

#ifdef DISP_AIRCARE
	COMMode = COM_UART;
#endif	//DISP_AIRCARE

	//Normal operation
	switch ( COMMode )
	{
		case COM_UART: //UART
		    initUARTCommMode();
            break;

#ifndef DISP_EAGLE
		case COM_SPI3: //SPI3
			  WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
			  while (!(P1IN & BIT4));                   // If clock sig from mstr stays low,
			                                            // it is not yet in SPI mode

			  P1SEL = BIT1 + BIT2 + BIT4;
			  P1SEL2 = BIT1 + BIT2 + BIT4;
			  UCA0CTL1 = UCSWRST;                       // **Put state machine in reset**
			  UCA0CTL0 |= UCCKPL + UCMSB + UCSYNC;      // 3-pin, 8-bit SPI master
			  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
			  IE2 |= UCA0RXIE;                          // Enable USCI0 RX interrupt
			break;

		case COM_I2C: //I2C
			  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
			  P1SEL |= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
			  P1SEL2|= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
			  UCB0CTL1 |= UCSWRST;                      // Enable SW reset
			  UCB0CTL0 = UCMODE_3 + UCSYNC;             // I2C Slave, synchronous mode
			  UCB0I2COA = 0x48;                         // Own Address is 048h
			  UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
			  IE2 |= UCB0RXIE;                          // Enable RX interrupt
			break;

		case COM_SPI4: //SPI4
			  WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
			  while (!(P1IN & BIT4));                   // If clock sig from mstr stays low,
			                                            // it is not yet in SPI mode

			  P1SEL = BIT1 + BIT2 + BIT4;
			  P1SEL2 = BIT1 + BIT2 + BIT4;
			  UCA0CTL1 = UCSWRST;                       // **Put state machine in reset**
			  UCA0CTL0 |= UCCKPL + UCMSB + UCSYNC;      // 3-pin, 8-bit SPI master
			  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
			  IE2 |= UCA0RXIE;                          // Enable USCI0 RX interrupt
			break;
#endif	//DISP_EAGLE
	}
#ifdef GPIO_EVENTS_ENABLED
	uint8_t i;
	//Now set up GPIO's that have been enabled. This may override the com setup above if
	//GPIO's have been configured on the comm pins that are shared.
	//Start with Input events
	for (i = 0; i < (NUM_OF_INPUT_EVENTS - START_IE); i++)
	{
		if (inputEventConfigs[i] & ECFG_ENABLED_ON_GPIO) // if an input event is currently using this gpio
		{
			ConfigGPIOAsInput((GPIO_T)((inputEventConfigs[i] & ECFG_GPIO_BITS)>>2));
		}
	}
	//Now Output Events
	for (i = 0; i < (NUM_OF_OUTPUT_EVENTS - START_OE); i++)
	{
		if (outputEventConfigs[i] & ECFG_ENABLED_ON_GPIO) // if an output event is currently using this gpio
		{
			ConfigGPIOAsOutput((GPIO_T)((outputEventConfigs[i] & ECFG_GPIO_BITS)>>2));
		}
	}
#endif //GPIO_EVENTS_ENABLED
}
void initUARTCommMode()
{
#ifdef DISP_EAGLE
	P4SEL |= BIT4+BIT5;                        // select uart function for 4.4 and 4.5

	UCA1CTL1 |= UCSWRST;
	UCA1CTL1 |= UCSSEL__SMCLK;
	UCA1BR0 = 0x4A;							// ~ 115942 @ 8MHz
	UCA1BR1 = 0x00;
	UCA1MCTL |= UCBRS_4 + UCBRF_0;
	UCA1CTL1 &= ~UCSWRST;
	UCA1IE |= UCRXIE;
#else
    WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
    UCA0CTL1 |= UCSWRST;                      //Hold USCI in reset while configuring
    UCA0CTL0 = 0x00;                          //8N1, UART Mode
    UCA0CTL1 = UCSSEL_2 + UCSWRST;           // SMCLK

    //Defines the UART Baud Rate based on the defined clock speed of the unit
#if(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_16Mhz)
    //This is an untested divider; I simply did the math and entered the value
    UCA0BR0 = 0x8B;                           // 16MHz 115200
    UCA0BR1 = 0x00;                           // 16MHz 115200
    UCA0MCTL = UCBRS_1;							//Modulation UCBRSx = 1
#endif	//(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_16Mhz)
#if(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_12Mhz)
    UCA0BR0 = 0x68;                           // 12MHz 115200
    UCA0BR1 = 0x00;                           // 12MHz 115200
    UCA0MCTL = UCBRS_1;                       // Modulation UCBRSx = 1
#endif	//(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_12Mhz)
#if(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_8Mhz)
    UCA0BR0 = 0x45;                           // 8MHz 115200
    UCA0BR1 = 0x00;                           // 8MHz 115200
    UCA0MCTL = UCBRS_4;                       // Modulation UCBRSx = 4
#endif	//(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_8Mhz)
#if(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_1Mhz)
    UCA0BR0 = 0x08;                           // 1MHz 115200
    UCA0BR1 = 0x00;                           // 1MHz 115200
    UCA0MCTL = UCBRS_6;							//Modulation UCBRSx = 6
#endif	//(FLASH_CLOCK_DIV == FLASH_CLOCK_DIV_1Mhz)
    UCA0ABCTL = 0x00;                         //disable Auto Baud
    UCA0IRTCTL = 0x00;                        //disable IrDA TX
    UCA0IRRCTL = 0x00;                        //disable IrDA RX
    P1SEL |= BIT1 + BIT2;                      // P1.1 = RXD, P1.2=TXD
    P1SEL2 |= BIT1 + BIT2;

    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt
#endif	//DISP_EAGLE

    //Check if Request to Send handshaking is enabled, and set up output if so
    if (outputEventConfigs[OE_REQUEST_TO_SEND - START_OE] & ECFG_ENABLED_ON_GPIO)
    {
        switch((GPIO_T)((outputEventConfigs[OE_REQUEST_TO_SEND - START_OE] & ECFG_GPIO_BITS)>>2))
            {
                case GPIO0:
                    GPIO0_OUTPUT_DIR;
                    break;
                case GPIO1:
                    GPIO1_OUTPUT_DIR;
                    break;
#ifndef DISP_AIRCARE
				case GPIO2:
					GPIO2_OUTPUT_DIR;
					break;
				case GPIO3:
					GPIO3_OUTPUT_DIR;
					break;
#endif	//DISP_AIRCARE
                case GPIO4:
                    GPIO4_OUTPUT_DIR;
                    break;
                case GPIO5:
                    GPIO5_OUTPUT_DIR;
                    break;
                case GPIO6:
                    GPIO6_OUTPUT_DIR;
                    break;
                case GPIO7:
                    GPIO7_OUTPUT_DIR;
                    break;
            }
    }
}
#ifdef GPIO_EVENTS_ENABLED
//This function will set up a GPIO as an output pin.
static ConfigGPIOAsOutput(GPIO_T gpio)
{
	switch(gpio)
	{
		case GPIO0:
			GPIO0_OUTPUT_DIR;
			break;
		case GPIO1:
			GPIO1_OUTPUT_DIR;
			break;
#ifndef DISP_AIRCARE
		case GPIO2:
			GPIO2_OUTPUT_DIR;
			break;
		case GPIO3:
			GPIO3_OUTPUT_DIR;
			break;
#endif	//DISP_AIRCARE
		case GPIO4:
			GPIO4_OUTPUT_DIR;
			break;
		case GPIO5:
			GPIO5_OUTPUT_DIR;
			break;
		case GPIO6:
			GPIO6_OUTPUT_DIR;
			break;
		case GPIO7:
			GPIO7_OUTPUT_DIR;
			break;
	}
}

//This function will set up a GPIO as an input pin.
static ConfigGPIOAsInput(GPIO_T gpio)
{
	switch(gpio)
	{
		case GPIO0:
			GPIO0_INPUT_DIR;
			break;
		case GPIO1:
			GPIO1_INPUT_DIR;
			break;
		case GPIO2:
			GPIO2_INPUT_DIR;
			break;
		case GPIO3:
			GPIO3_INPUT_DIR;
			break;
		case GPIO4:
			GPIO4_INPUT_DIR;
			break;
		case GPIO5:
			GPIO5_INPUT_DIR;
			break;
		case GPIO6:
			GPIO6_INPUT_DIR;
			break;
		case GPIO7:
			GPIO7_INPUT_DIR;
			break;
	}
}
#endif //GPIO_EVENTS_ENABLED

//UART receive isr
#ifdef DISP_EAGLE
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    rxbuf[rxindex] = UCA1RXBUF;
    rxindex++;
    if (rxindex >= sizeof(rxbuf))
    {
        rxindex = 0;
    }
}
#else
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    rxbuf[rxindex] = UCA0RXBUF;
    rxindex++;
    if (rxindex >= sizeof(rxbuf))
    {
        rxindex = 0;
    }
}
#endif	//DISP_EAGLE



