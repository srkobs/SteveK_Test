/*
 * di_events.h
 *
 *  Created on: May 28, 2013
 *      Author: Dave Murphy
 *
 * © 2013. Confidential and proprietary information of Georgia-Pacific Consumer Products LP. All rights reserved.
 *
 */

#ifndef DI_EVENTS_H_
#define DI_EVENTS_H_

#define NUM_OF_GPIOS 8

#define MAX_PARAM 0xFF

//Event Config Bit Defines
#define ECFG_HIGH_PRIORITY 0x80
#define ECFG_ENABLED_ON_GPIO 0x40
#define ECFG_ENABLED_ON_SERIAL 0x20
#define ECFG_GPIO_BITS 0x1C
#define ECFG_GPIO_TYPE_BITS 0x03

//GPIO port defines
//GPIO0: P1.6
#define GPIO0_IN (P1IN & BIT6)
#define GPIO0_SET (P1OUT |= BIT6)
#define GPIO0_CLR (P1OUT &= ~BIT6)
#define GPIO0_OUTPUT_DIR (P1DIR |= BIT6)
#define GPIO0_INPUT_DIR (P1DIR &= ~BIT6)
//GPIO1: P1.7
#define GPIO1_IN (P1IN & BIT7)
#define GPIO1_SET (P1OUT |= BIT7)
#define GPIO1_CLR (P1OUT &= ~BIT7)
#define GPIO1_OUTPUT_DIR (P1DIR |= BIT7)
#define GPIO1_INPUT_DIR (P1DIR &= ~BIT7)
//GPIO2: P1.5
#define GPIO2_IN (P1IN & BIT5)
#define GPIO2_SET (P1OUT |= BIT5)
#define GPIO2_CLR (P1OUT &= ~BIT5)
#define GPIO2_OUTPUT_DIR (P1DIR |= BIT5)
#define GPIO2_INPUT_DIR (P1DIR &= ~BIT5)
//GPIO3: P1.4
#define GPIO3_IN (P1IN & BIT4)
#define GPIO3_SET (P1OUT |= BIT4)
#define GPIO3_CLR (P1OUT &= ~BIT4)
#define GPIO3_OUTPUT_DIR (P1DIR |= BIT4)
#define GPIO3_INPUT_DIR (P1DIR &= ~BIT4)
//GPIO4: P2.0
#define GPIO4_IN (P2IN & BIT0)
#define GPIO4_SET (P2OUT |= BIT0)
#define GPIO4_CLR (P2OUT &= ~BIT0)
#define GPIO4_OUTPUT_DIR (P2DIR |= BIT0)
#define GPIO4_INPUT_DIR (P2DIR &= ~BIT0)
//GPIO5: P2.1
#define GPIO5_IN (P2IN & BIT1)
#define GPIO5_SET (P2OUT |= BIT1)
#define GPIO5_CLR (P2OUT &= ~BIT1)
#define GPIO5_OUTPUT_DIR (P2DIR |= BIT1)
#define GPIO5_INPUT_DIR (P2DIR &= ~BIT1)
//GPIO6: P1.1
#define GPIO6_IN (P1IN & BIT1)
#define GPIO6_SET (P1OUT |= BIT1)
#define GPIO6_CLR (P1OUT &= ~BIT1)
#define GPIO6_OUTPUT_DIR (P1DIR |= BIT1)
#define GPIO6_INPUT_DIR (P1DIR &= ~BIT1)
//GPIO7: P1.2
#define GPIO7_IN (P1IN & BIT2)
#define GPIO7_SET (P1OUT |= BIT2)
#define GPIO7_CLR (P1OUT &= ~BIT2)
#define GPIO7_OUTPUT_DIR (P1DIR |= BIT2)
#define GPIO7_INPUT_DIR (P1DIR &= ~BIT2)

typedef enum
{
	GPIO0,
	GPIO1,
	GPIO2,
	GPIO3,
	GPIO4,
	GPIO5,
	GPIO6,
	GPIO7
}GPIO_T;

extern uint8_t gpioModes; //1 for input, 0 for output
extern uint8_t gpioOutputs;
extern uint8_t gpioLevels; //the levels at which the gpios are active
extern uint8_t inputEventData;
extern uint8_t inputEventConfigs[];
extern uint8_t outputEventConfigs[];

void configgpio(void);
void sendEvent(di_paramKey_t evt, uint16_t evtData);
void triggerEvent(di_paramKey_t event);
void configureInputEvent(di_paramKey_t event, uint8_t config);
void configureOutputEvent(di_paramKey_t event, uint8_t config);
void handleEvents();
uint8_t RemoteDispenseTrigger();
uint8_t getGPIO(GPIO_T gpio);
void setGPIO(GPIO_T gpio, uint8_t value);

#endif /* ENMOTION_GPIO_H_ */
