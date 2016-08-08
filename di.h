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
//#define DISP_COMPACT_VERTICAL_SLED 11
#define DISP_ENMOTION_SOAP_CAP 12

//define the below value to the type of dispenser this is.
#define DISP_TYPE DISP_ENMOTION_SOAP_CAP
#define DISP_TYPE_STR XSTR(DISP_TYPE)
/********************************************************************************************************/
// These three parameters must match, meaning the SWREV must be the first 4 characters of SWREV_STR, and
// the SWDESCRIPTION_STR should match as well. These should be updated every release.
#warning FW version needs updating!
#define SWREV 0x20 //SWREV: 0xMm, M= Major, m = minor
#define BUILDNUM 0x03
#define SWREV_STR "200320160808_1046" //Format: "<SWREV>_YYYYMMDD_HHMM
#define SWDESCRIPTION_STR "enMotionSoap v2.0 build 03"		// max string length 32
/********************************************************************************************************/
#warning DI version needs updating!
#define DISP_INT_REV "20160808_0841"

//These defines are for setting the programming clock on the
//Flash controller. They are divisors of SMCLK and must result
//in a clock between 257kHz and 476kHz. The numbers below place the
//flash clock in the middle of the range for the respective SMCLK
#define FLASH_CLOCK_DIV_16Mhz   44
#define FLASH_CLOCK_DIV_12Mhz   32
#define FLASH_CLOCK_DIV_8Mhz    21
#define FLASH_CLOCK_DIV_1Mhz    3

//Select one of the above based on SMCLK
#define FLASH_CLOCK_DIV FLASH_CLOCK_DIV_8Mhz
#ifndef FLASH_CLOCK_DIV
#error //must define FLASH_CLOCK_DIV to appropriate value
#endif

#ifdef DISP_ENMOTION_SOAP_CAP
///Define OPTIMIZE_DI_TABLES to enable manual trimming of tables to optimal usage sizes
#define OPTIMIZE_DI_TABLES

#ifdef OPTIMIZE_DI_TABLES
#warning DI Table Optimization Enabled

///Define OPTIMIZE_DI_INPUT_EVENTS_TBL to enable manual trimming of INPUT EVENTS tables for code space optimization
#define OPTIMIZE_DI_INPUT_EVENTS_TBL
#define UNUSED_DI_INPUT_EVENTS 		8

///Define OPTIMIZE_DI_OUTPUT_EVENTS_TBL to enable manual trimming of OUTPUT EVENTS tables for code space optimization
#define OPTIMIZE_DI_OUTPUT_EVENTS_TBL
#define UNUSED_DI_OUTPUT_EVENTS 	14

///Define OPTIMIZE_DI_PARAM_TBL to enable manual trimming of PARAM tables for code space optimization
#define OPTIMIZE_DI_PARAM_TBL
#define UNUSED_DI_PARAMS 			13
#endif //OPTIMIZE_DI_TABLES
#endif //DISP_ENMOTION_SOAP_CAP


/**********    Define above for specific dispenser   ********************************/
/************************************************************************************/

// Define NULL.
#ifndef NULL
#define NULL								0
#endif

// Define TRUE and FALSE.
#ifndef TRUE
#define TRUE								1
#endif
#ifndef FALSE
#define FALSE								0
#endif

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
	PARAM_NUMBER_SETTING,
	PARAM_HANG_SETTING,
	PARAM_PRODUCT_ID,						///< SRK: product ID parameter data
	PARAM_TEMPERATURE,						///< SRK: dispenser temperature monitoring
	NUM_OF_PARAMS_M1,    //Added this to support the BSL key parameter, which is defined below this
	PARAM_LAST,
#ifdef OPTIMIZE_DI_PARAM_TBL
	NUM_OF_PARAMS = PARAM_LAST - UNUSED_DI_PARAMS,
#else
	NUM_OF_PARAMS = PARAM_LAST,
#endif //OPTIMIZE_DI_PARAM_TBL
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
	// These are all for Towel
	OE_TEAR_SWITCH_STATE_CHANGE,
	OE_TEAR_BUMP_OCCURRED,
	OE_DELAY_SWITCH_STATE_CHANGE,
	OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,
	OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,
	OE_DISPENSE_MODE_SWITCH_STATE,
	OE_MANUAL_FEED,
	OE_DISPENSE_TIME_UPDATE,
	OE_DISPENSE_CLICK_UPDATE,
	// End Towel
	OE_MAINT_SWITCH_STATE_CHANGE,
	OE_BATTERY_FUEL_GAUGE,
	OE_PRODUCT_FUEL_GAUGE,
	OE_ROLL_STUBBABLE,

	OE_I2C_RESET,	// I2C Reset - Towel
	OE_FUEL_LEVEL,	// Fuel Level - Towel
	OE_FUEL_CLICKS,	// Fuel Clicks - Towel
	OE_NUMBER_STATE_CHANGE,	// Number State Change - Eagle
	OE_BUTTON_PRESS,	// Button Press - Eagle

	OE_PRODUCT_STATE_CHANGE,	// enMotion IoT
	OE_PRODUCT_ID,				// enMotion IoT
	OE_TEMPERATURE,				// enMotion IoT
	OE_LAST,
#ifdef OPTIMIZE_DI_OUTPUT_EVENTS_TBL
	NUM_OF_OUTPUT_EVENTS = OE_LAST - UNUSED_DI_OUTPUT_EVENTS,
#else
	NUM_OF_OUTPUT_EVENTS = OE_LAST,
#endif

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

	IE_LAST,
#ifdef OPTIMIZE_DI_INPUT_EVENTS_TBL
	NUM_OF_INPUT_EVENTS = IE_LAST - UNUSED_DI_INPUT_EVENTS,
#else
	NUM_OF_INPUT_EVENTS = IE_LAST,
#endif
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
