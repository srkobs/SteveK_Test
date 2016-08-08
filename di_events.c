/*
 * di_events.c
 *
 *  Created on: May 28, 2013
 *      Author: Dave Murphy
 *
 * © 2013. Confidential and proprietary information of Georgia-Pacific Consumer Products LP. All rights reserved.
 *
 */

#include <stdint.h>
#include <msp430.h>
#include "di.h"

#if defined(DISP_ENMOTION_SOAP) || defined(DISP_ENMOTION_SOAP_CAP) || defined (DISP_SAFEHAVEN_SOAP)
#include "../Bsp.h"
#include "../exception.h"
#endif //DISP_ENMOTION_SOAP

#ifdef DISP_ENMOTION_CLASSIC
#include "../Dispense.h"
#include "../exception.h"

extern DISPENSE_STATE_T currentDispenseState;
#endif //DISP_ENMOTION_CLASSIC

#ifdef DISP_UNIFIED_CHASSIS
#include "../unifiedProc.h"
#include "../io.h"
#include "../dispenser_defines.h"
#include "../led.h"
extern volatile SYSTEM_STATES systemState;
extern bool_t disableDispense;
extern bool_t indicateDispenser;
extern ERROR_STATES errorState;
extern BATTERY_STATE batteryState;
extern volatile bool_t lowPaper;
#endif //DISP_UNIFIED_CHASSIS

#ifdef DISP_AIRCARE
#include "../airProc.h"
#include "../io.h"
extern bool_t disableDispense;
extern bool_t indicateDispenser;
#endif	//DISP_AIRCARE

#ifdef DISP_ATHENA_SOAP
#include "../athenaProc.h"
#include "../io.h"
extern SYSTEM_STATES systemState;
#endif	//DISP_ATHENA_SOAP

extern di_Status_t di_status;

#ifndef DISP_ENMOTION_SOAP_CAP
static uint8_t triggerDispense = FALSE;
#endif //DISP_ENMOTION_SOAP_CAP

//TBDstatic void pulse(uint8_t gpio);
#ifndef DISP_ENMOTION_SOAP_CAP
static void performDispense(void);
static void enterFaultMode(void );
static void indicateOutofProduct(void );
static void externalHeartbeat(void );
static void clearLatchedOutputs(void );
static void externalLowBattery(void );
#endif //DISP_ENMOTION_SOAP_CAP
static void setDispenserState();
#ifdef DISP_ENMOTION_CLASSIC
static void performTearBump(void );
static void performManualFeed(void );
#endif //DISP_ENMOTION_CLASSIC
#ifdef DISP_SAFEHAVEN_SOAP
static void processBadgeDetect(void);
static void enterDemoMode(void);
#endif //DISP_SAFEHAVEN_SOAP
static uint8_t getgpio(void);
#ifdef DISP_UNIFIED_CHASSIS
static void motorTest(void);
static void expansion1Test(void);
static void expansion2Test(void);
static void i2cTest(void);
#endif	//DISP_UNIFIED_CHASSIS

//allows the creation, reading, and writing of a bit array
//assumes uint16_t size of 16
/*
#define DEFINE_BIT_ARRAY(name, x) uint16_t name[((x) >> 4) + 1]
#define GET_BIT(name, x)          (name[(x) >> 4] & (1 << ((x) & 0xF)))
#define SET_BIT(name, x)          (name[(x) >> 4] |= (1 << ((x) & 0xF)))
#define CLEAR_BIT(name, x)        (name[(x) >> 4] &= ~(1 << ((x) & 0xF)))
*/

#define SAFELY_CALL(fn)           if (fn!= NULL) fn()

//GPIO mode enumeration
enum
{
	LEVEL_HIGH = 0,
	PULSED,
	LEVEL_LOW,
	LATCHED
};
static void (*gpioIntputEventFn[NUM_OF_GPIOS])(void)  = {0,0,0,0,0,0,0,0}; //fn pointers to execute for a gpio input event
//static void (*gpioOutputEventFn[NUM_OF_GPIOS])(void); //fn pointers to to cause a gpio to fire
//static void (*outputEventFn[NUM_OF_OUTPUT_EVENTS - START_OE])(void); //fn pointers to execute for output events

uint8_t gpioModes = 0xFF; //1 for input, 0 for output

#ifdef DISP_ENMOTION_SOAP
#define INPUT_EVENT_TABLE_DEFINED
uint8_t inputEventConfigs[NUM_OF_INPUT_EVENTS - START_IE] =
{
	ECFG_ENABLED_ON_SERIAL,					//IE_PERFORM_DISPENSE = START_IE,
	ECFG_ENABLED_ON_SERIAL,					//IE_ENTER_FAULT_MODE,
	ECFG_ENABLED_ON_SERIAL,					//IE_OUT_OF_PRODUCT,
	ECFG_ENABLED_ON_SERIAL,					//IE_EXTERNAL_HEARTBEAT_EVENT,
	ECFG_ENABLED_ON_SERIAL,					//IE_CLEAR_LATCHED_OUTPUTS,
	ECFG_ENABLED_ON_SERIAL,					//IE_EXTERNAL_LOW_BATTERY,
	0,										//IE_SET_DISPENSER_STATE,
	ECFG_ENABLED_ON_GPIO + (GPIO5 << 2),	//IE_CLEAR_TO_SEND
	0,										//IE_TEAR_BUMP,
	0,										//IE_MANUAL_FEED
	0,										//IE_MOTOR_TEST
	0,										//IE_EXPANSION_1_TEST
	0,										//IE_EXPANSION_2_TEST
	0										//IE_I2C_TEST
};
#endif	//DISP_ENMOTION_SOAP

#ifdef DISP_ENMOTION_CLASSIC
#define INPUT_EVENT_TABLE_DEFINED
uint8_t inputEventConfigs[NUM_OF_INPUT_EVENTS - START_IE] =
{
	ECFG_ENABLED_ON_SERIAL,					//IE_PERFORM_DISPENSE = START_IE,
	ECFG_ENABLED_ON_SERIAL,					//IE_ENTER_FAULT_MODE,
	ECFG_ENABLED_ON_SERIAL,					//IE_OUT_OF_PRODUCT,
	ECFG_ENABLED_ON_SERIAL,					//IE_EXTERNAL_HEARTBEAT_EVENT,
	ECFG_ENABLED_ON_SERIAL,					//IE_CLEAR_LATCHED_OUTPUTS,
	ECFG_ENABLED_ON_SERIAL,					//IE_EXTERNAL_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL,					//IE_SET_DISPENSER_STATE,
	ECFG_ENABLED_ON_GPIO + (GPIO5 << 2),	//IE_CLEAR_TO_SEND
	ECFG_ENABLED_ON_SERIAL,					//IE_TEAR_BUMP,
	ECFG_ENABLED_ON_SERIAL,					//IE_MANUAL_FEED,
	0,										//IE_MOTOR_TEST
	0,										//IE_EXPANSION_1_TEST
	0,										//IE_EXPANSION_2_TEST
	0										//IE_I2C_TEST
};
#endif	//DISP_ENMOTION_CLASSIC
#ifdef DISP_UNIFIED_CHASSIS
#define INPUT_EVENT_TABLE_DEFINED
uint8_t inputEventConfigs[NUM_OF_INPUT_EVENTS - START_IE] =
{
	ECFG_ENABLED_ON_SERIAL,					//IE_PERFORM_DISPENSE = START_IE,
	ECFG_ENABLED_ON_SERIAL,					//IE_ENTER_FAULT_MODE,
	ECFG_ENABLED_ON_SERIAL,					//IE_OUT_OF_PRODUCT,
	ECFG_ENABLED_ON_SERIAL,					//IE_EXTERNAL_HEARTBEAT_EVENT,
	0,										//IE_CLEAR_LATCHED_OUTPUTS,
	ECFG_ENABLED_ON_SERIAL,					//IE_EXTERNAL_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL,					//IE_SET_DISPENSER_STATE,
	ECFG_ENABLED_ON_GPIO + (GPIO5 << 2),	//IE_CLEAR_TO_SEND
	0,										//IE_TEAR_BUMP,
	0,										//IE_MANUAL_FEED,
	ECFG_ENABLED_ON_SERIAL,					//IE_MOTOR_TEST
	ECFG_ENABLED_ON_SERIAL,					//IE_EXPANSION_1_TEST
	ECFG_ENABLED_ON_SERIAL,					//IE_EXPANSION_2_TEST
	ECFG_ENABLED_ON_SERIAL					//IE_I2C_TEST
};
#endif	//DISP_UNIFIED_CHASSIS
#ifdef DISP_ATHENA_SOAP
#define INPUT_EVENT_TABLE_DEFINED
uint8_t inputEventConfigs[NUM_OF_INPUT_EVENTS - START_IE] =
{
	ECFG_ENABLED_ON_SERIAL,					//IE_PERFORM_DISPENSE = START_IE,
	ECFG_ENABLED_ON_SERIAL,					//IE_ENTER_FAULT_MODE,
	ECFG_ENABLED_ON_SERIAL,					//IE_OUT_OF_PRODUCT,
	ECFG_ENABLED_ON_SERIAL,					//IE_EXTERNAL_HEARTBEAT_EVENT,
	0,										//IE_CLEAR_LATCHED_OUTPUTS,
	ECFG_ENABLED_ON_SERIAL,					//IE_EXTERNAL_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL,					//IE_SET_DISPENSER_STATE,
	ECFG_ENABLED_ON_GPIO + (GPIO5 << 2),	//IE_CLEAR_TO_SEND
	0,										//IE_TEAR_BUMP,
	0,										//IE_MANUAL_FEED,
	0,										//IE_MOTOR_TEST
	0,										//IE_EXPANSION_1_TEST
	0,										//IE_EXPANSION_2_TEST
	0										//IE_I2C_TEST
};
#endif	//DISP_ATHENA_SOAP
#ifdef DISP_EAGLE
#define INPUT_EVENT_TABLE_DEFINED
uint8_t inputEventConfigs[NUM_OF_INPUT_EVENTS - START_IE] =
{
	ECFG_ENABLED_ON_SERIAL,					//IE_PERFORM_DISPENSE = START_IE,
	ECFG_ENABLED_ON_SERIAL,					//IE_ENTER_FAULT_MODE,
	0,										//IE_OUT_OF_PRODUCT,
	ECFG_ENABLED_ON_SERIAL,					//IE_EXTERNAL_HEARTBEAT_EVENT,
	0,										//IE_CLEAR_LATCHED_OUTPUTS,
	0,										//IE_EXTERNAL_LOW_BATTERY,
	0,										//IE_SET_DISPENSER_STATE,
	ECFG_ENABLED_ON_GPIO + (GPIO5 << 2),	//IE_CLEAR_TO_SEND
	0,										//IE_TEAR_BUMP,
	0,										//IE_MANUAL_FEED,
	0,										//IE_MOTOR_TEST
	0,										//IE_EXPANSION_1_TEST
	0,										//IE_EXPANSION_2_TEST
	0										//IE_I2C_TEST
};
#endif	//DISP_EAGLE
#ifdef DISP_TISSUE_FUEL
#define INPUT_EVENT_TABLE_DEFINED
uint8_t inputEventConfigs[NUM_OF_INPUT_EVENTS - START_IE] =
{
	0,										//IE_PERFORM_DISPENSE = START_IE,
	0,										//IE_ENTER_FAULT_MODE,
	0,										//IE_OUT_OF_PRODUCT,
	0,										//IE_EXTERNAL_HEARTBEAT_EVENT,
	0,										//IE_CLEAR_LATCHED_OUTPUTS,
	0,										//IE_EXTERNAL_LOW_BATTERY,
	0,										//IE_SET_DISPENSER_STATE,
	ECFG_ENABLED_ON_GPIO + (GPIO5 << 2),	//IE_CLEAR_TO_SEND
	0,										//IE_TEAR_BUMP,
	0,										//IE_MANUAL_FEED,
	0,										//IE_MOTOR_TEST
	0,										//IE_EXPANSION_1_TEST
	0,										//IE_EXPANSION_2_TEST
	0										//IE_I2C_TEST
};
#endif	//DISP_TISSUE_FUEL

#ifdef DISP_AIRCARE
#define INPUT_EVENT_TABLE_DEFINED
uint8_t inputEventConfigs[NUM_OF_INPUT_EVENTS - START_IE] =
{
	ECFG_ENABLED_ON_SERIAL,					//IE_PERFORM_DISPENSE = START_IE,
	0,										//IE_ENTER_FAULT_MODE,
	ECFG_ENABLED_ON_SERIAL,					//IE_OUT_OF_PRODUCT,
	ECFG_ENABLED_ON_SERIAL,					//IE_EXTERNAL_HEARTBEAT_EVENT,
	0,										//IE_CLEAR_LATCHED_OUTPUTS,
	0,										//IE_EXTERNAL_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL,					//IE_SET_DISPENSER_STATE,
	ECFG_ENABLED_ON_GPIO + (GPIO5 << 2),	//IE_CLEAR_TO_SEND
	0,										//IE_TEAR_BUMP,
	0,										//IE_MANUAL_FEED,
	0,										//IE_MOTOR_TEST
	0,										//IE_EXPANSION_1_TEST
	0,										//IE_EXPANSION_2_TEST
	0										//IE_I2C_TEST
};
#endif	//DISP_AIRCARE
#ifdef DISP_COMPACT_VERTICAL_SLED
#define INPUT_EVENT_TABLE_DEFINED
uint8_t inputEventConfigs[NUM_OF_INPUT_EVENTS - START_IE] =
{
	0,										//IE_PERFORM_DISPENSE = START_IE,
	ECFG_ENABLED_ON_SERIAL,					//IE_ENTER_FAULT_MODE,
	0,										//IE_OUT_OF_PRODUCT,
	0,										//IE_EXTERNAL_HEARTBEAT_EVENT,
	0,										//IE_CLEAR_LATCHED_OUTPUTS,
	0,										//IE_EXTERNAL_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL,					//IE_SET_DISPENSER_STATE,
	ECFG_ENABLED_ON_GPIO + (GPIO5 << 2),	//IE_CLEAR_TO_SEND
	0,										//IE_TEAR_BUMP,
	0,										//IE_MANUAL_FEED
	0,										//IE_MOTOR_TEST
	0,										//IE_EXPANSION_1_TEST
	0,										//IE_EXPANSION_2_TEST
	0										//IE_I2C_TEST
};
#endif	//DISP_COMPACT_VERTICAL_SLED

#ifdef DISP_ENMOTION_SOAP_CAP
#define INPUT_EVENT_TABLE_DEFINED
uint8_t inputEventConfigs[NUM_OF_INPUT_EVENTS - START_IE] =
{
//	ECFG_ENABLED_ON_SERIAL,					//IE_PERFORM_DISPENSE = START_IE,
//	ECFG_ENABLED_ON_SERIAL,					//IE_ENTER_FAULT_MODE,
//	0,										//IE_OUT_OF_PRODUCT,
//	0,										//IE_EXTERNAL_HEARTBEAT_EVENT,
//	0,										//IE_CLEAR_LATCHED_OUTPUTS,
//	0,										//IE_EXTERNAL_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL,					//IE_SET_DISPENSER_STATE,
	ECFG_ENABLED_ON_GPIO + (GPIO5 << 2),	//IE_CLEAR_TO_SEND
//	0,										//IE_TEAR_BUMP,
//	0,										//IE_MANUAL_FEED,
};
#endif //DISP_ENMOTION_SOAP_CAP

#ifndef INPUT_EVENT_TABLE_DEFINED
#error Need to define the input event array.
#endif	//INPUT_EVENT_TABLE_DEFINED


#ifdef DISP_ENMOTION_SOAP
#define OUTPUT_EVENT_TABLE_DEFINED
uint8_t outputEventConfigs[NUM_OF_OUTPUT_EVENTS - START_OE] =
{
	ECFG_ENABLED_ON_SERIAL,						//	OE_DISPENSE = START_OE,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_FAULT,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//  OE_PRODUCT_LOW
	0,											//	OE_USER_SENSOR_STATE_CHANGE,
	0,											//	OE_MOTOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_DOOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,                  	//  OE_POWERUP,
	0,                  						//  OE_ENABLED_EVENT_OCCURRED,
	ECFG_ENABLED_ON_GPIO + (GPIO4 << 2),		//	OE_REQUEST_TO_SEND
	0,											//	OE_MOTOR_STATE_CHANGE
	0,											//	OE_TEAR_SWITCH_STATE_CHANGE,
	0,											//	OE_TEAR_BUMP_OCCURRED,
	0,											//	OE_DELAY_SWITCH_STATE_CHANGE,
	0,											//	OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,
	0,											//	OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,
	0,											//	OE_DISPENSE_MODE_SWITCH_STATE,
	0,                     						//  OE_MANUAL_FEED
	0,											//  OE_DISPENSE_TIME_UPDATE
	0,											//  OE_DISPENSE_CLICK_UPDATE
	ECFG_ENABLED_ON_SERIAL,						//	OE_MAINT_SWITCH_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_BATTERY_FUEL_GAUGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_PRODUCT_FUEL_GAUGE
	0,											//	OE_ROLL_STUBBABLE
	0,											//	OE_I2C_RESET
	0,											//	OE_FUEL_LEVEL
	0,											//	OE_FUEL_CLICKS
	0,											//	OE_LEFT_NUMBER_STATE_CHANGE
	0,											//	OE_MIDDLE_NUMBER_STATE_CHANGE
	0,											//	OE_RIGHT_NUMBER_STATE_CHANGE
	0,											//	OE_HANG_NUMBER_STATE_CHANGE
	0											//	OE_BUTTON_PRESS
};
#endif	//DISP_ENMOTION_SOAP
#ifdef DISP_ENMOTION_CLASSIC
#define OUTPUT_EVENT_TABLE_DEFINED
uint8_t outputEventConfigs[NUM_OF_OUTPUT_EVENTS - START_OE] =
{
	ECFG_ENABLED_ON_SERIAL,						//	OE_DISPENSE = START_OE,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_FAULT,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//  OE_PRODUCT_LOW
	0,											//	OE_USER_SENSOR_STATE_CHANGE,
	0,											//	OE_MOTOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_DOOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,                  	//  OE_POWERUP,
	0,                  						//  OE_ENABLED_EVENT_OCCURRED,
	ECFG_ENABLED_ON_GPIO + (GPIO4 << 2),		//	OE_REQUEST_TO_SEND
	0,											//	OE_MOTOR_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL,						//	OE_TEAR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_TEAR_BUMP_OCCURRED,
	ECFG_ENABLED_ON_SERIAL,						//	OE_DELAY_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_DISPENSE_MODE_SWITCH_STATE,
	ECFG_ENABLED_ON_SERIAL,                     //  OE_MANUAL_FEED
	ECFG_ENABLED_ON_SERIAL,						//  OE_DISPENSE_TIME_UPDATE
	ECFG_ENABLED_ON_SERIAL,						//  OE_DISPENSE_CLICK_UPDATE
	0,											//	OE_MAINT_SWITCH_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_BATTERY_FUEL_GAUGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_PRODUCT_FUEL_GAUGE
	ECFG_ENABLED_ON_SERIAL,						//	OE_ROLL_STUBBABLE
	ECFG_ENABLED_ON_SERIAL,						//	OE_I2C_RESET
	ECFG_ENABLED_ON_SERIAL,						//	OE_FUEL_LEVEL
	ECFG_ENABLED_ON_SERIAL,						//	OE_FUEL_CLICKS
	0,											//	OE_LEFT_NUMBER_STATE_CHANGE
	0,											//	OE_MIDDLE_NUMBER_STATE_CHANGE
	0,											//	OE_RIGHT_NUMBER_STATE_CHANGE
	0,											//	OE_HANG_NUMBER_STATE_CHANGE
	0											//	OE_BUTTON_PRESS
};
#endif	//DISP_ENMOTION_CLASSIC
#ifdef DISP_UNIFIED_CHASSIS
#define OUTPUT_EVENT_TABLE_DEFINED
uint8_t outputEventConfigs[NUM_OF_OUTPUT_EVENTS - START_OE] =
{
	ECFG_ENABLED_ON_SERIAL,						//	OE_DISPENSE = START_OE,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_FAULT,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//  OE_PRODUCT_LOW
	0,											//	OE_USER_SENSOR_STATE_CHANGE,
	0,											//	OE_MOTOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_DOOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,                  	//  OE_POWERUP,
	0,                  						//  OE_ENABLED_EVENT_OCCURRED,
	ECFG_ENABLED_ON_GPIO + (GPIO4 << 2),		//	OE_REQUEST_TO_SEND
	0,											//	OE_MOTOR_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL,						//	OE_TEAR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_TEAR_BUMP_OCCURRED,
	ECFG_ENABLED_ON_SERIAL,						//	OE_DELAY_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_DISPENSE_MODE_SWITCH_STATE,
	ECFG_ENABLED_ON_SERIAL,                     //  OE_MANUAL_FEED
	ECFG_ENABLED_ON_SERIAL,						//  OE_DISPENSE_TIME_UPDATE
	ECFG_ENABLED_ON_SERIAL,						//  OE_DISPENSE_CLICK_UPDATE
	0,											//	OE_MAINT_SWITCH_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_BATTERY_FUEL_GAUGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_PRODUCT_FUEL_GAUGE
	ECFG_ENABLED_ON_SERIAL,						//	OE_ROLL_STUBBABLE
	ECFG_ENABLED_ON_SERIAL,						//	OE_I2C_RESET
	ECFG_ENABLED_ON_SERIAL,						//	OE_FUEL_LEVEL
	ECFG_ENABLED_ON_SERIAL,						//	OE_FUEL_CLICKS
	0,											//	OE_LEFT_NUMBER_STATE_CHANGE
	0,											//	OE_MIDDLE_NUMBER_STATE_CHANGE
	0,											//	OE_RIGHT_NUMBER_STATE_CHANGE
	0,											//	OE_HANG_NUMBER_STATE_CHANGE
	0											//	OE_BUTTON_PRESS
};
#endif	//DISP_UNIFIED_CHASSIS
#ifdef DISP_ATHENA_SOAP
#define OUTPUT_EVENT_TABLE_DEFINED
uint8_t outputEventConfigs[NUM_OF_OUTPUT_EVENTS - START_OE] =
{
	ECFG_ENABLED_ON_SERIAL,						//	OE_DISPENSE = START_OE,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_FAULT,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//  OE_PRODUCT_LOW
	0,											//	OE_USER_SENSOR_STATE_CHANGE,
	0,											//	OE_MOTOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_DOOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,                  	//  OE_POWERUP,
	0,                  						//  OE_ENABLED_EVENT_OCCURRED,
	ECFG_ENABLED_ON_GPIO + (GPIO4 << 2),		//	OE_REQUEST_TO_SEND
	0,											//	OE_MOTOR_STATE_CHANGE
	0,											//	OE_TEAR_SWITCH_STATE_CHANGE,
	0,											//	OE_TEAR_BUMP_OCCURRED,
	0,											//	OE_DELAY_SWITCH_STATE_CHANGE,
	0,											//	OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,
	0,											//	OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,
	0,											//	OE_DISPENSE_MODE_SWITCH_STATE,
	0,              					        //  OE_MANUAL_FEED
	0,											//  OE_DISPENSE_TIME_UPDATE
	0,											//  OE_DISPENSE_CLICK_UPDATE
	ECFG_ENABLED_ON_SERIAL,						//	OE_MAINT_SWITCH_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_BATTERY_FUEL_GAUGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_PRODUCT_FUEL_GAUGE
	0,											//	OE_ROLL_STUBBABLE
	0,											//	OE_I2C_RESET
	0,											//	OE_FUEL_LEVEL
	0,											//	OE_FUEL_CLICKS
	0,											//	OE_LEFT_NUMBER_STATE_CHANGE
	0,											//	OE_MIDDLE_NUMBER_STATE_CHANGE
	0,											//	OE_RIGHT_NUMBER_STATE_CHANGE
	0,											//	OE_HANG_NUMBER_STATE_CHANGE
	0											//	OE_BUTTON_PRESS
};
#endif	//DISP_ATHENA_SOAP
#ifdef DISP_EAGLE
#define OUTPUT_EVENT_TABLE_DEFINED
uint8_t outputEventConfigs[NUM_OF_OUTPUT_EVENTS - START_OE] =
{
	ECFG_ENABLED_ON_SERIAL,						//	OE_DISPENSE = START_OE,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_FAULT,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//  OE_PRODUCT_LOW
	0,											//	OE_USER_SENSOR_STATE_CHANGE,
	0,											//	OE_MOTOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_DOOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,                  	//  OE_POWERUP,
	0,                  						//  OE_ENABLED_EVENT_OCCURRED,
	ECFG_ENABLED_ON_GPIO + (GPIO4 << 2),		//	OE_REQUEST_TO_SEND
	0,											//	OE_MOTOR_STATE_CHANGE
	0,											//	OE_TEAR_SWITCH_STATE_CHANGE,
	0,											//	OE_TEAR_BUMP_OCCURRED,
	0,											//	OE_DELAY_SWITCH_STATE_CHANGE,
	0,											//	OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,
	0,											//	OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,
	0,											//	OE_DISPENSE_MODE_SWITCH_STATE,
	ECFG_ENABLED_ON_SERIAL,					    //  OE_MANUAL_FEED
	0,											//	OE_DISPENSE_TIME_UPDATE
	0,											//	OE_DISPENSE_CLICK_UPDATE
	0,											//	OE_MAINT_SWITCH_STATE_CHANGE
	0,                   						//  OE_BATTERY_FUEL_GAUGE
	0,                     						//  OE_PRODUCT_FUEL_GAUGE
	0,											//	OE_ROLL_STUBBABLE
	0,											//	OE_I2C_RESET
	0,											//	OE_FUEL_LEVEL
	0,											//	OE_FUEL_CLICKS
	ECFG_ENABLED_ON_SERIAL,						//	OE_LEFT_NUMBER_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL,						//	OE_MIDDLE_NUMBER_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL,						//	OE_RIGHT_NUMBER_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL,						//	OE_HANG_NUMBER_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL						//	OE_BUTTON_PRESS
};
#endif	//DISP_EAGLE
#ifdef DISP_TISSUE_FUEL
#define OUTPUT_EVENT_TABLE_DEFINED
uint8_t outputEventConfigs[NUM_OF_OUTPUT_EVENTS - START_OE] =
{
	ECFG_ENABLED_ON_SERIAL,						//	OE_DISPENSE = START_OE,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_FAULT,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//  OE_PRODUCT_LOW
	0,											//	OE_USER_SENSOR_STATE_CHANGE,
	0,											//	OE_MOTOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_DOOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,                  	//  OE_POWERUP,
	0,                  						//  OE_ENABLED_EVENT_OCCURRED,
	ECFG_ENABLED_ON_GPIO + (GPIO4 << 2),		//	OE_REQUEST_TO_SEND
	0,											//	OE_MOTOR_STATE_CHANGE
	0,											//	OE_TEAR_SWITCH_STATE_CHANGE,
	0,											//	OE_TEAR_BUMP_OCCURRED,
	0,											//	OE_DELAY_SWITCH_STATE_CHANGE,
	0,											//	OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,
	0,											//	OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,
	0,											//	OE_DISPENSE_MODE_SWITCH_STATE,
	0,                 						    //  OE_MANUAL_FEED
	0,											//	OE_DISPENSE_TIME_UPDATE
	0,											//	OE_DISPENSE_CLICK_UPDATE
	0,											//	OE_MAINT_SWITCH_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_BATTERY_FUEL_GAUGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_PRODUCT_FUEL_GAUGE
	0,											//	OE_ROLL_STUBBABLE
	0,											//	OE_I2C_RESET
	0,											//	OE_FUEL_LEVEL
	0,											//	OE_FUEL_CLICKS
	0,											//	OE_LEFT_NUMBER_STATE_CHANGE
	0,											//	OE_MIDDLE_NUMBER_STATE_CHANGE
	0,											//	OE_RIGHT_NUMBER_STATE_CHANGE
	0,											//	OE_HANG_NUMBER_STATE_CHANGE
	0											//	OE_BUTTON_PRESS
};
#endif	//DISP_TISSUE_FUEL
#ifdef DISP_AIRCARE
#define OUTPUT_EVENT_TABLE_DEFINED
uint8_t outputEventConfigs[NUM_OF_OUTPUT_EVENTS - START_OE] =
{
	ECFG_ENABLED_ON_SERIAL,						//	OE_DISPENSE = START_OE,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_FAULT,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//  OE_PRODUCT_LOW
	0,											//	OE_USER_SENSOR_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_MOTOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_DOOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,                  	//  OE_POWERUP,
	0,                  						//  OE_ENABLED_EVENT_OCCURRED,
	ECFG_ENABLED_ON_GPIO + (GPIO4 << 2),		//	OE_REQUEST_TO_SEND
	0,											//	OE_MOTOR_STATE_CHANGE
	0,											//	OE_TEAR_SWITCH_STATE_CHANGE,
	0,											//	OE_TEAR_BUMP_OCCURRED,
	0,											//	OE_DELAY_SWITCH_STATE_CHANGE,
	0,											//	OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,
	0,											//	OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,
	0,											//	OE_DISPENSE_MODE_SWITCH_STATE,
	0,                 						    //  OE_MANUAL_FEED
	0,											//	OE_DISPENSE_TIME_UPDATE
	0,											//	OE_DISPENSE_CLICK_UPDATE
	0,											//	OE_MAINT_SWITCH_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_BATTERY_FUEL_GAUGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_PRODUCT_FUEL_GAUGE
	0,											//	OE_ROLL_STUBBABLE
	0,											//	OE_I2C_RESET
	0,											//	OE_FUEL_LEVEL
	0,											//	OE_FUEL_CLICKS
	0,											//	OE_LEFT_NUMBER_STATE_CHANGE
	0,											//	OE_MIDDLE_NUMBER_STATE_CHANGE
	0,											//	OE_RIGHT_NUMBER_STATE_CHANGE
	0,											//	OE_HANG_NUMBER_STATE_CHANGE
	0											//	OE_BUTTON_PRESS
};
#endif	//DISP_AIRCARE
#ifdef DISP_COMPACT_VERTICAL_SLED
#define OUTPUT_EVENT_TABLE_DEFINED
uint8_t outputEventConfigs[NUM_OF_OUTPUT_EVENTS - START_OE] =
{
	0,						//	OE_DISPENSE = START_OE,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_FAULT,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//  OE_PRODUCT_LOW
	0,											//	OE_USER_SENSOR_STATE_CHANGE,
	0,											//	OE_MOTOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_DOOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,                  	//  OE_POWERUP,
	0,                  						//  OE_ENABLED_EVENT_OCCURRED,
	ECFG_ENABLED_ON_GPIO + (GPIO4 << 2),		//	OE_REQUEST_TO_SEND
	0,											//	OE_MOTOR_STATE_CHANGE
	0,											//	OE_TEAR_SWITCH_STATE_CHANGE,
	0,											//	OE_TEAR_BUMP_OCCURRED,
	0,											//	OE_DELAY_SWITCH_STATE_CHANGE,
	0,											//	OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,
	0,											//	OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,
	0,											//	OE_DISPENSE_MODE_SWITCH_STATE,
	0,                     						//  OE_MANUAL_FEED
	0,											//  OE_DISPENSE_TIME_UPDATE
	0,											//  OE_DISPENSE_CLICK_UPDATE
	ECFG_ENABLED_ON_SERIAL,						//	OE_MAINT_SWITCH_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_BATTERY_FUEL_GAUGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_PRODUCT_FUEL_GAUGE
	0,											//	OE_ROLL_STUBBABLE
	0,											//	OE_I2C_RESET
	0,											//	OE_FUEL_LEVEL
	0,											//	OE_FUEL_CLICKS
	0,											//	OE_LEFT_NUMBER_STATE_CHANGE
	0,											//	OE_MIDDLE_NUMBER_STATE_CHANGE
	0,											//	OE_RIGHT_NUMBER_STATE_CHANGE
	0,											//	OE_HANG_NUMBER_STATE_CHANGE
	0											//	OE_BUTTON_PRESS
};
#endif	//DISP_COMPACT_VERTICAL_SLED

#ifdef DISP_ENMOTION_SOAP_CAP
#define OUTPUT_EVENT_TABLE_DEFINED
uint8_t outputEventConfigs[NUM_OF_OUTPUT_EVENTS - START_OE] =
{
	ECFG_ENABLED_ON_SERIAL,						//	OE_DISPENSE = START_OE,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_LOW_BATTERY,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_FAULT,
	ECFG_ENABLED_ON_SERIAL + ECFG_HIGH_PRIORITY,//	OE_PRODUCT_LOW,
	0,											//	OE_USER_SENSOR_STATE_CHANGE,
	0,											//	OE_MOTOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_DOOR_SWITCH_STATE_CHANGE,
	ECFG_ENABLED_ON_SERIAL,                  	//  OE_POWERUP,
	ECFG_ENABLED_ON_SERIAL,                  	//  OE_ENABLED_EVENT_OCCURRED,
	ECFG_ENABLED_ON_GPIO + (GPIO4 << 2),		//	OE_REQUEST_TO_SEND
	0,											//	OE_MOTOR_STATE_CHANGE
//	0,											//	OE_TEAR_SWITCH_STATE_CHANGE,
//	0,											//	OE_TEAR_BUMP_OCCURRED,
//	0,											//	OE_DELAY_SWITCH_STATE_CHANGE,
//	0,											//	OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,
//	0,											//	OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,
//	0,											//	OE_DISPENSE_MODE_SWITCH_STATE,
//	0,                     						//  OE_MANUAL_FEED
//	0,                     						//  OE_DISPENSE_TIME_UPDATE,
//	0,                     						//  OE_DISPENSE_CLICK_UPDATE,
	ECFG_ENABLED_ON_SERIAL,						//	OE_MAINT_SWITCH_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_BATTERY_FUEL_GAUGE
	ECFG_ENABLED_ON_SERIAL,                     //  OE_PRODUCT_FUEL_GAUGE
//	0,											//	OE_ROLL_STUBBABLE

//	0,											//	OE_I2C_RESET
	ECFG_ENABLED_ON_SERIAL,						//	OE_FUEL_LEVEL
//	0,											//	OE_FUEL_CLICKS
//	0,											//	OE_NUMBER_CHG
//	0,											//	OE_BUTTON_PRESS

	ECFG_ENABLED_ON_SERIAL,						//	OE_PRODUCT_STATE_CHANGE
	ECFG_ENABLED_ON_SERIAL,						//	OE_PRODUCT_ID
	ECFG_ENABLED_ON_SERIAL,						//	OE_TEMPERATURE
#ifdef ENABLE_SW_FUEL_GAUGE_ALG
	ECFG_ENABLED_ON_SERIAL,						//	OE_FUEL_ALGORITHM
#endif //ENABLE_SW_FUEL_GAUGE_ALG
#ifdef DISP_SAFEHAVEN_SOAP
	ECFG_ENABLED_ON_SERIAL,						//	OE_HEARTBEAT_EVENT
#endif //DISP_SAFEHAVEN_SOAP
};
#endif //DISP_ENMOTION_SOAP_CAP


#ifndef OUTPUT_EVENT_TABLE_DEFINED
#error Need to define output table.
#endif	//OUTPUT_EVENT_TABLE_DEFINED


uint8_t gpioOutputs = 0;
uint8_t latchGpios = 0; //those gpios which are "latched" type inputs and outputs
uint8_t levelGpios = 0; //those gpios which are "level" type inputs and outputs
uint8_t gpioLevels = 0; //the levels at which the gpios are active
uint8_t inputEventData = 0;

enum
{
	UART = 0,
	I2C,
	SPI3,
	SPI4
};

#ifdef DISP_ENMOTION_SOAP
#define INPUT_FUNCTION_TABLE_DEFINED
//These function pointers must be defined in the same order as the
//di_paramKey_t enumeration.
static void (*inputEvtFn[NUM_OF_INPUT_EVENTS - START_IE])(void) = \
{
	performDispense,		//IE_PERFORM_DISPENSE
	enterFaultMode,			//IE_ENTER_FAULT_MODE
	indicateOutofProduct,	//IE_INDICATE_OUT_OF_PRODUCT
	externalHeartbeat,		//IE_EXTERNAL_HEARTBEAT_EVENT
	clearLatchedOutputs,	//IE_CLEAR_LATCHED_OUTPUTS
	externalLowBattery,		//IE_EXTERNAL_LOW_BATTERY
	setDispenserState,		//IE_SET_DISPENSER_STATE
	NULL,					//IE_CLEAR_TO_SEND
	NULL,					//IE_TEAR_BUMP
	NULL,					//IE_MANUAL_FEED
	NULL,					//IE_MOTOR_TEST
	NULL,					//IE_EXPANSION_1_TEST
	NULL,					//IE_EXPANSION_2_TEST
	NULL 					//IE_I2C_TEST
};
#endif	//DISP_ENMOTION_SOAP
#ifdef DISP_ENMOTION_CLASSIC
#define INPUT_FUNCTION_TABLE_DEFINED
//These function pointers must be defined in the same order as the
//di_paramKey_t enumeration.
static void (*inputEvtFn[NUM_OF_INPUT_EVENTS - START_IE])(void) = \
{
	performDispense,		//IE_PERFORM_DISPENSE
	enterFaultMode,			//IE_ENTER_FAULT_MODE
	indicateOutofProduct,	//IE_INDICATE_OUT_OF_PRODUCT
	externalHeartbeat,		//IE_EXTERNAL_HEARTBEAT_EVENT
	clearLatchedOutputs,	//IE_CLEAR_LATCHED_OUTPUTS
	externalLowBattery,		//IE_EXTERNAL_LOW_BATTERY
	setDispenserState,		//IE_SET_DISPENSER_STATE
	NULL,					//IE_CLEAR_TO_SEND
	performTearBump,		//IE_TEAR_BUMP
	performManualFeed,		//IE_MANUAL_FEED
	NULL,					//IE_MOTOR_TEST
	NULL,					//IE_EXPANSION_1_TEST
	NULL,					//IE_EXPANSION_2_TEST
	NULL		 			//IE_I2C_TEST
};
#endif	//DISP_ENMOTION_CLASSIC
#ifdef DISP_UNIFIED_CHASSIS
#define INPUT_FUNCTION_TABLE_DEFINED
//These function pointers must be defined in the same order as the
//di_paramKey_t enumeration.
static void (*inputEvtFn[NUM_OF_INPUT_EVENTS - START_IE])(void) = \
{
	performDispense,		//IE_PERFORM_DISPENSE
	enterFaultMode,			//IE_ENTER_FAULT_MODE
	indicateOutofProduct,	//IE_INDICATE_OUT_OF_PRODUCT
	externalHeartbeat,		//IE_EXTERNAL_HEARTBEAT_EVENT
	clearLatchedOutputs,	//IE_CLEAR_LATCHED_OUTPUTS
	externalLowBattery,		//IE_EXTERNAL_LOW_BATTERY
	setDispenserState,		//IE_SET_DISPENSER_STATE
	NULL,					//IE_CLEAR_TO_SEND
	NULL,					//IE_TEAR_BUMP
	NULL,					//IE_MANUAL_FEED
	motorTest,				//IE_MOTOR_TEST
	expansion1Test,			//IE_EXPANSION_1_TEST
	expansion2Test,			//IE_EXPANSION_2_TEST
	i2cTest 				//IE_I2C_TEST
};
#endif	//DISP_UNIFIED_CHASSIS
#ifdef DISP_ATHENA_SOAP
#define INPUT_FUNCTION_TABLE_DEFINED
//These function pointers must be defined in the same order as the
//di_paramKey_t enumeration.
static void (*inputEvtFn[NUM_OF_INPUT_EVENTS - START_IE])(void) = \
{
	performDispense,		//IE_PERFORM_DISPENSE
	enterFaultMode,			//IE_ENTER_FAULT_MODE
	indicateOutofProduct,	//IE_INDICATE_OUT_OF_PRODUCT
	externalHeartbeat,		//IE_EXTERNAL_HEARTBEAT_EVENT
	clearLatchedOutputs,	//IE_CLEAR_LATCHED_OUTPUTS
	externalLowBattery,		//IE_EXTERNAL_LOW_BATTERY
	setDispenserState,		//IE_SET_DISPENSER_STATE
	NULL,					//IE_CLEAR_TO_SEND
	NULL,					//IE_TEAR_BUMP
	NULL,					//IE_MANUAL_FEED
	NULL,					//IE_MOTOR_TEST
	NULL,					//IE_EXPANSION_1_TEST
	NULL,					//IE_EXPANSION_2_TEST
	NULL 					//IE_I2C_TEST
};
#endif	//DISP_ATHENA_SOAP
#ifdef DISP_EAGLE
#define INPUT_FUNCTION_TABLE_DEFINED
//These function pointers must be defined in the same order as the
//di_paramKey_t enumeration.
static void (*inputEvtFn[NUM_OF_INPUT_EVENTS - START_IE])(void) = \
{
	performDispense,		//IE_PERFORM_DISPENSE
	enterFaultMode,			//IE_ENTER_FAULT_MODE
	indicateOutofProduct,	//IE_INDICATE_OUT_OF_PRODUCT
	externalHeartbeat,		//IE_EXTERNAL_HEARTBEAT_EVENT
	clearLatchedOutputs,	//IE_CLEAR_LATCHED_OUTPUTS
	externalLowBattery,		//IE_EXTERNAL_LOW_BATTERY
	setDispenserState,		//IE_SET_DISPENSER_STATE
	NULL,					//IE_CLEAR_TO_SEND
	NULL,					//IE_TEAR_BUMP
	NULL,					//IE_MANUAL_FEED
	NULL,					//IE_MOTOR_TEST
	NULL,					//IE_EXPANSION_1_TEST
	NULL,					//IE_EXPANSION_2_TEST
	NULL 					//IE_I2C_TEST
};
#endif	//DISP_EAGLE
#ifdef DISP_TISSUE_FUEL
#define INPUT_FUNCTION_TABLE_DEFINED
//These function pointers must be defined in the same order as the
//di_paramKey_t enumeration.
static void (*inputEvtFn[NUM_OF_INPUT_EVENTS - START_IE])(void) = \
{
	performDispense,		//IE_PERFORM_DISPENSE
	enterFaultMode,			//IE_ENTER_FAULT_MODE
	indicateOutofProduct,	//IE_INDICATE_OUT_OF_PRODUCT
	externalHeartbeat,		//IE_EXTERNAL_HEARTBEAT_EVENT
	clearLatchedOutputs,	//IE_CLEAR_LATCHED_OUTPUTS
	externalLowBattery,		//IE_EXTERNAL_LOW_BATTERY
	setDispenserState,		//IE_SET_DISPENSER_STATE
	NULL,					//IE_CLEAR_TO_SEND
	NULL,					//IE_TEAR_BUMP
	NULL,					//IE_MANUAL_FEED
	NULL,					//IE_MOTOR_TEST
	NULL,					//IE_EXPANSION_1_TEST
	NULL,					//IE_EXPANSION_2_TEST
	NULL 					//IE_I2C_TEST
};
#endif	//DISP_TISSUE_FUEL
#ifdef DISP_AIRCARE
#define INPUT_FUNCTION_TABLE_DEFINED
//These function pointers must be defined in the same order as the
//di_paramKey_t enumeration.
static void (*inputEvtFn[NUM_OF_INPUT_EVENTS - START_IE])(void) = \
{
	NULL,					//IE_PERFORM_DISPENSE
	NULL,					//IE_ENTER_FAULT_MODE
	NULL,					//IE_INDICATE_OUT_OF_PRODUCT
	NULL,					//IE_EXTERNAL_HEARTBEAT_EVENT
	NULL,					//IE_CLEAR_LATCHED_OUTPUTS
	NULL,					//IE_EXTERNAL_LOW_BATTERY
	setDispenserState,		//IE_SET_DISPENSER_STATE
	NULL,					//IE_CLEAR_TO_SEND
	NULL,					//IE_TEAR_BUMP
	NULL,					//IE_MANUAL_FEED
	NULL,					//IE_MOTOR_TEST
	NULL,					//IE_EXPANSION_1_TEST
	NULL,					//IE_EXPANSION_2_TEST
	NULL 					//IE_I2C_TEST
};
#endif	//DISP_AIRCARE
#ifdef DISP_COMPACT_VERTICAL_SLED
#define INPUT_FUNCTION_TABLE_DEFINED
//These function pointers must be defined in the same order as the
//di_paramKey_t enumeration.
static void (*inputEvtFn[NUM_OF_INPUT_EVENTS - START_IE])(void) = \
{
	NULL,					//IE_PERFORM_DISPENSE
	NULL,					//IE_ENTER_FAULT_MODE
	NULL,					//IE_INDICATE_OUT_OF_PRODUCT
	NULL,					//IE_EXTERNAL_HEARTBEAT_EVENT
	NULL,					//IE_CLEAR_LATCHED_OUTPUTS
	NULL,					//IE_EXTERNAL_LOW_BATTERY
	setDispenserState,		//IE_SET_DISPENSER_STATE
	NULL,					//IE_CLEAR_TO_SEND
	NULL,					//IE_TEAR_BUMP
	NULL,					//IE_MANUAL_FEED
	NULL,					//IE_MOTOR_TEST
	NULL,					//IE_EXPANSION_1_TEST
	NULL,					//IE_EXPANSION_2_TEST
	NULL 					//IE_I2C_TEST
};
#endif	//DISP_COMPACT_VERTICAL_SLED

#ifdef DISP_ENMOTION_SOAP_CAP
#define INPUT_FUNCTION_TABLE_DEFINED
//These function pointers must be defined in the same order as the 
//di_paramKey_t enumeration.
static void (*inputEvtFn[NUM_OF_INPUT_EVENTS - START_IE])(void) = \
{
#if 0
	performDispense,		//IE_PERFORM_DISPENSE
	enterFaultMode,			//IE_ENTER_FAULT_MODE
	indicateOutofProduct,	//IE_INDICATE_OUT_OF_PRODUCT
	NULL,					//IE_EXTERNAL_HEARTBEAT_EVENT
	NULL,					//IE_CLEAR_LATCHED_OUTPUTS
	NULL,					//IE_EXTERNAL_LOW_BATTERY
#endif
	setDispenserState,		//IE_SET_DISPENSER_STATE
	NULL,					//IE_CLEAR_TO_SEND
#ifdef DISP_ENMOTION_CLASSIC
	NULL,					//IE_TEAR_BUMP
	NULL					//IE_MANUAL_FEED
#endif	//DISP_ENMOTION_CLASSIC
#ifdef DISP_SAFEHAVEN_SOAP
	processBadgeDetect,		//IE_BADGE_DETECT
	enterDemoMode			//IE_ENTER_DEMO_MODE
#endif
};
#endif //DISP_ENMOTION_SOAP_CAP


#ifndef INPUT_FUNCTION_TABLE_DEFINED
#error Need to define the input function look up table.
#endif	//INPUT_FUNCTION_TABLE_DEFINED


static uint8_t eventDataAll()
{
	return 0xFF;
}

//loading fn ptr array with functions that return a constant so they
//are safe if called inappropriately.
static uint8_t (*gpioInputEventData[NUM_OF_GPIOS])(void)  = \
{
	eventDataAll,
	eventDataAll,
	eventDataAll,
	eventDataAll,
	eventDataAll,
	eventDataAll,
	eventDataAll,
	eventDataAll
};

void handleEvents()
{
	static uint16_t armed = 0;
	static uint16_t lastState = 0;
	uint8_t gpios = getgpio();
	uint16_t fire;
	gpios ^= ~gpioLevels; //flip everything that's "active low" so that active events are 1 and inactive 0
	armed |= ~lastState & gpios; //detect rising edges
	fire = lastState & ~gpios; //detect falling edges
	fire &= armed; //only fire armed pulses

	fire |= (levelGpios & (lastState ^ gpios)); //flag level changes to fire

	fire &= gpioModes; //mask off outputs
	if (fire)
	{
		uint16_t i;
		for (i = 0; i < NUM_OF_GPIOS; i++)
		{
			if ((fire >> i) & 1)
			{
				inputEventData = gpioInputEventData[i](); //load the default event data for this gpio
				SAFELY_CALL(gpioIntputEventFn[i]);
			}
		}
	}
	lastState = gpios;
}


void sendEvent(di_paramKey_t evt, uint16_t evtData)
{
	di_commandKey_t cmd = CM_EVENT_OCCURRED;
	if (outputEventConfigs[evt - START_OE] & ECFG_ENABLED_ON_SERIAL)
	{

		uint8_t len = 1;

		txbuf[CDATA_START] = evt;

#ifdef DISP_ENMOTION_SOAP
#define OE_SWITCH_DEFINED
		switch(evt)
		{
		//Fill in TX buffer with event data for 1 byte events
		case OE_LOW_BATTERY:
		case OE_FAULT:
		case OE_USER_SENSOR_STATE_CHANGE:
		case OE_MOTOR_SWITCH_STATE_CHANGE:
		case OE_DOOR_SWITCH_STATE_CHANGE:
		case OE_BATTERY_FUEL_GAUGE:
		case OE_MAINT_SWITCH_STATE_CHANGE:
			txbuf[CDATA_START+1] = evtData;
			len++;
			break;
		case OE_DISPENSE:
			break;
		default: //No event data
		break;
		}
#endif	//DISP_ENMOTION_SOAP
#ifdef DISP_ENMOTION_CLASSIC
#define OE_SWITCH_DEFINED
		switch(evt)
		{
		//Fill in TX buffer with event data for 1 byte events
		case OE_LOW_BATTERY:
		case OE_FAULT:
		case OE_USER_SENSOR_STATE_CHANGE:
		case OE_MOTOR_SWITCH_STATE_CHANGE:
		case OE_DOOR_SWITCH_STATE_CHANGE:
		case OE_TEAR_SWITCH_STATE_CHANGE:
		case OE_DELAY_SWITCH_STATE_CHANGE:
		case OE_PAPER_LENGTH_SWITCH_STATE_CHANGE:
		case OE_SENSOR_RANGE_SWITCH_STATE_CHANGE:
		case OE_DISPENSE_MODE_SWITCH_STATE:
		case OE_DISPENSE:
		case OE_MANUAL_FEED:
		case OE_BATTERY_FUEL_GAUGE:
		case OE_PRODUCT_FUEL_GAUGE:
		case OE_I2C_RESET:
		case OE_FUEL_LEVEL:
		case OE_FUEL_CLICKS:
		case OE_DISPENSE_CLICK_UPDATE:
			txbuf[CDATA_START+1] = evtData;
			len++;
			break;

		case OE_DISPENSE_TIME_UPDATE:
			txbuf[CDATA_START+1] = evtData >> 8; //MSB first
			txbuf[CDATA_START+2] = evtData; //now LSB
			len+=2;
			break;

		default: //No event data
		break;
		}
#endif	//DISP_ENMOTION_CLASSIC
#ifdef DISP_UNIFIED_CHASSIS
#define OE_SWITCH_DEFINED
		switch(evt)
		{
		//Fill in TX buffer with event data for 1 byte events
		case OE_LOW_BATTERY:
		case OE_FAULT:
		case OE_USER_SENSOR_STATE_CHANGE:
		case OE_MOTOR_SWITCH_STATE_CHANGE:
		case OE_DOOR_SWITCH_STATE_CHANGE:
		case OE_BATTERY_FUEL_GAUGE:
		case OE_TEAR_SWITCH_STATE_CHANGE:
		case OE_DELAY_SWITCH_STATE_CHANGE:
		case OE_PAPER_LENGTH_SWITCH_STATE_CHANGE:
		case OE_SENSOR_RANGE_SWITCH_STATE_CHANGE:
		case OE_DISPENSE_MODE_SWITCH_STATE:
		case OE_DISPENSE:
		case OE_MANUAL_FEED:
		case OE_DISPENSE_CLICK_UPDATE:
		case OE_MAINT_SWITCH_STATE_CHANGE:
		case OE_PRODUCT_FUEL_GAUGE:
		case OE_I2C_RESET:
		case OE_FUEL_LEVEL:
		case OE_FUEL_CLICKS:
		txbuf[CDATA_START+1] = evtData;
		len++;
		break;
		//Fill in TX uffer with event data for 2 byte events
		case OE_DISPENSE_TIME_UPDATE:
			txbuf[CDATA_START+1] = evtData >> 8; //MSB first
			txbuf[CDATA_START+2] = evtData; //now LSB
			len+=2;
			break;
		default: //No event data
			break;
		}
#endif	//DISP_UNIFIED_CHASSIS
#ifdef DISP_ATHENA_SOAP
#define OE_SWITCH_DEFINED
		switch(evt)
		{
		//Fill in TX buffer with event data for 1 byte events
		case OE_LOW_BATTERY:
		case OE_FAULT:
		case OE_USER_SENSOR_STATE_CHANGE:
		case OE_MOTOR_SWITCH_STATE_CHANGE:
		case OE_DOOR_SWITCH_STATE_CHANGE:
		case OE_BATTERY_FUEL_GAUGE:
		case OE_MAINT_SWITCH_STATE_CHANGE:
			txbuf[CDATA_START+1] = evtData;
			len++;
			break;
		case OE_DISPENSE:
			break;
		default: //No event data
		break;
		}
#endif	//DISP_ATHENA_SOAP
#ifdef DISP_EAGLE
#define OE_SWITCH_DEFINED
		switch(evt)
		{
		//Fill in TX buffer with event data for 1 byte events
		case OE_LOW_BATTERY:
		case OE_FAULT:
		case OE_USER_SENSOR_STATE_CHANGE:
		case OE_MOTOR_SWITCH_STATE_CHANGE:
		case OE_DOOR_SWITCH_STATE_CHANGE:
		case OE_BATTERY_FUEL_GAUGE:
		case OE_POWERUP:
		case OE_MANUAL_FEED:
		case OE_LEFT_NUMBER_STATE_CHANGE:
		case OE_MIDDLE_NUMBER_STATE_CHANGE:
		case OE_RIGHT_NUMBER_STATE_CHANGE:
		case OE_HANG_NUMBER_STATE_CHANGE:
		case OE_BUTTON_PRESS:
		case OE_DISPENSE:
			txbuf[CDATA_START+1] = evtData;
			len++;
			break;
		default: //No event data
		break;
		}
#endif	//DISP_EAGLE
#ifdef DISP_TISSUE_FUEL
#define OE_SWITCH_DEFINED
		switch(evt)
		{
		//Fill in TX buffer with event data for 1 byte events
		case OE_LOW_BATTERY:
		case OE_FAULT:
		case OE_USER_SENSOR_STATE_CHANGE:
		case OE_MOTOR_SWITCH_STATE_CHANGE:
		case OE_DOOR_SWITCH_STATE_CHANGE:
		case OE_BATTERY_FUEL_GAUGE:
		case OE_PRODUCT_FUEL_GAUGE:
			txbuf[CDATA_START+1] = evtData;
			len++;
			break;
		default: //No event data
		break;
		}
#endif	//DISP_TISSUE_FUEL

#ifdef DISP_AIRCARE
#define OE_SWITCH_DEFINED
		switch(evt){
		case OE_DISPENSE:
		case OE_LOW_BATTERY:
		case OE_MOTOR_SWITCH_STATE_CHANGE:
			txbuf[CDATA_START+1] = evtData;
			len++;
		}
#endif	//DISP_AIRCARE
#ifdef DISP_COMPACT_VERTICAL_SLED
#define OE_SWITCH_DEFINED
		switch(evt)
		{
		//Fill in TX buffer with event data for 1 byte events
		case OE_LOW_BATTERY:
		case OE_FAULT:
		case OE_USER_SENSOR_STATE_CHANGE:
		case OE_MOTOR_SWITCH_STATE_CHANGE:
		case OE_DOOR_SWITCH_STATE_CHANGE:
		case OE_BATTERY_FUEL_GAUGE:
		case OE_MAINT_SWITCH_STATE_CHANGE:
			txbuf[CDATA_START+1] = evtData;
			len++;
			break;
		case OE_DISPENSE:
			break;
		default: //No event data
		break;
		}
#endif	//DISP_COMPACT_VERTICAL_SLED

#ifdef DISP_ENMOTION_SOAP_CAP
#define OE_SWITCH_DEFINED
		switch(evt)
		{
			//Fill in TX buffer with event data (if needed)
			case OE_POWERUP:
#ifndef ENG_TEST
			case OE_DISPENSE:
#endif //ENG_TEST
			case OE_LOW_BATTERY:
			case OE_FAULT:
			case OE_PRODUCT_LOW:
			case OE_USER_SENSOR_STATE_CHANGE:
			case OE_MOTOR_SWITCH_STATE_CHANGE:
			case OE_DOOR_SWITCH_STATE_CHANGE:
			case OE_BATTERY_FUEL_GAUGE:
			case OE_MAINT_SWITCH_STATE_CHANGE:
			case OE_FUEL_LEVEL:
#ifndef ENG_TEST
			case OE_PRODUCT_FUEL_GAUGE:
#endif //ENG_TEST
			case OE_PRODUCT_STATE_CHANGE:
			case OE_TEMPERATURE:
#ifdef ENABLE_SW_FUEL_GAUGE_ALG
			case OE_FUEL_ALGORITHM:
#endif //ENABLE_SW_FUEL_GAUGE_ALG
				txbuf[CDATA_START+1] = evtData;
				len++;
				break;
			//Fill in TX uffer with event data for 2 byte events
#ifdef ENG_TEST
			case OE_DISPENSE:
			case OE_ENABLED_EVENT_OCCURRED:
			case OE_PRODUCT_FUEL_GAUGE:
#endif //ENG_TEST
			case OE_PRODUCT_ID:
//			case OE_DISPENSE_TIME_UPDATE:
				txbuf[CDATA_START+1] = evtData >> 8; //MSB first
				txbuf[CDATA_START+2] = evtData; //now LSB
				len+=2;
				break;

			default: //No event data
				break;
		}
#endif //DISP_ENMOTION_SOAP_CAP

#ifndef OE_SWITCH_DEFINED
	#error Need to define the output event switch statement.
#endif	//OE_SWITCH_DEFINED

		//Check if this is a high priority event
		if (outputEventConfigs[evt - START_OE] & ECFG_HIGH_PRIORITY)
		{
			cmd = CM_HIGH_PRIORITY_EVENT;
		}

		buildframe(len, cmd);
	}

#ifdef GPIO_EVENTS_ENABLED
	//fn pointer will be null and not execute if
	// there is no GPIO to fire for this event
	if (outputEventConfigs[evt - START_OE] & ECFG_ENABLED_ON_GPIO)
	{

		switch (outputEventConfigs[evt - START_OE] & ECFG_GPIO_TYPE_BITS)
		{
			case LEVEL_HIGH:
				setGPIO((GPIO_T)((outputEventConfigs[evt - START_OE] & ECFG_GPIO_BITS)>>2), evtData);
				break;
			case PULSED:
				//pulsed not supported in this version;

				break;
			case LEVEL_LOW:
				setGPIO((GPIO_T)((outputEventConfigs[evt - START_OE] & ECFG_GPIO_BITS)>>2), !evtData);
				break;
			case LATCHED:
				//latched not supported in this version;
				break;
		}
	}
#endif //GPIO_EVENTS_ENABLED
}

void triggerEvent(di_paramKey_t event)
{
	if ((inputEventConfigs[event - START_IE] & ECFG_ENABLED_ON_SERIAL)
			&& event < NUM_OF_INPUT_EVENTS && event >= START_IE)
		SAFELY_CALL(inputEvtFn[event - START_IE]);
}

void configureInputEvent(di_paramKey_t event, uint8_t config)
{
	static uint16_t zIndexEvt, gpio, i;
	zIndexEvt = event - START_IE; //zero index the event

	//if this event isn't enabled for gpio, just set the config and leave the function
	if (!(config & ECFG_ENABLED_ON_GPIO))
	{
		inputEventConfigs[zIndexEvt] = config;
		return;
	}

	gpio = BIT_SEQUENCE(config, 5, 3); //extract gpio info from config
	for (i = 0; i < (NUM_OF_INPUT_EVENTS - START_IE); i++)
	{
		if (BIT_SEQUENCE(inputEventConfigs[i], 5, 3) == gpio) // if an input event is currently using this gpio
		{
			inputEventConfigs[i] &= ~ECFG_ENABLED_ON_GPIO;
			//levelGpios &= ~(1 << gpio);
			//latchGpios &= ~(1 << gpio);
		}
	}

	for (i = 0; i < (NUM_OF_OUTPUT_EVENTS - START_OE); i++)
	{
		if (BIT_SEQUENCE(outputEventConfigs[i], 5, 3) == gpio) // if an output event is currently using this gpio
		{
			outputEventConfigs[i] &= ~ECFG_ENABLED_ON_GPIO;
			//levelGpios &= ~(1 << gpio);
			//latchGpios &= ~(1 << gpio);
		}
	}

	inputEventConfigs[zIndexEvt] = config;
	gpioModes |= (1 << gpio); //set this gpio as an input
	if (config & 1)
		gpioLevels |= (1 << gpio); //set this gpio to active high
	else
		gpioLevels &= ~(1 << gpio); //set this gpio to active low

	if (config & ECFG_ENABLED_ON_GPIO)
		gpioIntputEventFn[gpio] = inputEvtFn[zIndexEvt]; //set the gpio fn pointer
	else
		gpioIntputEventFn[gpio] = NULL;
	configgpio();
}

void configureOutputEvent(di_paramKey_t event, uint8_t config)
{
	static uint16_t zIndexEvt, gpio, type, i;
	zIndexEvt = event - START_OE; //zero index the event

	//if this event isn't enabled for gpio, just set the config and leave the function
	if (!(config & ECFG_ENABLED_ON_GPIO))
	{
		outputEventConfigs[zIndexEvt] = config;
		return;
	}

	gpio = BIT_SEQUENCE(config, 5, 3); //extract gpio info from config
	type = BIT_SEQUENCE(config, 2, 1); //extract gpio info from config

	for (i = 0; i < (NUM_OF_INPUT_EVENTS - START_IE); i++)
	{
		if (BIT_SEQUENCE(inputEventConfigs[i], 5, 3) == gpio) // if an input event is currently using this gpio
		{
			inputEventConfigs[i] &= ~ECFG_ENABLED_ON_GPIO;
			//levelGpios &= ~(1 << gpio);
			//latchGpios &= ~(1 << gpio);
		}
	}

	for (i = 0; i < (NUM_OF_OUTPUT_EVENTS - START_OE); i++)
	{
		if (BIT_SEQUENCE(outputEventConfigs[i], 5, 3) == gpio) // if an output event is currently using this gpio
		{
			outputEventConfigs[i] &= ~ECFG_ENABLED_ON_GPIO;
			//levelGpios &= ~(1 << gpio);
			//latchGpios &= ~(1 << gpio);
		}
	}
	if (type == LATCHED)
		latchGpios |= 1 << gpio;
	else
		latchGpios &= ~(1 << gpio);

	// change the event config to "gpioConfig" by replacing the gpio
	// segment with the event number and remove serial enabled bit
	outputEventConfigs[zIndexEvt] = config;
	gpioModes &= ~(1 << gpio); //set this gpio as an output
	if (config & 1)
		gpioLevels |= (1 << gpio); //set this gpio to active high
	else
		gpioLevels &= ~(1 << gpio); //set this gpio to active low
	configgpio();
}

/*** Set  ***/
void configgpio(void)
{
	//Port 1.6 GPIO0/SCL/SOMI
		if(gpioModes & BIT0)
			P1DIR 	&= ~BIT6;
		else
			P1DIR 	|= BIT6; // over written by USCI

	//Port 1.7 GPI01/SDA/SIMO
		if(gpioModes & BIT1)
			P1DIR 	&= ~BIT7;
		else
			P1DIR 	|= BIT7; // over written by USCI

	//Port 1.5 GPIO2/SCLK
		if(gpioModes & BIT2)
			P1DIR 	&= ~BIT5;
		else
			P1DIR 	|= BIT5; // over written by USCI

	//Port 1.4 GPIO3/STE
		if(gpioModes & BIT3)
			P1DIR	&= ~BIT4;
		else
			P1DIR	|= BIT4; // over written by USCI

	//Port 2.0 GPIO4
		if(gpioModes & BIT4)
			P2DIR 	&= ~BIT0;
		else
			P2DIR 	|=  BIT0;

	//Port 2.1 GPIO5
		if(gpioModes & BIT5)
			P2DIR 	&= ~BIT1;
		else
			P2DIR 	|=  BIT1;

	//Port 1.1 RXD/GPIO6
		if(gpioModes & BIT6)
			P1DIR 	&= ~BIT1;
		else
			P1DIR 	|= BIT1; // over written by USCI

	//Port 1.2 TXD/GPIO7
		if(gpioModes & BIT7)
			P1DIR 	&= ~BIT2;
		else
			P1DIR 	|= BIT2;  // over written by USCI

}

uint8_t getGPIO(GPIO_T gpio)
{
	uint8_t retVal;
	switch(gpio)
	{
		case GPIO0:
			retVal = GPIO0_IN;
			break;
		case GPIO1:
			retVal = GPIO1_IN;
			break;
#ifndef DISP_AIRCARE
		case GPIO2:
			retVal = GPIO2_IN;
			break;
		case GPIO3:
			retVal = GPIO3_IN;
			break;
#endif	//DISP_AIRCARE
		case GPIO4:
			retVal = GPIO4_IN;
			break;
		case GPIO5:
			retVal = GPIO5_IN;
			break;
		case GPIO6:
			retVal = GPIO6_IN;
			break;
		case GPIO7:
			retVal = GPIO7_IN;
			break;
	}
	return retVal;
}

void setGPIO(GPIO_T gpio, uint8_t value)
{
	switch(gpio)
	{
		case GPIO0:
			(value) ? GPIO0_SET : GPIO0_CLR;
			break;
		case GPIO1:
			(value) ? GPIO1_SET : GPIO1_CLR;
			break;
#ifndef DISP_AIRCARE
		case GPIO2:
			(value) ? GPIO2_SET : GPIO2_CLR;
			break;
		case GPIO3:
			(value) ? GPIO3_SET : GPIO3_CLR;
			break;
#endif	//DISP_AIRCARE
		case GPIO4:
			(value) ? GPIO4_SET : GPIO4_CLR;
			break;
		case GPIO5:
			(value) ? GPIO5_SET : GPIO5_CLR;
			break;
		case GPIO6:
			(value) ? GPIO6_SET : GPIO6_CLR;
			break;
		case GPIO7:
			(value) ? GPIO7_SET : GPIO7_CLR;
			break;
	}
}


//********************************************************************************
// Input Event Functions: The following functions are called when an input
// event occurs. They should be set up appropriately for the particular dispenser
//

#ifndef DISP_ENMOTION_SOAP_CAP
void performDispense()
{
    triggerDispense = TRUE;

#ifdef DISP_UNIFIED_CHASSIS
    if(systemState != COVER_OPEN_STATE){
    	systemState = DISPENSE;
    }
#endif //DISP_UNIFIED_CHASSIS
#ifdef DISP_ENMOTION_CLASSIC
    currentDispenseState = STATE_DISPENSE;
#endif //DISP_ENMOTION_CLASSIC
#ifdef DISP_ATHENA_SOAP
    if(systemState != COVER_OPEN_STATE){
    	systemState = DISPENSE;
    }
#endif	//DISP_ATHENA_SOAP
}
#endif //DISP_ENMOTION_SOAP_CAP

#ifndef DISP_ENMOTION_SOAP_CAP
void enterFaultMode()
{
#ifdef DISP_UNIFIED_CHASSIS
    errorState = FORCE_ERROR;
#endif //DISP_UNIFIED_CHASSIS
}
#endif //DISP_ENMOTION_SOAP_CAP

#ifndef DISP_ENMOTION_SOAP_CAP
void indicateOutofProduct()
{
#ifdef DISP_UNIFIED_CHASSIS
	if(lowPaper){
		lowPaper = FALSE;
	} else {
		lowPaper = TRUE;
	}
#endif //DISP_UNIFIED_CHASSIS
#ifdef DISP_ENMOTION_SOAP
	//TODO: May need to make this persistent by manipulating the fuel gauge
	//TURN_TRI_LED_OFF;
	//TURN_LOW_FUEL_LED_ON;
#endif //DISP_ENMOTION_SOAP
}
#endif //DISP_ENMOTION_SOAP_CAP

#ifdef DISP_ENMOTION_CLASSIC
void performTearBump()
{
    performJog();
}
#endif //DISP_ENMOTION_CLASSIC

#ifndef DISP_ENMOTION_SOAP_CAP
void externalHeartbeat()
{
#ifdef DISP_SAFEHAVEN_SOAP
	//respond with a heartbeat event
	sendEvent(OE_HEARTBEAT_EVENT, 1);
#endif //DISP_SAFEHAVEN_SOAP
#ifdef DISP_UNIFIED_CHASSIS
	unsigned int saveState = P3OUT;
	ALL_LEDS_OFF;
#ifdef FADE_ACTIVE_LED
	fadeActive(1);
	fadeActive(0);
#else
	ACTIVE_LED_ON;
	delayMs(1000);
	ACTIVE_LED_OFF;
	delayMs(1000);
#endif	//FADE_ACTIVE_LED
	P3OUT = saveState;
#endif	//DISP_UNIFIED_CHASSIS
}
#endif //DISP_ENMOTION_SOAP_CAP

void setDispenserState()
{
#if defined(DISP_ENMOTION_SOAP) || defined(DISP_ENMOTION_SOAP_CAP)
	if (inputEventData < NUM_DISP_STATES)
	{
		di_status.dispState = (di_dispenserState_t)inputEventData;
	}
#endif //DISP_ENMOTION_SOAP
#ifdef DISP_ENMOTION_CLASSIC
	if (inputEventData < NUM_DISP_STATES)
		{
			di_status.dispState = (di_dispenserState_t)inputEventData;
		}
#endif //DISP_ENMOTION_CLASSIC
#ifdef DISP_UNIFIED_CHASSIS
	switch(inputEventData){
	case 0x00:	//Set Normal Operation
		disableDispense = FALSE;
		indicateDispenser = FALSE;
		break;

	case 0x01:	//Disable Dispense
		disableDispense = TRUE;
		indicateDispenser = FALSE;
		break;

	case 0x02:	//Indicate the Dispenser
		indicateDispenser = TRUE;
		disableDispense = FALSE;
		break;

	default:	//Default catch-all for other undefined events
		disableDispense = FALSE;
		indicateDispenser = FALSE;
		break;
	}
#endif //DISP_UNIFIED_CHASSIS
#ifdef DISP_AIRCARE
	switch(inputEventData){
	case 0x00:	//Set Normal Operation
		disableDispense = FALSE;
		indicateDispenser = FALSE;
		break;

	case 0x01:	//Disable Dispense
		disableDispense = TRUE;
		indicateDispenser = FALSE;
		break;

	case 0x02:	//Indicate the Dispenser
		indicateDispenser = TRUE;
		disableDispense = FALSE;
		break;

	default:	//Default catch-all for other undefined events
		disableDispense = FALSE;
		indicateDispenser = FALSE;
		break;
	}
#endif	//DISP_AIRCARE
#ifdef DISP_COMPACT_VERTICAL_SLED
	if (inputEventData < NUM_DISP_STATES)
	{
		di_status.dispState = (di_dispenserState_t)inputEventData;
	}
#endif //DISP_COMPACT_VERTICAL_SLED

}

#ifndef DISP_ENMOTION_SOAP_CAP
void externalLowBattery()
{
#ifdef DISP_UNIFIED_CHASSIS
	if(batteryState == BATTERY_SHUTDOWN){
		batteryState = BATTERY_NORMAL;
	} else if(batteryState == BATTERY_LOW){
		batteryState = BATTERY_SHUTDOWN;
	} else {
		batteryState = BATTERY_LOW;
	}
#endif //DISP_UNIFIED_CHASSIS
}
#endif //DISP_ENMOTION_SOAP_CAP

#ifdef DISP_ENMOTION_CLASSIC
void performManualFeed()
{
    static uint8_t on;
    on = inputEventData;
    if(on)
    {
#if defined(DISP_ENMOTION_SOAP) || defined (DISP_SAFEHAVEN_SOAP)
        di_status.flags.sbits.motor = MOTOR_DISPENSING;
        TurnMotorOn();
#endif //DISP_ENMOTION_SOAP
#ifdef DISP_ENMOTION_CLASSIC
        beginDispense();
#endif //DISP_ENMOTION_CLASSIC
    }
    else //off
    {
#if defined(DISP_ENMOTION_SOAP) || defined (DISP_SAFEHAVEN_SOAP)
        di_status.flags.sbits.motor = MOTOR_IDLE;
        TurnMotorOff();
#endif //DISP_ENMOTION_SOAP
#ifdef DISP_ENMOTION_CLASSIC
        endDispense();
#endif //DISP_ENMOTION_CLASSIC
    }
}
#endif //DISP_ENMOTION_CLASSIC

#ifdef DISP_SAFEHAVEN_SOAP
void processBadgeDetect()
{
	//TODO:
    di_status.flags.sbits.badgeDetected = TRUE;
}

void enterDemoMode()
{
	//TODO:
}
#endif //DISP_SAFEHAVEN_SOAP

#ifndef DISP_ENMOTION_SOAP_CAP
void clearLatchedOutputs()
{
    static uint8_t clearLatches, temp;
    clearLatches = inputEventData;
    temp = clearLatches & gpioLevels; //all the active high gpio's that are to be cleared
    gpioOutputs &= ~temp;
    temp = clearLatches & ~gpioLevels; //all the active low gpio's that are to be cleared
    gpioOutputs |= temp;
}
#endif //DISP_ENMOTION_SOAP_CAP



// Read the Dispenser Interface GPIO pins
uint8_t getgpio(void)
{
    uint8_t temp;
    uint8_t GPIO = 0;

    // Mask and shift each GPIO bit

    temp = P1IN;
    GPIO |= (temp & BIT6 ) >> 6; //GPIO 0
    GPIO |= (temp & BIT7 ) >> 6; //GPIO 1
    GPIO |= (temp & BIT5 ) >> 3; //GPIO 2
    GPIO |= (temp & BIT4 ) >> 1; //GPIO 3
    GPIO |= (temp & BIT1 ) << 5; //GPIO 6
    GPIO |= (temp & BIT2 ) << 5; //GPIO 7

    temp = P2IN;
    GPIO |= (temp & BIT0 ) << 4; //GPIO 4
    GPIO |= (temp & BIT1 ) << 4; //GPIO 5

    return GPIO;

}

#ifndef DISP_ENMOTION_SOAP_CAP
uint8_t RemoteDispenseTrigger()
{
	uint8_t retVal = FALSE;
	if (triggerDispense == TRUE)
	{
		triggerDispense = FALSE;
		retVal = TRUE;
	}
	return retVal;
}
#endif //DISP_ENMOTION_SOAP_CAP

