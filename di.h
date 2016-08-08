/*
 * di.h
 *
 *  Created on: Feb 28, 2014
 *      Author: Dave Murphy
 *
 * © 2014. Confidential and proprietary information of Georgia-Pacific Consumer Products LP. All rights reserved.
 */

#ifndef DI_H_
#define DI_H_
#define XSTR(s) STR(s)
#define STR(s) #s

/************************************************************************************/
/**********    Define below for specific dispenser   ********************************/
/************************************************************************************/
//Define the appropriate dispenser, and undefine the rest
//#error //Must define a dispenser type, with a unique number
//#define DISP_ENMOTION_SOAP 4
//#define DISP_ENMOTION_CLASSIC 5
//#define DISP_UNIFIED_CHASSIS 6
//#define DISP_ATHENA_SOAP 7
//#define DISP_EAGLE	8
//#define DISP_TISSUE_FUEL	9
//#define DISP_AIRCARE 10
#define DISP_COMPACT_VERTICAL_SLED 11

//define the below value to the type of dispenser this is.
#define DISP_TYPE DISP_COMPACT_VERTICAL_SLED
#define DISP_TYPE_STR XSTR(DISP_TYPE)
/********************************************************************************************************/
// These three parameters must match, meaning the SWREV must be the first 4 characters of SWREV_STR, and
// the SWDESCRIPTION_STR should match as well. These should be updated every release.
#define SWREV 0x01 //SWREV: 0xMm, M= Major, m = minor
#define BUILDNUM 0x00
#define SWREV_STR "010020160504_1011" //Format: "<SWREV>_YYYYMMDD_HHMM
#define SWDESCRIPTION_STR "Compact Tissue v0.1 build 00"
/********************************************************************************************************/
#define DISP_INT_REV "20160407_1412"
//END NEW STUFF

//These defines are for setting the programming clock on the
//Flash controller. They are divisors of SMCLK and must result
//in a clock between 257kHz and 476kHz. The numbers below place the
//flash clock in the middle of the range for the respective SMCLK
#define FLASH_CLOCK_DIV_16Mhz   44
#define FLASH_CLOCK_DIV_12Mhz   32
#define FLASH_CLOCK_DIV_8Mhz    21
#define FLASH_CLOCK_DIV_1Mhz    3

//Select one of the above based on SMCLK
#define FLASH_CLOCK_DIV FLASH_CLOCK_DIV_12Mhz //Select appropriate value from above
#ifndef FLASH_CLOCK_DIV
#error //must define FLASH_CLOCK_DIV to appropriate value
#endif	//FLASH_CLOCK_DIV

// Define NULL.
#ifndef NULL
#define NULL								0
#endif	//NULL

// Define TRUE and FALSE.
#ifndef TRUE
#define TRUE								1
#endif	//TRUE
#ifndef FALSE
#define FALSE								0
#endif	//FALSE

#define START_OE 0x40
#define START_IE 0x80
#define MAXDATABYTES 32
#define NVMAXDATABYTES 21   //This is smaller than MAXDATABYTES to fit
                            //the nVdata into one segment of info flash
//Parameter Enumeration
typedef enum
{
	PARAM_SOFTWARE_REVISION = 0x00,
	PARAM_DISPINT_REVISION,
	PARAM_DISPENSER_TYPE,
	PARAM_DISPENSER_SERIAL_NUMBER,
	PARAM_DISPENSER_REVISION,
	PARAM_BATTERY_LEVEL,
	PARAM_FAULTSTATUS ,
	PARAM_PRODUCT_REMAINING,
	PARAM_USER_SENSOR_STATE,
	PARAM_MOTOR_SWITCH_STATE,
	PARAM_COVER_SWITCH_STATE,
	PARAM_MOTOR_STATE,
	PARAM_NUM_DISPENSES_SINCE_RESET,
	PARAM_TEAR_SWITCH_STATE,
	PARAM_DELAY_SETTING,
	PARAM_PAPER_LENGTH_SETTING,
	PARAM_SENSOR_RANGE_SETTING,
	PARAM_DISPENSE_MODE_SETTING,
	PARAM_CAM_SWITCH_STATE,
	PARAM_SHEET_LENGTH_MODE,
	PARAM_NEW_USER_TIME,
	PARAM_NUM_USERS_ADJUST,
	PARAM_MAINT_SWITCH_STATE,
	PARAM_LOW_PRODUCT_THRESHOLD,
	PARAM_FUEL_GAUGE_CAL,
	PARAM_FUEL_GAUGE_SW_REV,
	PARAM_LEFT_SETTING,
	PARAM_MIDDLE_SETTING,
	PARAM_RIGHT_SETTING,
	PARAM_HANG_SETTING,
	NUM_OF_PARAMS_M1,    //Added this to support the BSL key parameter, which is defined below this
	NUM_OF_PARAMS,
	PARAM_DISPENSER_BSL_KEY = 0x3F, //This parameter is fixed at param code 0x3F

	//output event configurations tell the firmware how
	//to notify the module of events
	OE_DISPENSE = START_OE,
	OE_LOW_BATTERY,
	OE_FAULT,
	OE_PRODUCT_LOW,
	OE_USER_SENSOR_STATE_CHANGE,
	OE_MOTOR_SWITCH_STATE_CHANGE,
	OE_DOOR_SWITCH_STATE_CHANGE,
	OE_POWERUP,
	OE_ENABLED_EVENT_OCCURRED,
	OE_REQUEST_TO_SEND,
	OE_MOTOR_STATE_CHANGE,
	OE_TEAR_SWITCH_STATE_CHANGE,
	OE_TEAR_BUMP_OCCURRED,
	OE_DELAY_SWITCH_STATE_CHANGE,
	OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,
	OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,
	OE_DISPENSE_MODE_SWITCH_STATE,
	OE_MANUAL_FEED,
	OE_DISPENSE_TIME_UPDATE,
	OE_DISPENSE_CLICK_UPDATE,
	OE_MAINT_SWITCH_STATE_CHANGE,
	OE_BATTERY_FUEL_GAUGE,
	OE_PRODUCT_FUEL_GAUGE,
	OE_ROLL_STUBBABLE,
	OE_I2C_RESET,
	OE_FUEL_LEVEL,
	OE_FUEL_CLICKS,
	OE_LEFT_NUMBER_STATE_CHANGE,
	OE_MIDDLE_NUMBER_STATE_CHANGE,
	OE_RIGHT_NUMBER_STATE_CHANGE,
	OE_HANG_NUMBER_STATE_CHANGE,
	OE_BUTTON_PRESS,
	NUM_OF_OUTPUT_EVENTS,

	//input event configurations tell the firmware how to
	//watch for and interpret events from the module
	IE_PERFORM_DISPENSE = START_IE,
	IE_ENTER_FAULT_MODE,
	IE_INDICATE_OUT_OF_PRODUCT,
	IE_EXTERNAL_HEARTBEAT_EVENT,
	IE_CLEAR_LATCHED_OUTPUTS,
	IE_EXTERNAL_LOW_BATTERY,
	IE_SET_DISPENSER_STATE,
	IE_CLEAR_TO_SEND,
	IE_TEAR_BUMP,
	IE_MANUAL_FEED,
	IE_MOTOR_TEST,
	IE_EXPANSION_1_TEST,
	IE_EXPANSION_2_TEST,
	IE_I2C_TEST,
	NUM_OF_INPUT_EVENTS
}di_paramKey_t;

typedef enum
{
    DB_DEVICE_INITIALIZED,
    DB_RTC_SET,
    DB_FACTORY_RESET
}di_daughterboardParamKey_t;

typedef enum
{
    DS_NORMAL,
    DS_DISABLE,
    DS_INDICATE,
    NUM_DISP_STATES
}di_dispenserState_t;

#define PARAM_TABLE_SIZE ((NUM_OF_INPUT_EVENTS - START_IE + 1)   \
                        + (NUM_OF_OUTPUT_EVENTS - START_OE + 1) \
                        + NUM_OF_PARAMS + 1)

//gets a bit sequence from a byte from start to end (inclusive). orders bits as 76543210
#define BIT_SEQUENCE(num, start, end)   (((num) & ~(0xFF << ((start) + 1))) >> (end))
#include <stdint.h>
#include "di_commands.h"
#include "di_params.h"
#include "di_events.h"
#include "di_com.h"


#endif /* DI_H_ */
