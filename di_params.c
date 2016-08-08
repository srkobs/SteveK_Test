/*
 * di_params.c
 *
 *  Created on: Feb 13, 2014
 *      Author: Dave Murphy
 *
 * © 2013. Confidential and proprietary information of Georgia-Pacific Consumer Products LP. All rights reserved.
 *
 */

#include <stdint.h>
#include <msp430.h>
#include "di.h"

#ifdef DISP_ENMOTION_SOAP
#include "../Bsp.h"
#endif //DISP_ENMOTION_SOAP

#ifdef DISP_ENMOTION_CLASSIC
#include "../init.h"
#include "../UserSettings.h"
#include "../Dispense.h"
extern unsigned char fuelGaugeSwRev;
#endif	//DISP_ENMOTION_CLASSIC

#ifdef DISP_UNIFIED_CHASSIS
#include "../adc.h"
#include "../unifiedProc.h"
#include "../io.h"
#endif // DISP_UNIFIED_CHASSIS

#ifdef DISP_ATHENA_SOAP
#include "../athenaProc.h"
extern bool_t maintPressed;
#endif	//DISP_ATHENA_SOAP
#ifdef DISP_COMPACT_VERTICAL_SLED
#include "../ofgProc.h"
extern bool_t maintPressed;
#endif   //DISP_COMPACT_VERTICAL_SLED

#define GP_MANF_CODE 0x29b1
#pragma RETAIN(fileHeader)
#pragma LOCATION (fileHeader, 0xC000)
const fileHeader_t fileHeader =
{
    0xFFFF,             //uint16_t shadowCRC
    0xFFFF,             //uint16_t CRC
    GP_MANF_CODE,       //uint16_t mfgCode
    DISP_TYPE,          //uint8_t imageType
    SWREV,              //uint16_t imageVer Format: 0xMmBB where M = Major, m = minor, BB = build#
    BUILDNUM,           //Build Number
    0x00004000,         //uint32_t imageLen:# of bytes in fw image (including header)
    SWDESCRIPTION_STR,  //uint8_t[32] description
    {
        DISP_TYPE_STR SWREV_STR,    //swRev should be defined for every release
        DISP_INT_REV     //disIntRev
    } //rONvParams_t rONvParams
}; //This file header is used by the firmware upgrade code.

di_Status_t di_status =
{
    {0},                //flags
    0,                  //activegpio
    0,                  //numDispenses
    0,                  //numCoverOpens
    BATTERY_STARTUP,	//batteryState
    0xFF,               //batteryRemaining
    0xFF,				//productRemaining
    DS_NORMAL,          //dispState
	FGCAL_NOTDONE,		//fuelCal
};
#ifdef DISP_ENMOTION_CLASSIC
extern UserSettings  overrideSettings;
#endif	//DISP_ENMOTION_CLASSIC

#ifdef DISP_UNIFIED_CHASSIS
bool_t tearPressed = FALSE;
bool_t camPressed = FALSE;
extern unsigned int lengthSetting;
extern unsigned int delaySetting;
extern USER_SETTING rangeSetting;
extern bool_t disableSettings;
extern DISPENSER_MODES dispenserMode;
extern uint16_t dispenseTime;
extern uint8_t adjustUserSetting;
extern bool_t useAutoSL;
extern uint8_t fuelGaugeSwRev;
#endif	//DISP_UNIFIED_CHASSIS

#ifdef DISP_EAGLE
extern volatile unsigned int left_number;
extern volatile unsigned int middle_number;
extern volatile unsigned int right_number;
extern volatile unsigned int hang_number;
#endif	//DISP_EAGLE

static void nvParamWrite(di_command_t * command, const di_param_t *pram);
static void nvParamFlashErase(uint8_t *addr);
static const di_param_t * GetParam(di_paramKey_t pKey);


#ifdef DISP_ENMOTION_CLASSIC
#define NV_PARAMS_DEFINED
#define NV_MFG_MODE 0xA5
#pragma DATA_SECTION(nvParams, ".infoC")
const nvParams_t nvParams =
{
	"enMotion Classic", 	    //dispenserType
	"",	        //dispenserSerialNum
	"Z-100011-02",         //dispenserRevision
	NV_MFG_MODE //initialize to MFG_MODE to allow nVParam write
};
#endif	//DISP_ENMOTION_CLASSIC

#ifdef DISP_ENMOTION_SOAP
#define NV_PARAMS_DEFINED
#define NV_MFG_MODE 0xA5
#pragma DATA_SECTION(nvParams, ".infoC")
const nvParams_t nvParams =
{
    "enMotion Soap",        //dispenserType
    "",         //dispenserSerialNum
    "Z-100037-02",         //dispenserRevision
    NV_MFG_MODE //initialize to MFG_MODE to allow nVParam write
};
#endif	//DISP_ENMOTION_SOAP
#ifdef DISP_UNIFIED_CHASSIS
#define NV_PARAMS_DEFINED
#define NV_MFG_MODE 0xA5
#pragma DATA_SECTION(nvParams, ".infoC")
const nvParams_t nvParams =
{
    //These values don't matter, they will be fed in during production
    "UTAC", 	   		 //dispenserType
    "12121212",	        		//dispenserSerialNum
    "Z-121212-01",      //dispenserRevision
    NV_MFG_MODE //initialize to MFG_MODE to allow nVParam write
};
#endif //DISP_UNIFIED_CHASSIS
#ifdef DISP_ATHENA_SOAP
#define NV_PARAMS_DEFINED
#define NV_MFG_MODE 0xA5
#pragma DATA_SECTION(nvParams, ".infoC")
const nvParams_t nvParams =
{
	//These values don't matter, they will be fed in during production
	"ATHENA", 	   		 //dispenserType
	"21212121",	        		//dispenserSerialNum
	"Z-212121-01",      //dispenserRevision
	NV_MFG_MODE //initialize to MFG_MODE to allow nVParam write
};
#endif	//DISP_ATHENA_SOAP
#ifdef DISP_EAGLE
#define NV_PARAMS_DEFINED
#define NV_MFG_MODE 0xA5
#pragma DATA_SECTION(nvParams, ".infoC")
const nvParams_t nvParams =
{
	//These values don't matter, they will be fed in during production
	"EAGLE", 	   		 //dispenserType
	"21212121",	        		//dispenserSerialNum
	"Z-212121-01",      //dispenserRevision
	NV_MFG_MODE //initialize to MFG_MODE to allow nVParam write
};
#endif	//DISP_EAGLE
#ifdef DISP_TISSUE_FUEL
#define NV_PARAMS_DEFINED
#define NV_MFG_MODE 0xA5
#pragma DATA_SECTION(nvParams, ".infoC")
const nvParams_t nvParams =
{
	//These values don't matter, they will be fed in during production
		"TISSUE", 	   		 //dispenserType
		"21212121",	        		//dispenserSerialNum
		"Z-212121-01",      //dispenserRevision
		NV_MFG_MODE //initialize to MFG_MODE to allow nVParam write
};
#endif	//DISP_TISSUE_FUEL

#ifdef DISP_AIRCARE
#define NV_PARAMS_DEFINED
#define NV_MFG_MODE 0xA5
#pragma DATA_SECTION(nvParams, ".infoC")
const nvParams_t nvParams =
{
	//These values don't matter, they will be fed in during production
		"TISSUE", 	   		 //dispenserType
		"21212121",	        		//dispenserSerialNum
		"Z-212121-01",      //dispenserRevision
		NV_MFG_MODE //initialize to MFG_MODE to allow nVParam write
};
#endif	//DISP_AIRCARE

#ifdef DISP_COMPACT_VERTICAL_SLED
#define NV_PARAMS_DEFINED
#define NV_MFG_MODE 0xA5
#pragma DATA_SECTION(nvParams, ".infoC")
const nvParams_t nvParams =
{
	//These values don't matter, they will be fed in during production
	"Compact Vertical", 	//dispenserType
	"",	        			//dispenserSerialNum
	"Z-xxxxxx-01",      	//dispenserRevision
	NV_MFG_MODE 			//initialize to MFG_MODE to allow nVParam write
};
#endif	//DISP_COMPACT_VERTICAL_SLED

#ifndef NV_PARAMS_DEFINED
#error Need to define NV params.
#endif

//Removed in eagle due to it running a different microcontroller
#ifndef DISP_EAGLE
//The following two variables are needed to support firmware upgrade using the Bootstrap Loader
#pragma RETAIN(dispenserBslKey)
#pragma LOCATION (dispenserBslKey, 0xFFE0)
uint8_t dispenserBslKey;

#pragma RETAIN(bslConfigByte)
#pragma LOCATION (bslConfigByte, 0xFFDE)
const uint16_t bslConfigByte = 0x0000;
#endif	//DISP_EAGLE

#ifdef DISP_ENMOTION_SOAP
#define DI_PARAM_TABLE_DEFINED
#pragma DATA_SECTION(diParamTable, ".const")
const di_param_t diParamTable[PARAM_TABLE_SIZE] =
{
        //Data Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {PARAM_SOFTWARE_REVISION,       MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.swRev[0],      READONLY},
        {PARAM_DISPINT_REVISION,        MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.disIntRev[0], 	READONLY},
        {PARAM_DISPENSER_TYPE,          MAXDATABYTES,   (uint8_t*)&nvParams.dispenserType[0],  		    RWMANFMODE},
        {PARAM_DISPENSER_SERIAL_NUMBER, MAXDATABYTES,   (uint8_t*)&nvParams.dispenserSerialNum[0], 		RWMANFMODE},
        {PARAM_DISPENSER_REVISION,      MAXDATABYTES,   (uint8_t*)&nvParams.dispenserRevision[0],  		RWMANFMODE},
        {PARAM_BATTERY_LEVEL,			  1,			(uint8_t*)&di_status.batteryRemaining,			READONLY},
		{PARAM_FAULTSTATUS,               1,            &di_status.flags.sbyte,                			READWRITE},
        {PARAM_PRODUCT_REMAINING,		  1,            (uint8_t*)&di_status.productRemaining, 			READONLY},
        {PARAM_USER_SENSOR_STATE,         1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_MOTOR_SWITCH_STATE,        1,            (uint8_t*)&P2IN,                    			READONLY},
        {PARAM_COVER_SWITCH_STATE,        1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_MOTOR_STATE,               1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_NUM_DISPENSES_SINCE_RESET, 4,            (uint8_t*)&di_status.numDispenses,     			READWRITE},
        {PARAM_TEAR_SWITCH_STATE,    	  1,            NULL,				                         	READONLY},
        {PARAM_DELAY_SETTING,          	  2,            NULL,											READWRITE},
        {PARAM_PAPER_LENGTH_SETTING,	  1,            NULL, 						                    READWRITE},
        {PARAM_SENSOR_RANGE_SETTING,   	  2,            NULL, 										    READWRITE},
        {PARAM_DISPENSE_MODE_SETTING,  	  1,            NULL,						                    READWRITE},
		{PARAM_CAM_SWITCH_STATE,		  1,			NULL,											READONLY},
        {PARAM_SHEET_LENGTH_MODE,   	  1,            NULL,   										READWRITE},
        {PARAM_NEW_USER_TIME,   		  1,            NULL,            								READWRITE},
        {PARAM_NUM_USERS_ADJUST,   		  2,            NULL,   									    READWRITE},
        {PARAM_MAINT_SWITCH_STATE, 	 	  1,			(uint8_t*)&di_status.flags.sbyte,				READONLY},
		{PARAM_LOW_PRODUCT_THRESHOLD,	  1,			NULL,											READWRITE},
		{PARAM_FUEL_GAUGE_CAL,		 	  1,			NULL,											READONLY},
		{PARAM_FUEL_GAUGE_SW_REV,		  1,			NULL,											READONLY},
		{PARAM_LEFT_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_MIDDLE_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_RIGHT_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_HANG_SETTING,			  1,			NULL,											READWRITE},
        {NUM_OF_PARAMS,					  0,			NULL,											READONLY},
		{PARAM_DISPENSER_BSL_KEY,		  MAXDATABYTES, &dispenserBslKey,								READONLY},

		//Output Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {OE_DISPENSE,                           1,          &outputEventConfigs[OE_DISPENSE - START_OE],                        READWRITE},
        {OE_LOW_BATTERY,                        1,          &outputEventConfigs[OE_LOW_BATTERY - START_OE],                     READWRITE},
        {OE_FAULT,                              1,          &outputEventConfigs[OE_FAULT - START_OE],                           READWRITE},
        {OE_PRODUCT_LOW,						1,			&outputEventConfigs[OE_PRODUCT_LOW - START_OE],                     READWRITE},
        {OE_USER_SENSOR_STATE_CHANGE,           1,          &outputEventConfigs[OE_USER_SENSOR_STATE_CHANGE - START_OE],        READWRITE},
        {OE_MOTOR_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MOTOR_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_DOOR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_DOOR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_POWERUP,                            1,          &outputEventConfigs[OE_POWERUP - START_OE],                         READWRITE},
        {OE_ENABLED_EVENT_OCCURRED,             1,          &outputEventConfigs[OE_ENABLED_EVENT_OCCURRED - START_OE],          READWRITE},
        {OE_REQUEST_TO_SEND,					1,			&outputEventConfigs[OE_REQUEST_TO_SEND - START_OE],          		READWRITE},
        {OE_MOTOR_STATE_CHANGE,          		1,          &outputEventConfigs[OE_MOTOR_STATE_CHANGE - START_OE],       		READWRITE},
        {OE_TEAR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_TEAR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_TEAR_BUMP_OCCURRED,                 1,          &outputEventConfigs[OE_TEAR_BUMP_OCCURRED - START_OE],              READWRITE},
        {OE_DELAY_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_DELAY_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_PAPER_LENGTH_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_SENSOR_RANGE_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_DISPENSE_MODE_SWITCH_STATE,         1,          &outputEventConfigs[OE_DISPENSE_MODE_SWITCH_STATE - START_OE],      READWRITE},
        {OE_MANUAL_FEED,                        1,          &outputEventConfigs[OE_MANUAL_FEED - START_OE],                     READWRITE},
        {OE_DISPENSE_TIME_UPDATE,               1,          &outputEventConfigs[OE_DISPENSE_TIME_UPDATE - START_OE],            READWRITE},
        {OE_DISPENSE_CLICK_UPDATE,              1,          &outputEventConfigs[OE_DISPENSE_CLICK_UPDATE - START_OE],           READWRITE},
        {OE_MAINT_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MAINT_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_BATTERY_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_BATTERY_FUEL_GAUGE - START_OE],              READWRITE},
        {OE_PRODUCT_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_PRODUCT_FUEL_GAUGE - START_OE],              READWRITE},
		{OE_ROLL_STUBBABLE,						1,			&outputEventConfigs[OE_ROLL_STUBBABLE - START_OE],					READWRITE},
		{OE_I2C_RESET,							1,			&outputEventConfigs[OE_I2C_RESET - START_OE],						READWRITE},
		{OE_FUEL_LEVEL,							1,			&outputEventConfigs[OE_FUEL_LEVEL - START_OE],						READWRITE},
		{OE_FUEL_CLICKS,						1,			&outputEventConfigs[OE_FUEL_CLICKS - START_OE],						READWRITE},
		{OE_LEFT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_LEFT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_MIDDLE_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_MIDDLE_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_RIGHT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_RIGHT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_HANG_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_HANG_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_BUTTON_PRESS,						1,			&outputEventConfigs[OE_BUTTON_PRESS - START_OE],					READWRITE},
        {NUM_OF_OUTPUT_EVENTS,					0,			NULL,																READONLY},
        //Input Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {IE_PERFORM_DISPENSE,              1,          &inputEventConfigs[IE_PERFORM_DISPENSE - START_IE],   		READWRITE},
        {IE_ENTER_FAULT_MODE,              1,          &inputEventConfigs[IE_ENTER_FAULT_MODE - START_IE],   		READWRITE},
        {IE_INDICATE_OUT_OF_PRODUCT,       1,          &inputEventConfigs[IE_INDICATE_OUT_OF_PRODUCT - START_IE],   READWRITE},
        {IE_EXTERNAL_HEARTBEAT_EVENT,      1,          &inputEventConfigs[IE_EXTERNAL_HEARTBEAT_EVENT - START_IE],  READWRITE},
        {IE_CLEAR_LATCHED_OUTPUTS,         1,          &inputEventConfigs[IE_CLEAR_LATCHED_OUTPUTS - START_IE],   	READWRITE},
        {IE_EXTERNAL_LOW_BATTERY,          1,          &inputEventConfigs[IE_EXTERNAL_LOW_BATTERY - START_IE],   	READWRITE},
        {IE_SET_DISPENSER_STATE,           1,          &inputEventConfigs[IE_SET_DISPENSER_STATE - START_IE],   	READWRITE},
        {IE_CLEAR_TO_SEND,			       1,		   &inputEventConfigs[IE_CLEAR_TO_SEND - START_OE],          	READWRITE},
        {IE_TEAR_BUMP,                     1,          &inputEventConfigs[IE_TEAR_BUMP - START_IE],   				READWRITE},
        {IE_MANUAL_FEED,                   1,          &inputEventConfigs[IE_MANUAL_FEED - START_IE],   			READWRITE},
		{IE_MOTOR_TEST,                    1,          &inputEventConfigs[IE_MOTOR_TEST - START_IE],   				READWRITE},
		{IE_EXPANSION_1_TEST,              1,          &inputEventConfigs[IE_EXPANSION_1_TEST - START_IE],   		READWRITE},
		{IE_EXPANSION_2_TEST,              1,          &inputEventConfigs[IE_EXPANSION_2_TEST - START_IE],   		READWRITE},
		{IE_I2C_TEST,		               1,          &inputEventConfigs[IE_I2C_TEST - START_IE],   				READWRITE},
		{NUM_OF_INPUT_EVENTS,			   0,		   NULL,														READONLY},
};
#endif	//DISP_ENMOTION_SOAP

#ifdef DISP_ENMOTION_CLASSIC
#define DI_PARAM_TABLE_DEFINED
#pragma DATA_SECTION(diParamTable, ".const")
const di_param_t diParamTable[PARAM_TABLE_SIZE] =
{
        //Data Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {PARAM_SOFTWARE_REVISION,       MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.swRev[0],      READONLY},
        {PARAM_DISPINT_REVISION,        MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.disIntRev[0], 	READONLY},
        {PARAM_DISPENSER_TYPE,          MAXDATABYTES,   (uint8_t*)&nvParams.dispenserType[0],  		    RWMANFMODE},
        {PARAM_DISPENSER_SERIAL_NUMBER, MAXDATABYTES,   (uint8_t*)&nvParams.dispenserSerialNum[0], 		RWMANFMODE},
        {PARAM_DISPENSER_REVISION,      MAXDATABYTES,   (uint8_t*)&nvParams.dispenserRevision[0],  		RWMANFMODE},
        {PARAM_BATTERY_LEVEL,			  1,			(uint8_t*)&di_status.batteryRemaining,			READONLY},
		{PARAM_FAULTSTATUS,               1,            &di_status.flags.sbyte,                			READWRITE},
        {PARAM_PRODUCT_REMAINING,		  1,            (uint8_t*)&di_status.productRemaining, 			READONLY},
        {PARAM_USER_SENSOR_STATE,         1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_MOTOR_SWITCH_STATE,        1,            (uint8_t*)&P2IN,                    			READONLY},
        {PARAM_COVER_SWITCH_STATE,        1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_MOTOR_STATE,               1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_NUM_DISPENSES_SINCE_RESET, 4,            (uint8_t*)&di_status.numDispenses,     			READWRITE},
        {PARAM_TEAR_SWITCH_STATE,    	  1,            NULL,				                         	READONLY},
        {PARAM_DELAY_SETTING,          	  2,            (uint8_t*)&overrideSettings.SelectedDelay,      READWRITE},
        {PARAM_PAPER_LENGTH_SETTING,	  1,            &overrideSettings.TowelLength,                  READWRITE},
        {PARAM_SENSOR_RANGE_SETTING,   	  2,            (uint8_t*)&overrideSettings.ProxySensorRange,   READWRITE},
        {PARAM_DISPENSE_MODE_SETTING,  	  1,            &overrideSettings.DispenseMode,                 READWRITE},
		{PARAM_CAM_SWITCH_STATE,		  1,			NULL,											READONLY},
        {PARAM_SHEET_LENGTH_MODE,   	  1,            (uint8_t*)&overrideSettings.SheetLengthMode,    READWRITE},
        {PARAM_NEW_USER_TIME,   		  1,            &overrideSettings.newUserTime100ms,             READWRITE},
        {PARAM_NUM_USERS_ADJUST,   		  2,            (uint8_t*)&overrideSettings.numUsersAdjust,     READWRITE},
        {PARAM_MAINT_SWITCH_STATE, 	 	  1,			(uint8_t*)&di_status.flags.sbyte,				READONLY},
		{PARAM_LOW_PRODUCT_THRESHOLD,	  1,			NULL,											READWRITE},
		{PARAM_FUEL_GAUGE_CAL,		 	  1,			(uint8_t*)&di_status.fuelCal,					READONLY},
		{PARAM_FUEL_GAUGE_SW_REV,		  1,			(uint8_t*)&fuelGaugeSwRev,						READONLY},
		{PARAM_LEFT_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_MIDDLE_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_RIGHT_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_HANG_SETTING,			  1,			NULL,											READWRITE},
        {NUM_OF_PARAMS,					  0,			NULL,											READONLY},
		{PARAM_DISPENSER_BSL_KEY,		  MAXDATABYTES, &dispenserBslKey,								READONLY},

		//Output Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {OE_DISPENSE,                           1,          &outputEventConfigs[OE_DISPENSE - START_OE],                        READWRITE},
        {OE_LOW_BATTERY,                        1,          &outputEventConfigs[OE_LOW_BATTERY - START_OE],                     READWRITE},
        {OE_FAULT,                              1,          &outputEventConfigs[OE_FAULT - START_OE],                           READWRITE},
        {OE_PRODUCT_LOW,						1,			&outputEventConfigs[OE_PRODUCT_LOW - START_OE],                     READWRITE},
        {OE_USER_SENSOR_STATE_CHANGE,           1,          &outputEventConfigs[OE_USER_SENSOR_STATE_CHANGE - START_OE],        READWRITE},
        {OE_MOTOR_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MOTOR_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_DOOR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_DOOR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_POWERUP,                            1,          &outputEventConfigs[OE_POWERUP - START_OE],                         READWRITE},
        {OE_ENABLED_EVENT_OCCURRED,             1,          &outputEventConfigs[OE_ENABLED_EVENT_OCCURRED - START_OE],          READWRITE},
        {OE_REQUEST_TO_SEND,					1,			&outputEventConfigs[OE_REQUEST_TO_SEND - START_OE],          		READWRITE},
        {OE_MOTOR_STATE_CHANGE,          		1,          &outputEventConfigs[OE_MOTOR_STATE_CHANGE - START_OE],       		READWRITE},
        {OE_TEAR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_TEAR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_TEAR_BUMP_OCCURRED,                 1,          &outputEventConfigs[OE_TEAR_BUMP_OCCURRED - START_OE],              READWRITE},
        {OE_DELAY_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_DELAY_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_PAPER_LENGTH_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_SENSOR_RANGE_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_DISPENSE_MODE_SWITCH_STATE,         1,          &outputEventConfigs[OE_DISPENSE_MODE_SWITCH_STATE - START_OE],      READWRITE},
        {OE_MANUAL_FEED,                        1,          &outputEventConfigs[OE_MANUAL_FEED - START_OE],                     READWRITE},
        {OE_DISPENSE_TIME_UPDATE,               1,          &outputEventConfigs[OE_DISPENSE_TIME_UPDATE - START_OE],            READWRITE},
        {OE_DISPENSE_CLICK_UPDATE,              1,          &outputEventConfigs[OE_DISPENSE_CLICK_UPDATE - START_OE],           READWRITE},
        {OE_MAINT_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MAINT_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_BATTERY_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_BATTERY_FUEL_GAUGE - START_OE],              READWRITE},
        {OE_PRODUCT_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_PRODUCT_FUEL_GAUGE - START_OE],              READWRITE},
		{OE_ROLL_STUBBABLE,						1,			&outputEventConfigs[OE_ROLL_STUBBABLE - START_OE],					READWRITE},
		{OE_I2C_RESET,							1,			&outputEventConfigs[OE_I2C_RESET - START_OE],						READWRITE},
		{OE_FUEL_LEVEL,							1,			&outputEventConfigs[OE_FUEL_LEVEL - START_OE],						READWRITE},
		{OE_FUEL_CLICKS,						1,			&outputEventConfigs[OE_FUEL_CLICKS - START_OE],						READWRITE},
		{OE_LEFT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_LEFT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_MIDDLE_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_MIDDLE_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_RIGHT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_RIGHT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_HANG_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_HANG_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_BUTTON_PRESS,						1,			&outputEventConfigs[OE_BUTTON_PRESS - START_OE],					READWRITE},
        {NUM_OF_OUTPUT_EVENTS,					0,			NULL,																READONLY},
        //Input Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {IE_PERFORM_DISPENSE,              1,          &inputEventConfigs[IE_PERFORM_DISPENSE - START_IE],   		READWRITE},
        {IE_ENTER_FAULT_MODE,              1,          &inputEventConfigs[IE_ENTER_FAULT_MODE - START_IE],   		READWRITE},
        {IE_INDICATE_OUT_OF_PRODUCT,       1,          &inputEventConfigs[IE_INDICATE_OUT_OF_PRODUCT - START_IE],   READWRITE},
        {IE_EXTERNAL_HEARTBEAT_EVENT,      1,          &inputEventConfigs[IE_EXTERNAL_HEARTBEAT_EVENT - START_IE],  READWRITE},
        {IE_CLEAR_LATCHED_OUTPUTS,         1,          &inputEventConfigs[IE_CLEAR_LATCHED_OUTPUTS - START_IE],   	READWRITE},
        {IE_EXTERNAL_LOW_BATTERY,          1,          &inputEventConfigs[IE_EXTERNAL_LOW_BATTERY - START_IE],   	READWRITE},
        {IE_SET_DISPENSER_STATE,           1,          &inputEventConfigs[IE_SET_DISPENSER_STATE - START_IE],   	READWRITE},
        {IE_CLEAR_TO_SEND,			       1,		   &inputEventConfigs[IE_CLEAR_TO_SEND - START_OE],          	READWRITE},
        {IE_TEAR_BUMP,                     1,          &inputEventConfigs[IE_TEAR_BUMP - START_IE],   				READWRITE},
        {IE_MANUAL_FEED,                   1,          &inputEventConfigs[IE_MANUAL_FEED - START_IE],   			READWRITE},
		{IE_MOTOR_TEST,                    1,          &inputEventConfigs[IE_MOTOR_TEST - START_IE],   				READWRITE},
		{IE_EXPANSION_1_TEST,              1,          &inputEventConfigs[IE_EXPANSION_1_TEST - START_IE],   		READWRITE},
		{IE_EXPANSION_2_TEST,              1,          &inputEventConfigs[IE_EXPANSION_2_TEST - START_IE],   		READWRITE},
		{IE_I2C_TEST,		               1,          &inputEventConfigs[IE_I2C_TEST - START_IE],   				READWRITE},
		{NUM_OF_INPUT_EVENTS,			   0,		   NULL,														READONLY},
};
#endif	//DISP_ENMOTION_CLASSIC

#ifdef DISP_UNIFIED_CHASSIS
#define DI_PARAM_TABLE_DEFINED
#pragma DATA_SECTION(diParamTable, ".const")
const di_param_t diParamTable[PARAM_TABLE_SIZE] =
{
        //Data Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {PARAM_SOFTWARE_REVISION,       MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.swRev[0],      READONLY},
        {PARAM_DISPINT_REVISION,        MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.disIntRev[0], 	READONLY},
        {PARAM_DISPENSER_TYPE,          MAXDATABYTES,   (uint8_t*)&nvParams.dispenserType[0],  		    RWMANFMODE},
        {PARAM_DISPENSER_SERIAL_NUMBER, MAXDATABYTES,   (uint8_t*)&nvParams.dispenserSerialNum[0], 		RWMANFMODE},
        {PARAM_DISPENSER_REVISION,      MAXDATABYTES,   (uint8_t*)&nvParams.dispenserRevision[0],  		RWMANFMODE},
        {PARAM_BATTERY_LEVEL,			  1,			(uint8_t*)&di_status.batteryRemaining,			READONLY},
		{PARAM_FAULTSTATUS,               1,            &di_status.flags.sbyte,                			READWRITE},
        {PARAM_PRODUCT_REMAINING,		  1,            (uint8_t*)&di_status.productRemaining, 			READONLY},
        {PARAM_USER_SENSOR_STATE,         1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_MOTOR_SWITCH_STATE,        1,            (uint8_t*)&P2IN,                    			READONLY},
        {PARAM_COVER_SWITCH_STATE,        1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_MOTOR_STATE,               1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_NUM_DISPENSES_SINCE_RESET, 4,            (uint8_t*)&di_status.numDispenses,     			READWRITE},
        {PARAM_TEAR_SWITCH_STATE,    	  1,            (uint8_t*)&tearPressed,                        	READONLY},
        {PARAM_DELAY_SETTING,          	  2,            (uint8_t*)&delaySetting,					    READWRITE},
        {PARAM_PAPER_LENGTH_SETTING,	  1,            (uint8_t*)&lengthSetting,	                    READWRITE},
        {PARAM_SENSOR_RANGE_SETTING,   	  2,            (uint8_t*)&rangeSetting,					    READWRITE},
        {PARAM_DISPENSE_MODE_SETTING,  	  1,            (uint8_t*)&dispenserMode,		                READWRITE},
		{PARAM_CAM_SWITCH_STATE,		  1,			(uint8_t*)&camPressed,							READONLY},
//      {PARAM_SHEET_LENGTH_MODE,   	  1,            (uint8_t*)&useAutoSL,						    READWRITE},
//      {PARAM_NEW_USER_TIME,   		  1,            (uint8_t*)&dispenseTime,			            READWRITE},
//		{PARAM_NUM_USERS_ADJUST,   		  2,            (uint8_t*)&adjustUserSetting,			        READWRITE},
		{PARAM_SHEET_LENGTH_MODE,   	  1,            NULL,										    READWRITE},	//TODO:CHANGED to allow the code to compile into a smaller footprint so that we can fit everything
		{PARAM_NEW_USER_TIME,   		  1,            NULL,								            READWRITE},	//TODO:CHANGED to allow the code to compile into a smaller footprint so that we can fit everything
		{PARAM_NUM_USERS_ADJUST,   		  2,            NULL,									        READWRITE},	//TODO:CHANGED to allow the code to compile into a smaller footprint so that we can fit everything
        {PARAM_MAINT_SWITCH_STATE, 	 	  1,			NULL,											READONLY},
		{PARAM_LOW_PRODUCT_THRESHOLD,	  1,			NULL,											READWRITE},
		{PARAM_FUEL_GAUGE_CAL,		 	  1,			(uint8_t*)&di_status.fuelCal,					READONLY},
		{PARAM_FUEL_GAUGE_SW_REV,		  1,			(uint8_t*)&fuelGaugeSwRev,						READONLY},
		{PARAM_LEFT_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_MIDDLE_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_RIGHT_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_HANG_SETTING,			  1,			NULL,											READWRITE},
        {NUM_OF_PARAMS,					  0,			NULL,											READONLY},
		{PARAM_DISPENSER_BSL_KEY,		  MAXDATABYTES, &dispenserBslKey,								READONLY},

		//Output Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {OE_DISPENSE,                           1,          &outputEventConfigs[OE_DISPENSE - START_OE],                        READWRITE},
        {OE_LOW_BATTERY,                        1,          &outputEventConfigs[OE_LOW_BATTERY - START_OE],                     READWRITE},
        {OE_FAULT,                              1,          &outputEventConfigs[OE_FAULT - START_OE],                           READWRITE},
        {OE_PRODUCT_LOW,						1,			&outputEventConfigs[OE_PRODUCT_LOW - START_OE],                     READWRITE},
        {OE_USER_SENSOR_STATE_CHANGE,           1,          &outputEventConfigs[OE_USER_SENSOR_STATE_CHANGE - START_OE],        READWRITE},
        {OE_MOTOR_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MOTOR_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_DOOR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_DOOR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_POWERUP,                            1,          &outputEventConfigs[OE_POWERUP - START_OE],                         READWRITE},
        {OE_ENABLED_EVENT_OCCURRED,             1,          &outputEventConfigs[OE_ENABLED_EVENT_OCCURRED - START_OE],          READWRITE},
        {OE_REQUEST_TO_SEND,					1,			&outputEventConfigs[OE_REQUEST_TO_SEND - START_OE],          		READWRITE},
        {OE_MOTOR_STATE_CHANGE,          		1,          &outputEventConfigs[OE_MOTOR_STATE_CHANGE - START_OE],       		READWRITE},
        {OE_TEAR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_TEAR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_TEAR_BUMP_OCCURRED,                 1,          &outputEventConfigs[OE_TEAR_BUMP_OCCURRED - START_OE],              READWRITE},
        {OE_DELAY_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_DELAY_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_PAPER_LENGTH_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_SENSOR_RANGE_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_DISPENSE_MODE_SWITCH_STATE,         1,          &outputEventConfigs[OE_DISPENSE_MODE_SWITCH_STATE - START_OE],      READWRITE},
        {OE_MANUAL_FEED,                        1,          &outputEventConfigs[OE_MANUAL_FEED - START_OE],                     READWRITE},
        {OE_DISPENSE_TIME_UPDATE,               1,          &outputEventConfigs[OE_DISPENSE_TIME_UPDATE - START_OE],            READWRITE},
        {OE_DISPENSE_CLICK_UPDATE,              1,          &outputEventConfigs[OE_DISPENSE_CLICK_UPDATE - START_OE],           READWRITE},
        {OE_MAINT_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MAINT_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_BATTERY_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_BATTERY_FUEL_GAUGE - START_OE],              READWRITE},
        {OE_PRODUCT_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_PRODUCT_FUEL_GAUGE - START_OE],              READWRITE},
		{OE_ROLL_STUBBABLE,						1,			&outputEventConfigs[OE_ROLL_STUBBABLE - START_OE],					READWRITE},
		{OE_I2C_RESET,							1,			&outputEventConfigs[OE_I2C_RESET - START_OE],						READWRITE},
		{OE_FUEL_LEVEL,							1,			&outputEventConfigs[OE_FUEL_LEVEL - START_OE],						READWRITE},
		{OE_FUEL_CLICKS,						1,			&outputEventConfigs[OE_FUEL_CLICKS - START_OE],						READWRITE},
		{OE_LEFT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_LEFT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_MIDDLE_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_MIDDLE_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_RIGHT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_RIGHT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_HANG_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_HANG_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_BUTTON_PRESS,						1,			&outputEventConfigs[OE_BUTTON_PRESS - START_OE],					READWRITE},
        {NUM_OF_OUTPUT_EVENTS,					0,			NULL,																READONLY},
        //Input Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {IE_PERFORM_DISPENSE,              1,          &inputEventConfigs[IE_PERFORM_DISPENSE - START_IE],   		READWRITE},
        {IE_ENTER_FAULT_MODE,              1,          &inputEventConfigs[IE_ENTER_FAULT_MODE - START_IE],   		READWRITE},
        {IE_INDICATE_OUT_OF_PRODUCT,       1,          &inputEventConfigs[IE_INDICATE_OUT_OF_PRODUCT - START_IE],   READWRITE},
        {IE_EXTERNAL_HEARTBEAT_EVENT,      1,          &inputEventConfigs[IE_EXTERNAL_HEARTBEAT_EVENT - START_IE],  READWRITE},
        {IE_CLEAR_LATCHED_OUTPUTS,         1,          &inputEventConfigs[IE_CLEAR_LATCHED_OUTPUTS - START_IE],   	READWRITE},
        {IE_EXTERNAL_LOW_BATTERY,          1,          &inputEventConfigs[IE_EXTERNAL_LOW_BATTERY - START_IE],   	READWRITE},
        {IE_SET_DISPENSER_STATE,           1,          &inputEventConfigs[IE_SET_DISPENSER_STATE - START_IE],   	READWRITE},
        {IE_CLEAR_TO_SEND,			       1,		   &inputEventConfigs[IE_CLEAR_TO_SEND - START_OE],          	READWRITE},
        {IE_TEAR_BUMP,                     1,          &inputEventConfigs[IE_TEAR_BUMP - START_IE],   				READWRITE},
        {IE_MANUAL_FEED,                   1,          &inputEventConfigs[IE_MANUAL_FEED - START_IE],   			READWRITE},
		{IE_MOTOR_TEST,                    1,          &inputEventConfigs[IE_MOTOR_TEST - START_IE],   				READWRITE},
		{IE_EXPANSION_1_TEST,              1,          &inputEventConfigs[IE_EXPANSION_1_TEST - START_IE],   		READWRITE},
		{IE_EXPANSION_2_TEST,              1,          &inputEventConfigs[IE_EXPANSION_2_TEST - START_IE],   		READWRITE},
		{IE_I2C_TEST,		               1,          &inputEventConfigs[IE_I2C_TEST - START_IE],   				READWRITE},
		{NUM_OF_INPUT_EVENTS,			   0,		   NULL,														READONLY},
};
#endif	//DISP_UNIFIED_CHASSIS

#ifdef DISP_ATHENA_SOAP
#define DI_PARAM_TABLE_DEFINED
#pragma DATA_SECTION(diParamTable, ".const")
const di_param_t diParamTable[PARAM_TABLE_SIZE] =
{
        //Data Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {PARAM_SOFTWARE_REVISION,       MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.swRev[0],      READONLY},
        {PARAM_DISPINT_REVISION,        MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.disIntRev[0], 	READONLY},
        {PARAM_DISPENSER_TYPE,          MAXDATABYTES,   (uint8_t*)&nvParams.dispenserType[0],  		    RWMANFMODE},
        {PARAM_DISPENSER_SERIAL_NUMBER, MAXDATABYTES,   (uint8_t*)&nvParams.dispenserSerialNum[0], 		RWMANFMODE},
        {PARAM_DISPENSER_REVISION,      MAXDATABYTES,   (uint8_t*)&nvParams.dispenserRevision[0],  		RWMANFMODE},
        {PARAM_BATTERY_LEVEL,			  1,			(uint8_t*)&di_status.batteryRemaining,			READONLY},
		{PARAM_FAULTSTATUS,               1,            &di_status.flags.sbyte,                			READWRITE},
        {PARAM_PRODUCT_REMAINING,		  1,            (uint8_t*)&di_status.productRemaining, 			READONLY},
        {PARAM_USER_SENSOR_STATE,         1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_MOTOR_SWITCH_STATE,        1,            (uint8_t*)&P2IN,                    			READONLY},
        {PARAM_COVER_SWITCH_STATE,        1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_MOTOR_STATE,               1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_NUM_DISPENSES_SINCE_RESET, 4,            (uint8_t*)&di_status.numDispenses,     			READWRITE},
		{PARAM_TEAR_SWITCH_STATE,    	  1,            NULL,				                         	READONLY},
		{PARAM_DELAY_SETTING,          	  2,            NULL,											READWRITE},
		{PARAM_PAPER_LENGTH_SETTING,	  1,            NULL, 						                    READWRITE},
		{PARAM_SENSOR_RANGE_SETTING,   	  2,            NULL, 										    READWRITE},
		{PARAM_DISPENSE_MODE_SETTING,  	  1,            NULL,						                    READWRITE},
		{PARAM_CAM_SWITCH_STATE,		  1,			NULL,											READONLY},
		{PARAM_SHEET_LENGTH_MODE,   	  1,            NULL,   										READWRITE},
		{PARAM_NEW_USER_TIME,   		  1,            NULL,            								READWRITE},
		{PARAM_NUM_USERS_ADJUST,   		  2,            NULL,   									    READWRITE},
		{PARAM_MAINT_SWITCH_STATE, 	 	  1,			(uint8_t*)&di_status.flags.sbyte,				READONLY},
		{PARAM_LOW_PRODUCT_THRESHOLD,	  1,			NULL,											READWRITE},
		{PARAM_FUEL_GAUGE_CAL,		 	  1,			NULL,											READONLY},
		{PARAM_FUEL_GAUGE_SW_REV,		  1,			NULL,											READONLY},
		{PARAM_LEFT_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_MIDDLE_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_RIGHT_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_HANG_SETTING,			  1,			NULL,											READWRITE},
		{NUM_OF_PARAMS,					  0,			NULL,											READONLY},
		{PARAM_DISPENSER_BSL_KEY,		  MAXDATABYTES, &dispenserBslKey,								READONLY},

		//Output Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {OE_DISPENSE,                           1,          &outputEventConfigs[OE_DISPENSE - START_OE],                        READWRITE},
        {OE_LOW_BATTERY,                        1,          &outputEventConfigs[OE_LOW_BATTERY - START_OE],                     READWRITE},
        {OE_FAULT,                              1,          &outputEventConfigs[OE_FAULT - START_OE],                           READWRITE},
        {OE_PRODUCT_LOW,						1,			&outputEventConfigs[OE_PRODUCT_LOW - START_OE],                     READWRITE},
        {OE_USER_SENSOR_STATE_CHANGE,           1,          &outputEventConfigs[OE_USER_SENSOR_STATE_CHANGE - START_OE],        READWRITE},
        {OE_MOTOR_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MOTOR_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_DOOR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_DOOR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_POWERUP,                            1,          &outputEventConfigs[OE_POWERUP - START_OE],                         READWRITE},
        {OE_ENABLED_EVENT_OCCURRED,             1,          &outputEventConfigs[OE_ENABLED_EVENT_OCCURRED - START_OE],          READWRITE},
        {OE_REQUEST_TO_SEND,					1,			&outputEventConfigs[OE_REQUEST_TO_SEND - START_OE],          		READWRITE},
        {OE_MOTOR_STATE_CHANGE,          		1,          &outputEventConfigs[OE_MOTOR_STATE_CHANGE - START_OE],       		READWRITE},
        {OE_TEAR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_TEAR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_TEAR_BUMP_OCCURRED,                 1,          &outputEventConfigs[OE_TEAR_BUMP_OCCURRED - START_OE],              READWRITE},
        {OE_DELAY_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_DELAY_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_PAPER_LENGTH_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_SENSOR_RANGE_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_DISPENSE_MODE_SWITCH_STATE,         1,          &outputEventConfigs[OE_DISPENSE_MODE_SWITCH_STATE - START_OE],      READWRITE},
        {OE_MANUAL_FEED,                        1,          &outputEventConfigs[OE_MANUAL_FEED - START_OE],                     READWRITE},
        {OE_DISPENSE_TIME_UPDATE,               1,          &outputEventConfigs[OE_DISPENSE_TIME_UPDATE - START_OE],            READWRITE},
        {OE_DISPENSE_CLICK_UPDATE,              1,          &outputEventConfigs[OE_DISPENSE_CLICK_UPDATE - START_OE],           READWRITE},
        {OE_MAINT_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MAINT_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_BATTERY_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_BATTERY_FUEL_GAUGE - START_OE],              READWRITE},
        {OE_PRODUCT_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_PRODUCT_FUEL_GAUGE - START_OE],              READWRITE},
		{OE_ROLL_STUBBABLE,						1,			&outputEventConfigs[OE_ROLL_STUBBABLE - START_OE],					READWRITE},
		{OE_I2C_RESET,							1,			&outputEventConfigs[OE_I2C_RESET - START_OE],						READWRITE},
		{OE_FUEL_LEVEL,							1,			&outputEventConfigs[OE_FUEL_LEVEL - START_OE],						READWRITE},
		{OE_FUEL_CLICKS,						1,			&outputEventConfigs[OE_FUEL_CLICKS - START_OE],						READWRITE},
		{OE_LEFT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_LEFT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_MIDDLE_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_MIDDLE_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_RIGHT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_RIGHT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_HANG_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_HANG_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_BUTTON_PRESS,						1,			&outputEventConfigs[OE_BUTTON_PRESS - START_OE],					READWRITE},
        {NUM_OF_OUTPUT_EVENTS,					0,			NULL,																READONLY},
        //Input Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {IE_PERFORM_DISPENSE,              1,          &inputEventConfigs[IE_PERFORM_DISPENSE - START_IE],   		READWRITE},
        {IE_ENTER_FAULT_MODE,              1,          &inputEventConfigs[IE_ENTER_FAULT_MODE - START_IE],   		READWRITE},
        {IE_INDICATE_OUT_OF_PRODUCT,       1,          &inputEventConfigs[IE_INDICATE_OUT_OF_PRODUCT - START_IE],   READWRITE},
        {IE_EXTERNAL_HEARTBEAT_EVENT,      1,          &inputEventConfigs[IE_EXTERNAL_HEARTBEAT_EVENT - START_IE],  READWRITE},
        {IE_CLEAR_LATCHED_OUTPUTS,         1,          &inputEventConfigs[IE_CLEAR_LATCHED_OUTPUTS - START_IE],   	READWRITE},
        {IE_EXTERNAL_LOW_BATTERY,          1,          &inputEventConfigs[IE_EXTERNAL_LOW_BATTERY - START_IE],   	READWRITE},
        {IE_SET_DISPENSER_STATE,           1,          &inputEventConfigs[IE_SET_DISPENSER_STATE - START_IE],   	READWRITE},
        {IE_CLEAR_TO_SEND,			       1,		   &inputEventConfigs[IE_CLEAR_TO_SEND - START_OE],          	READWRITE},
        {IE_TEAR_BUMP,                     1,          &inputEventConfigs[IE_TEAR_BUMP - START_IE],   				READWRITE},
        {IE_MANUAL_FEED,                   1,          &inputEventConfigs[IE_MANUAL_FEED - START_IE],   			READWRITE},
		{IE_MOTOR_TEST,                    1,          &inputEventConfigs[IE_MOTOR_TEST - START_IE],   				READWRITE},
		{IE_EXPANSION_1_TEST,              1,          &inputEventConfigs[IE_EXPANSION_1_TEST - START_IE],   		READWRITE},
		{IE_EXPANSION_2_TEST,              1,          &inputEventConfigs[IE_EXPANSION_2_TEST - START_IE],   		READWRITE},
		{IE_I2C_TEST,		               1,          &inputEventConfigs[IE_I2C_TEST - START_IE],   				READWRITE},
		{NUM_OF_INPUT_EVENTS,			   0,		   NULL,														READONLY},
};
#endif	//DISP_ATHENA_SOAP

#ifdef DISP_EAGLE
#define DI_PARAM_TABLE_DEFINED
#pragma DATA_SECTION(diParamTable, ".const")
const di_param_t diParamTable[PARAM_TABLE_SIZE] =
{
        //Data Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {PARAM_SOFTWARE_REVISION,       MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.swRev[0],      READONLY},
        {PARAM_DISPINT_REVISION,        MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.disIntRev[0], 	READONLY},
        {PARAM_DISPENSER_TYPE,          MAXDATABYTES,   (uint8_t*)&nvParams.dispenserType[0],  		    RWMANFMODE},
        {PARAM_DISPENSER_SERIAL_NUMBER, MAXDATABYTES,   (uint8_t*)&nvParams.dispenserSerialNum[0], 		RWMANFMODE},
        {PARAM_DISPENSER_REVISION,      MAXDATABYTES,   (uint8_t*)&nvParams.dispenserRevision[0],  		RWMANFMODE},
        {PARAM_BATTERY_LEVEL,			  1,			(uint8_t*)&di_status.batteryRemaining,			READONLY},
		{PARAM_FAULTSTATUS,               1,            &di_status.flags.sbyte,                			READWRITE},
        {PARAM_PRODUCT_REMAINING,		  1,            (uint8_t*)&di_status.productRemaining, 			READONLY},
        {PARAM_USER_SENSOR_STATE,         1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_MOTOR_SWITCH_STATE,        1,            (uint8_t*)&P2IN,                    			READONLY},
        {PARAM_COVER_SWITCH_STATE,        1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_MOTOR_STATE,               1,            NULL,           				     			READONLY},
        {PARAM_NUM_DISPENSES_SINCE_RESET, 4,            (uint8_t*)&di_status.numDispenses,     			READWRITE},
        {PARAM_TEAR_SWITCH_STATE,    	  1,            NULL,				                         	READONLY},
        {PARAM_DELAY_SETTING,          	  2,            NULL,      										READWRITE},
        {PARAM_PAPER_LENGTH_SETTING,	  1,            NULL,	              							READWRITE},
        {PARAM_SENSOR_RANGE_SETTING,   	  2,            NULL,   										READWRITE},
        {PARAM_DISPENSE_MODE_SETTING,  	  1,            NULL,                 							READWRITE},
		{PARAM_CAM_SWITCH_STATE,		  1,			NULL,											READONLY},
        {PARAM_SHEET_LENGTH_MODE,   	  1,            NULL,    										READWRITE},
        {PARAM_NEW_USER_TIME,   		  1,            NULL,             								READWRITE},
        {PARAM_NUM_USERS_ADJUST,   		  2,            NULL,     										READWRITE},
        {PARAM_MAINT_SWITCH_STATE, 	 	  1,			NULL,											READONLY},
		{PARAM_LOW_PRODUCT_THRESHOLD,	  1,			NULL,											READWRITE},
		{PARAM_FUEL_GAUGE_CAL,		 	  1,			NULL,											READONLY},
		{PARAM_FUEL_GAUGE_SW_REV,		  1,			NULL,											READONLY},
		{PARAM_LEFT_SETTING,			  1,			(uint8_t*)&left_number,							READWRITE},
		{PARAM_MIDDLE_SETTING,			  1,			(uint8_t*)&middle_number,						READWRITE},
		{PARAM_RIGHT_SETTING,			  1,			(uint8_t*)&right_number,						READWRITE},
		{PARAM_HANG_SETTING,			  1,			(uint8_t*)&hang_number,							READWRITE},
        {NUM_OF_PARAMS,					  0,			NULL,											READONLY},

		//Output Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {OE_DISPENSE,                           1,          &outputEventConfigs[OE_DISPENSE - START_OE],                        READWRITE},
        {OE_LOW_BATTERY,                        1,          &outputEventConfigs[OE_LOW_BATTERY - START_OE],                     READWRITE},
        {OE_FAULT,                              1,          &outputEventConfigs[OE_FAULT - START_OE],                           READWRITE},
        {OE_PRODUCT_LOW,						1,			&outputEventConfigs[OE_PRODUCT_LOW - START_OE],                     READWRITE},
        {OE_USER_SENSOR_STATE_CHANGE,           1,          &outputEventConfigs[OE_USER_SENSOR_STATE_CHANGE - START_OE],        READWRITE},
        {OE_MOTOR_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MOTOR_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_DOOR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_DOOR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_POWERUP,                            1,          &outputEventConfigs[OE_POWERUP - START_OE],                         READWRITE},
        {OE_ENABLED_EVENT_OCCURRED,             1,          &outputEventConfigs[OE_ENABLED_EVENT_OCCURRED - START_OE],          READWRITE},
        {OE_REQUEST_TO_SEND,					1,			&outputEventConfigs[OE_REQUEST_TO_SEND - START_OE],          		READWRITE},
        {OE_MOTOR_STATE_CHANGE,          		1,          &outputEventConfigs[OE_MOTOR_STATE_CHANGE - START_OE],       		READWRITE},
        {OE_TEAR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_TEAR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_TEAR_BUMP_OCCURRED,                 1,          &outputEventConfigs[OE_TEAR_BUMP_OCCURRED - START_OE],              READWRITE},
        {OE_DELAY_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_DELAY_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_PAPER_LENGTH_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_SENSOR_RANGE_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_DISPENSE_MODE_SWITCH_STATE,         1,          &outputEventConfigs[OE_DISPENSE_MODE_SWITCH_STATE - START_OE],      READWRITE},
        {OE_MANUAL_FEED,                        1,          &outputEventConfigs[OE_MANUAL_FEED - START_OE],                     READWRITE},
        {OE_DISPENSE_TIME_UPDATE,               1,          &outputEventConfigs[OE_DISPENSE_TIME_UPDATE - START_OE],            READWRITE},
        {OE_DISPENSE_CLICK_UPDATE,              1,          &outputEventConfigs[OE_DISPENSE_CLICK_UPDATE - START_OE],           READWRITE},
        {OE_MAINT_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MAINT_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_BATTERY_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_BATTERY_FUEL_GAUGE - START_OE],              READWRITE},
        {OE_PRODUCT_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_PRODUCT_FUEL_GAUGE - START_OE],              READWRITE},
		{OE_ROLL_STUBBABLE,						1,			&outputEventConfigs[OE_ROLL_STUBBABLE - START_OE],					READWRITE},
		{OE_I2C_RESET,							1,			&outputEventConfigs[OE_I2C_RESET - START_OE],						READWRITE},
		{OE_FUEL_LEVEL,							1,			&outputEventConfigs[OE_FUEL_LEVEL - START_OE],						READWRITE},
		{OE_FUEL_CLICKS,						1,			&outputEventConfigs[OE_FUEL_CLICKS - START_OE],						READWRITE},
		{OE_LEFT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_LEFT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_MIDDLE_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_MIDDLE_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_RIGHT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_RIGHT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_HANG_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_HANG_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_BUTTON_PRESS,						1,			&outputEventConfigs[OE_BUTTON_PRESS - START_OE],					READWRITE},
        {NUM_OF_OUTPUT_EVENTS,					0,			NULL,																READONLY},
        //Input Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {IE_PERFORM_DISPENSE,              1,          &inputEventConfigs[IE_PERFORM_DISPENSE - START_IE],   		READWRITE},
        {IE_ENTER_FAULT_MODE,              1,          &inputEventConfigs[IE_ENTER_FAULT_MODE - START_IE],   		READWRITE},
        {IE_INDICATE_OUT_OF_PRODUCT,       1,          &inputEventConfigs[IE_INDICATE_OUT_OF_PRODUCT - START_IE],   READWRITE},
        {IE_EXTERNAL_HEARTBEAT_EVENT,      1,          &inputEventConfigs[IE_EXTERNAL_HEARTBEAT_EVENT - START_IE],  READWRITE},
        {IE_CLEAR_LATCHED_OUTPUTS,         1,          &inputEventConfigs[IE_CLEAR_LATCHED_OUTPUTS - START_IE],   	READWRITE},
        {IE_EXTERNAL_LOW_BATTERY,          1,          &inputEventConfigs[IE_EXTERNAL_LOW_BATTERY - START_IE],   	READWRITE},
        {IE_SET_DISPENSER_STATE,           1,          &inputEventConfigs[IE_SET_DISPENSER_STATE - START_IE],   	READWRITE},
        {IE_CLEAR_TO_SEND,			       1,		   &inputEventConfigs[IE_CLEAR_TO_SEND - START_OE],          	READWRITE},
        {IE_TEAR_BUMP,                     1,          &inputEventConfigs[IE_TEAR_BUMP - START_IE],   				READWRITE},
        {IE_MANUAL_FEED,                   1,          &inputEventConfigs[IE_MANUAL_FEED - START_IE],   			READWRITE},
		{IE_MOTOR_TEST,                    1,          &inputEventConfigs[IE_MOTOR_TEST - START_IE],   				READWRITE},
		{IE_EXPANSION_1_TEST,              1,          &inputEventConfigs[IE_EXPANSION_1_TEST - START_IE],   		READWRITE},
		{IE_EXPANSION_2_TEST,              1,          &inputEventConfigs[IE_EXPANSION_2_TEST - START_IE],   		READWRITE},
		{IE_I2C_TEST,		               1,          &inputEventConfigs[IE_I2C_TEST - START_IE],   				READWRITE},
		{NUM_OF_INPUT_EVENTS,			   0,		   NULL,														READONLY},
};
#endif	//DISP_EAGLE

#ifdef DISP_TISSUE_FUEL
#define DI_PARAM_TABLE_DEFINED
#pragma DATA_SECTION(diParamTable, ".const")
const di_param_t diParamTable[PARAM_TABLE_SIZE] =
{
        //Data Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {PARAM_SOFTWARE_REVISION,       MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.swRev[0],      READONLY},
        {PARAM_DISPINT_REVISION,        MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.disIntRev[0], 	READONLY},
        {PARAM_DISPENSER_TYPE,          MAXDATABYTES,   (uint8_t*)&nvParams.dispenserType[0],  		    RWMANFMODE},
        {PARAM_DISPENSER_SERIAL_NUMBER, MAXDATABYTES,   (uint8_t*)&nvParams.dispenserSerialNum[0], 		RWMANFMODE},
        {PARAM_DISPENSER_REVISION,      MAXDATABYTES,   (uint8_t*)&nvParams.dispenserRevision[0],  		RWMANFMODE},
        {PARAM_BATTERY_LEVEL,			  1,			(uint8_t*)&di_status.batteryRemaining,			READONLY},
		{PARAM_FAULTSTATUS,               1,            &di_status.flags.sbyte,                			READWRITE},
        {PARAM_PRODUCT_REMAINING,		  1,            (uint8_t*)&di_status.productRemaining, 			READONLY},
        {PARAM_USER_SENSOR_STATE,         1,            NULL,                							READONLY},
        {PARAM_MOTOR_SWITCH_STATE,        1,            NULL,                    						READONLY},
        {PARAM_COVER_SWITCH_STATE,        1,            NULL,                							READONLY},
        {PARAM_MOTOR_STATE,               1,            NULL,                							READONLY},
        {PARAM_NUM_DISPENSES_SINCE_RESET, 4,            NULL,     										READWRITE},
        {PARAM_TEAR_SWITCH_STATE,    	  1,            NULL,				                         	READONLY},
        {PARAM_DELAY_SETTING,          	  2,            NULL,      										READWRITE},
        {PARAM_PAPER_LENGTH_SETTING,	  1,            NULL,                  							READWRITE},
        {PARAM_SENSOR_RANGE_SETTING,   	  2,            NULL,   										READWRITE},
        {PARAM_DISPENSE_MODE_SETTING,  	  1,            NULL,							                READWRITE},
		{PARAM_CAM_SWITCH_STATE,		  1,			NULL,											READONLY},
        {PARAM_SHEET_LENGTH_MODE,   	  1,            NULL,    										READWRITE},
        {PARAM_NEW_USER_TIME,   		  1,            NULL,             								READWRITE},
        {PARAM_NUM_USERS_ADJUST,   		  2,            NULL,     										READWRITE},
        {PARAM_MAINT_SWITCH_STATE, 	 	  1,			NULL,											READONLY},
		{PARAM_LOW_PRODUCT_THRESHOLD,	  1,			NULL,											READWRITE},
		{PARAM_FUEL_GAUGE_CAL,		 	  1,			NULL,											READONLY},
		{PARAM_FUEL_GAUGE_SW_REV,		  1,			NULL,											READONLY},
		{PARAM_LEFT_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_MIDDLE_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_RIGHT_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_HANG_SETTING,			  1,			NULL,											READWRITE},
        {NUM_OF_PARAMS,					  0,			NULL,											READONLY},
		{PARAM_DISPENSER_BSL_KEY,		  MAXDATABYTES, &dispenserBslKey,								READONLY},

		//Output Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {OE_DISPENSE,                           1,          &outputEventConfigs[OE_DISPENSE - START_OE],                        READWRITE},
        {OE_LOW_BATTERY,                        1,          &outputEventConfigs[OE_LOW_BATTERY - START_OE],                     READWRITE},
        {OE_FAULT,                              1,          &outputEventConfigs[OE_FAULT - START_OE],                           READWRITE},
        {OE_PRODUCT_LOW,						1,			&outputEventConfigs[OE_PRODUCT_LOW - START_OE],                     READWRITE},
        {OE_USER_SENSOR_STATE_CHANGE,           1,          &outputEventConfigs[OE_USER_SENSOR_STATE_CHANGE - START_OE],        READWRITE},
        {OE_MOTOR_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MOTOR_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_DOOR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_DOOR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_POWERUP,                            1,          &outputEventConfigs[OE_POWERUP - START_OE],                         READWRITE},
        {OE_ENABLED_EVENT_OCCURRED,             1,          &outputEventConfigs[OE_ENABLED_EVENT_OCCURRED - START_OE],          READWRITE},
        {OE_REQUEST_TO_SEND,					1,			&outputEventConfigs[OE_REQUEST_TO_SEND - START_OE],          		READWRITE},
        {OE_MOTOR_STATE_CHANGE,          		1,          &outputEventConfigs[OE_MOTOR_STATE_CHANGE - START_OE],       		READWRITE},
        {OE_TEAR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_TEAR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_TEAR_BUMP_OCCURRED,                 1,          &outputEventConfigs[OE_TEAR_BUMP_OCCURRED - START_OE],              READWRITE},
        {OE_DELAY_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_DELAY_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_PAPER_LENGTH_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_SENSOR_RANGE_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_DISPENSE_MODE_SWITCH_STATE,         1,          &outputEventConfigs[OE_DISPENSE_MODE_SWITCH_STATE - START_OE],      READWRITE},
        {OE_MANUAL_FEED,                        1,          &outputEventConfigs[OE_MANUAL_FEED - START_OE],                     READWRITE},
        {OE_DISPENSE_TIME_UPDATE,               1,          &outputEventConfigs[OE_DISPENSE_TIME_UPDATE - START_OE],            READWRITE},
        {OE_DISPENSE_CLICK_UPDATE,              1,          &outputEventConfigs[OE_DISPENSE_CLICK_UPDATE - START_OE],           READWRITE},
        {OE_MAINT_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MAINT_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_BATTERY_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_BATTERY_FUEL_GAUGE - START_OE],              READWRITE},
        {OE_PRODUCT_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_PRODUCT_FUEL_GAUGE - START_OE],              READWRITE},
		{OE_ROLL_STUBBABLE,						1,			&outputEventConfigs[OE_ROLL_STUBBABLE - START_OE],					READWRITE},
		{OE_I2C_RESET,							1,			&outputEventConfigs[OE_I2C_RESET - START_OE],						READWRITE},
		{OE_FUEL_LEVEL,							1,			&outputEventConfigs[OE_FUEL_LEVEL - START_OE],						READWRITE},
		{OE_FUEL_CLICKS,						1,			&outputEventConfigs[OE_FUEL_CLICKS - START_OE],						READWRITE},
		{OE_LEFT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_LEFT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_MIDDLE_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_MIDDLE_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_RIGHT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_RIGHT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_HANG_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_HANG_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_BUTTON_PRESS,						1,			&outputEventConfigs[OE_BUTTON_PRESS - START_OE],					READWRITE},
        {NUM_OF_OUTPUT_EVENTS,					0,			NULL,																READONLY},
        //Input Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {IE_PERFORM_DISPENSE,              1,          &inputEventConfigs[IE_PERFORM_DISPENSE - START_IE],   		READWRITE},
        {IE_ENTER_FAULT_MODE,              1,          &inputEventConfigs[IE_ENTER_FAULT_MODE - START_IE],   		READWRITE},
        {IE_INDICATE_OUT_OF_PRODUCT,       1,          &inputEventConfigs[IE_INDICATE_OUT_OF_PRODUCT - START_IE],   READWRITE},
        {IE_EXTERNAL_HEARTBEAT_EVENT,      1,          &inputEventConfigs[IE_EXTERNAL_HEARTBEAT_EVENT - START_IE],  READWRITE},
        {IE_CLEAR_LATCHED_OUTPUTS,         1,          &inputEventConfigs[IE_CLEAR_LATCHED_OUTPUTS - START_IE],   	READWRITE},
        {IE_EXTERNAL_LOW_BATTERY,          1,          &inputEventConfigs[IE_EXTERNAL_LOW_BATTERY - START_IE],   	READWRITE},
        {IE_SET_DISPENSER_STATE,           1,          &inputEventConfigs[IE_SET_DISPENSER_STATE - START_IE],   	READWRITE},
        {IE_CLEAR_TO_SEND,			       1,		   &inputEventConfigs[IE_CLEAR_TO_SEND - START_OE],          	READWRITE},
        {IE_TEAR_BUMP,                     1,          &inputEventConfigs[IE_TEAR_BUMP - START_IE],   				READWRITE},
        {IE_MANUAL_FEED,                   1,          &inputEventConfigs[IE_MANUAL_FEED - START_IE],   			READWRITE},
		{IE_MOTOR_TEST,                    1,          &inputEventConfigs[IE_MOTOR_TEST - START_IE],   				READWRITE},
		{IE_EXPANSION_1_TEST,              1,          &inputEventConfigs[IE_EXPANSION_1_TEST - START_IE],   		READWRITE},
		{IE_EXPANSION_2_TEST,              1,          &inputEventConfigs[IE_EXPANSION_2_TEST - START_IE],   		READWRITE},
		{IE_I2C_TEST,		               1,          &inputEventConfigs[IE_I2C_TEST - START_IE],   				READWRITE},
		{NUM_OF_INPUT_EVENTS,			   0,		   NULL,														READONLY},
};
#endif	//DISP_TISSUE_FUEL

#ifdef DISP_AIRCARE
#define DI_PARAM_TABLE_DEFINED
#pragma DATA_SECTION(diParamTable, ".const")
const di_param_t diParamTable[PARAM_TABLE_SIZE] =
{
        //Data Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {PARAM_SOFTWARE_REVISION,       MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.swRev[0],      READONLY},
        {PARAM_DISPINT_REVISION,        MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.disIntRev[0], 	READONLY},
        {PARAM_DISPENSER_TYPE,          MAXDATABYTES,   (uint8_t*)&nvParams.dispenserType[0],  		    RWMANFMODE},
        {PARAM_DISPENSER_SERIAL_NUMBER, MAXDATABYTES,   (uint8_t*)&nvParams.dispenserSerialNum[0], 		RWMANFMODE},
        {PARAM_DISPENSER_REVISION,      MAXDATABYTES,   (uint8_t*)&nvParams.dispenserRevision[0],  		RWMANFMODE},
        {PARAM_BATTERY_LEVEL,			  1,			(uint8_t*)&di_status.batteryRemaining,			READONLY},
		{PARAM_FAULTSTATUS,               1,            &di_status.flags.sbyte,                			READWRITE},
        {PARAM_PRODUCT_REMAINING,		  1,            (uint8_t*)&di_status.productRemaining, 			READONLY},
        {PARAM_USER_SENSOR_STATE,         1,            NULL,                							READONLY},
        {PARAM_MOTOR_SWITCH_STATE,        1,            NULL,                    						READONLY},
        {PARAM_COVER_SWITCH_STATE,        1,            NULL,                							READONLY},
        {PARAM_MOTOR_STATE,               1,            NULL,                							READONLY},
        {PARAM_NUM_DISPENSES_SINCE_RESET, 4,            NULL,     										READWRITE},
        {PARAM_TEAR_SWITCH_STATE,    	  1,            NULL,				                         	READONLY},
        {PARAM_DELAY_SETTING,          	  2,            NULL,      										READWRITE},
        {PARAM_PAPER_LENGTH_SETTING,	  1,            NULL,                  							READWRITE},
        {PARAM_SENSOR_RANGE_SETTING,   	  2,            NULL,   										READWRITE},
        {PARAM_DISPENSE_MODE_SETTING,  	  1,            NULL,							                READWRITE},
		{PARAM_CAM_SWITCH_STATE,		  1,			NULL,											READONLY},
        {PARAM_SHEET_LENGTH_MODE,   	  1,            NULL,    										READWRITE},
        {PARAM_NEW_USER_TIME,   		  1,            NULL,             								READWRITE},
        {PARAM_NUM_USERS_ADJUST,   		  2,            NULL,     										READWRITE},
        {PARAM_MAINT_SWITCH_STATE, 	 	  1,			NULL,											READONLY},
		{PARAM_LOW_PRODUCT_THRESHOLD,	  1,			NULL,											READWRITE},
		{PARAM_FUEL_GAUGE_CAL,		 	  1,			NULL,											READONLY},
		{PARAM_FUEL_GAUGE_SW_REV,		  1,			NULL,											READONLY},
		{PARAM_LEFT_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_MIDDLE_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_RIGHT_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_HANG_SETTING,			  1,			NULL,											READWRITE},
        {NUM_OF_PARAMS,					  0,			NULL,											READONLY},
		{PARAM_DISPENSER_BSL_KEY,		  MAXDATABYTES, &dispenserBslKey,								READONLY},

		//Output Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {OE_DISPENSE,                           1,          &outputEventConfigs[OE_DISPENSE - START_OE],                        READWRITE},
        {OE_LOW_BATTERY,                        1,          &outputEventConfigs[OE_LOW_BATTERY - START_OE],                     READWRITE},
        {OE_FAULT,                              1,          &outputEventConfigs[OE_FAULT - START_OE],                           READWRITE},
        {OE_PRODUCT_LOW,						1,			&outputEventConfigs[OE_PRODUCT_LOW - START_OE],                     READWRITE},
        {OE_USER_SENSOR_STATE_CHANGE,           1,          &outputEventConfigs[OE_USER_SENSOR_STATE_CHANGE - START_OE],        READWRITE},
        {OE_MOTOR_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MOTOR_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_DOOR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_DOOR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_POWERUP,                            1,          &outputEventConfigs[OE_POWERUP - START_OE],                         READWRITE},
        {OE_ENABLED_EVENT_OCCURRED,             1,          &outputEventConfigs[OE_ENABLED_EVENT_OCCURRED - START_OE],          READWRITE},
        {OE_REQUEST_TO_SEND,					1,			&outputEventConfigs[OE_REQUEST_TO_SEND - START_OE],          		READWRITE},
        {OE_MOTOR_STATE_CHANGE,          		1,          &outputEventConfigs[OE_MOTOR_STATE_CHANGE - START_OE],       		READWRITE},
        {OE_TEAR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_TEAR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_TEAR_BUMP_OCCURRED,                 1,          &outputEventConfigs[OE_TEAR_BUMP_OCCURRED - START_OE],              READWRITE},
        {OE_DELAY_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_DELAY_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_PAPER_LENGTH_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_SENSOR_RANGE_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_DISPENSE_MODE_SWITCH_STATE,         1,          &outputEventConfigs[OE_DISPENSE_MODE_SWITCH_STATE - START_OE],      READWRITE},
        {OE_MANUAL_FEED,                        1,          &outputEventConfigs[OE_MANUAL_FEED - START_OE],                     READWRITE},
        {OE_DISPENSE_TIME_UPDATE,               1,          &outputEventConfigs[OE_DISPENSE_TIME_UPDATE - START_OE],            READWRITE},
        {OE_DISPENSE_CLICK_UPDATE,              1,          &outputEventConfigs[OE_DISPENSE_CLICK_UPDATE - START_OE],           READWRITE},
        {OE_MAINT_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MAINT_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_BATTERY_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_BATTERY_FUEL_GAUGE - START_OE],              READWRITE},
        {OE_PRODUCT_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_PRODUCT_FUEL_GAUGE - START_OE],              READWRITE},
		{OE_ROLL_STUBBABLE,						1,			&outputEventConfigs[OE_ROLL_STUBBABLE - START_OE],					READWRITE},
		{OE_I2C_RESET,							1,			&outputEventConfigs[OE_I2C_RESET - START_OE],						READWRITE},
		{OE_FUEL_LEVEL,							1,			&outputEventConfigs[OE_FUEL_LEVEL - START_OE],						READWRITE},
		{OE_FUEL_CLICKS,						1,			&outputEventConfigs[OE_FUEL_CLICKS - START_OE],						READWRITE},
		{OE_LEFT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_LEFT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_MIDDLE_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_MIDDLE_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_RIGHT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_RIGHT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_HANG_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_HANG_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_BUTTON_PRESS,						1,			&outputEventConfigs[OE_BUTTON_PRESS - START_OE],					READWRITE},
        {NUM_OF_OUTPUT_EVENTS,					0,			NULL,																READONLY},
        //Input Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {IE_PERFORM_DISPENSE,              1,          &inputEventConfigs[IE_PERFORM_DISPENSE - START_IE],   		READWRITE},
        {IE_ENTER_FAULT_MODE,              1,          &inputEventConfigs[IE_ENTER_FAULT_MODE - START_IE],   		READWRITE},
        {IE_INDICATE_OUT_OF_PRODUCT,       1,          &inputEventConfigs[IE_INDICATE_OUT_OF_PRODUCT - START_IE],   READWRITE},
        {IE_EXTERNAL_HEARTBEAT_EVENT,      1,          &inputEventConfigs[IE_EXTERNAL_HEARTBEAT_EVENT - START_IE],  READWRITE},
        {IE_CLEAR_LATCHED_OUTPUTS,         1,          &inputEventConfigs[IE_CLEAR_LATCHED_OUTPUTS - START_IE],   	READWRITE},
        {IE_EXTERNAL_LOW_BATTERY,          1,          &inputEventConfigs[IE_EXTERNAL_LOW_BATTERY - START_IE],   	READWRITE},
        {IE_SET_DISPENSER_STATE,           1,          &inputEventConfigs[IE_SET_DISPENSER_STATE - START_IE],   	READWRITE},
        {IE_CLEAR_TO_SEND,			       1,		   &inputEventConfigs[IE_CLEAR_TO_SEND - START_OE],          	READWRITE},
        {IE_TEAR_BUMP,                     1,          &inputEventConfigs[IE_TEAR_BUMP - START_IE],   				READWRITE},
        {IE_MANUAL_FEED,                   1,          &inputEventConfigs[IE_MANUAL_FEED - START_IE],   			READWRITE},
		{IE_MOTOR_TEST,                    1,          &inputEventConfigs[IE_MOTOR_TEST - START_IE],   				READWRITE},
		{IE_EXPANSION_1_TEST,              1,          &inputEventConfigs[IE_EXPANSION_1_TEST - START_IE],   		READWRITE},
		{IE_EXPANSION_2_TEST,              1,          &inputEventConfigs[IE_EXPANSION_2_TEST - START_IE],   		READWRITE},
		{IE_I2C_TEST,		               1,          &inputEventConfigs[IE_I2C_TEST - START_IE],   				READWRITE},
		{NUM_OF_INPUT_EVENTS,			   0,		   NULL,														READONLY},
};
#endif	//DISP_AIRCARE

#ifdef DISP_COMPACT_VERTICAL_SLED
#define DI_PARAM_TABLE_DEFINED
#pragma DATA_SECTION(diParamTable, ".const")
const di_param_t diParamTable[PARAM_TABLE_SIZE] =
{
        //Data Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {PARAM_SOFTWARE_REVISION,       MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.swRev[0],      READONLY},
        {PARAM_DISPINT_REVISION,        MAXDATABYTES,   (uint8_t*)&fileHeader.rONvParams.disIntRev[0], 	READONLY},
        {PARAM_DISPENSER_TYPE,          MAXDATABYTES,   (uint8_t*)&nvParams.dispenserType[0],  		    RWMANFMODE},
        {PARAM_DISPENSER_SERIAL_NUMBER, MAXDATABYTES,   (uint8_t*)&nvParams.dispenserSerialNum[0], 		RWMANFMODE},
        {PARAM_DISPENSER_REVISION,      MAXDATABYTES,   (uint8_t*)&nvParams.dispenserRevision[0],  		RWMANFMODE},
        {PARAM_BATTERY_LEVEL,			  1,			(uint8_t*)&di_status.batteryRemaining,			READONLY},
		{PARAM_FAULTSTATUS,               1,            &di_status.flags.sbyte,                			READWRITE},
        {PARAM_PRODUCT_REMAINING,		  1,            (uint8_t*)&di_status.productRemaining, 			READONLY},
        {PARAM_USER_SENSOR_STATE,         1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_MOTOR_SWITCH_STATE,        1,            (uint8_t*)&P2IN,                    			READONLY},
        {PARAM_COVER_SWITCH_STATE,        1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_MOTOR_STATE,               1,            &di_status.flags.sbyte,                			READONLY},
        {PARAM_NUM_DISPENSES_SINCE_RESET, 4,            (uint8_t*)&di_status.numDispenses,     			READWRITE},
        {PARAM_TEAR_SWITCH_STATE,    	  1,            NULL,				                         	READONLY},
        {PARAM_DELAY_SETTING,          	  2,            NULL,											READWRITE},
        {PARAM_PAPER_LENGTH_SETTING,	  1,            NULL, 						                    READWRITE},
        {PARAM_SENSOR_RANGE_SETTING,   	  2,            NULL, 										    READWRITE},
        {PARAM_DISPENSE_MODE_SETTING,  	  1,            NULL,						                    READWRITE},
		{PARAM_CAM_SWITCH_STATE,		  1,			NULL,											READONLY},
        {PARAM_SHEET_LENGTH_MODE,   	  1,            NULL,   										READWRITE},
        {PARAM_NEW_USER_TIME,   		  1,            NULL,            								READWRITE},
        {PARAM_NUM_USERS_ADJUST,   		  2,            NULL,   									    READWRITE},
		{PARAM_MAINT_SWITCH_STATE,		  1,			(uint8_t*)&maintPressed,						READONLY},
		{PARAM_LOW_PRODUCT_THRESHOLD,	  1,			NULL,											READWRITE},
		{PARAM_FUEL_GAUGE_CAL,		 	  1,			NULL,											READONLY},
		{PARAM_FUEL_GAUGE_SW_REV,		  1,			NULL,											READONLY},
		{PARAM_LEFT_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_MIDDLE_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_RIGHT_SETTING,			  1,			NULL,											READWRITE},
		{PARAM_HANG_SETTING,			  1,			NULL,											READWRITE},
        {NUM_OF_PARAMS,					  0,			NULL,											READONLY},
		{PARAM_DISPENSER_BSL_KEY,		  MAXDATABYTES, &dispenserBslKey,								READONLY},

		//Output Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {OE_DISPENSE,                           1,          &outputEventConfigs[OE_DISPENSE - START_OE],                        READWRITE},
        {OE_LOW_BATTERY,                        1,          &outputEventConfigs[OE_LOW_BATTERY - START_OE],                     READWRITE},
        {OE_FAULT,                              1,          &outputEventConfigs[OE_FAULT - START_OE],                           READWRITE},
        {OE_PRODUCT_LOW,						1,			&outputEventConfigs[OE_PRODUCT_LOW - START_OE],                     READWRITE},
        {OE_USER_SENSOR_STATE_CHANGE,           1,          &outputEventConfigs[OE_USER_SENSOR_STATE_CHANGE - START_OE],        READWRITE},
        {OE_MOTOR_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MOTOR_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_DOOR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_DOOR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_POWERUP,                            1,          &outputEventConfigs[OE_POWERUP - START_OE],                         READWRITE},
        {OE_ENABLED_EVENT_OCCURRED,             1,          &outputEventConfigs[OE_ENABLED_EVENT_OCCURRED - START_OE],          READWRITE},
        {OE_REQUEST_TO_SEND,					1,			&outputEventConfigs[OE_REQUEST_TO_SEND - START_OE],          		READWRITE},
        {OE_MOTOR_STATE_CHANGE,          		1,          &outputEventConfigs[OE_MOTOR_STATE_CHANGE - START_OE],       		READWRITE},
        {OE_TEAR_SWITCH_STATE_CHANGE,           1,          &outputEventConfigs[OE_TEAR_SWITCH_STATE_CHANGE - START_OE],        READWRITE},
        {OE_TEAR_BUMP_OCCURRED,                 1,          &outputEventConfigs[OE_TEAR_BUMP_OCCURRED - START_OE],              READWRITE},
        {OE_DELAY_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_DELAY_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_PAPER_LENGTH_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_PAPER_LENGTH_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_SENSOR_RANGE_SWITCH_STATE_CHANGE,   1,          &outputEventConfigs[OE_SENSOR_RANGE_SWITCH_STATE_CHANGE - START_OE],READWRITE},
        {OE_DISPENSE_MODE_SWITCH_STATE,         1,          &outputEventConfigs[OE_DISPENSE_MODE_SWITCH_STATE - START_OE],      READWRITE},
        {OE_MANUAL_FEED,                        1,          &outputEventConfigs[OE_MANUAL_FEED - START_OE],                     READWRITE},
        {OE_DISPENSE_TIME_UPDATE,               1,          &outputEventConfigs[OE_DISPENSE_TIME_UPDATE - START_OE],            READWRITE},
        {OE_DISPENSE_CLICK_UPDATE,              1,          &outputEventConfigs[OE_DISPENSE_CLICK_UPDATE - START_OE],           READWRITE},
        {OE_MAINT_SWITCH_STATE_CHANGE,          1,          &outputEventConfigs[OE_MAINT_SWITCH_STATE_CHANGE - START_OE],       READWRITE},
        {OE_BATTERY_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_BATTERY_FUEL_GAUGE - START_OE],              READWRITE},
        {OE_PRODUCT_FUEL_GAUGE,                 1,          &outputEventConfigs[OE_PRODUCT_FUEL_GAUGE - START_OE],              READWRITE},
		{OE_ROLL_STUBBABLE,						1,			&outputEventConfigs[OE_ROLL_STUBBABLE - START_OE],					READWRITE},
		{OE_I2C_RESET,							1,			&outputEventConfigs[OE_I2C_RESET - START_OE],						READWRITE},
		{OE_FUEL_LEVEL,							1,			&outputEventConfigs[OE_FUEL_LEVEL - START_OE],						READWRITE},
		{OE_FUEL_CLICKS,						1,			&outputEventConfigs[OE_FUEL_CLICKS - START_OE],						READWRITE},
		{OE_LEFT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_LEFT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_MIDDLE_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_MIDDLE_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_RIGHT_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_RIGHT_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_HANG_NUMBER_STATE_CHANGE,			1,			&outputEventConfigs[OE_HANG_NUMBER_STATE_CHANGE - START_OE],		READWRITE},
		{OE_BUTTON_PRESS,						1,			&outputEventConfigs[OE_BUTTON_PRESS - START_OE],					READWRITE},
        {NUM_OF_OUTPUT_EVENTS,					0,			NULL,																READONLY},
        //Input Event Config Parameters.
        //These parameters need to be sorted in the order that they appear in the Parameter Enumeration

        {IE_PERFORM_DISPENSE,              1,          &inputEventConfigs[IE_PERFORM_DISPENSE - START_IE],   		READWRITE},
        {IE_ENTER_FAULT_MODE,              1,          &inputEventConfigs[IE_ENTER_FAULT_MODE - START_IE],   		READWRITE},
        {IE_INDICATE_OUT_OF_PRODUCT,       1,          &inputEventConfigs[IE_INDICATE_OUT_OF_PRODUCT - START_IE],   READWRITE},
        {IE_EXTERNAL_HEARTBEAT_EVENT,      1,          &inputEventConfigs[IE_EXTERNAL_HEARTBEAT_EVENT - START_IE],  READWRITE},
        {IE_CLEAR_LATCHED_OUTPUTS,         1,          &inputEventConfigs[IE_CLEAR_LATCHED_OUTPUTS - START_IE],   	READWRITE},
        {IE_EXTERNAL_LOW_BATTERY,          1,          &inputEventConfigs[IE_EXTERNAL_LOW_BATTERY - START_IE],   	READWRITE},
        {IE_SET_DISPENSER_STATE,           1,          &inputEventConfigs[IE_SET_DISPENSER_STATE - START_IE],   	READWRITE},
        {IE_CLEAR_TO_SEND,			       1,		   &inputEventConfigs[IE_CLEAR_TO_SEND - START_OE],          	READWRITE},
        {IE_TEAR_BUMP,                     1,          &inputEventConfigs[IE_TEAR_BUMP - START_IE],   				READWRITE},
        {IE_MANUAL_FEED,                   1,          &inputEventConfigs[IE_MANUAL_FEED - START_IE],   			READWRITE},
		{IE_MOTOR_TEST,                    1,          &inputEventConfigs[IE_MOTOR_TEST - START_IE],   				READWRITE},
		{IE_EXPANSION_1_TEST,              1,          &inputEventConfigs[IE_EXPANSION_1_TEST - START_IE],   		READWRITE},
		{IE_EXPANSION_2_TEST,              1,          &inputEventConfigs[IE_EXPANSION_2_TEST - START_IE],   		READWRITE},
		{IE_I2C_TEST,		               1,          &inputEventConfigs[IE_I2C_TEST - START_IE],   				READWRITE},
		{NUM_OF_INPUT_EVENTS,			   0,		   NULL,														READONLY},
};
#endif	//DISP_COMPACT_VERTICAL_SLED

#ifndef DI_PARAM_TABLE_DEFINED
#error Need to define diPramTable for the dispenser that is defined.
#endif	//DI_PARAM_TABLE_DEFINED


int16_t parameterCompare(const void * a, const void * b)
{
    const di_param_t *p1 = a;
    const di_param_t *p2 = b;
    return ((p1->code) - (p2->code));
}

//TODO: Add Meaningful Comment
void readpram(di_paramKey_t pKey)
{
    const di_param_t * pram;
    uint8_t i;
    //Get the parameter from the paramTable
    pram = GetParam(pKey);
    //Check if the parameter was valid
    if (pram != NULL)
    {
        //Add pram code to tx buffer
        txbuf[CDATA_START] = pram->code;

        //Fill Tx buffer with the parameter data. Note that the MSP430 is little endian,
        //and network byte order is big endian so multibyte data needs to be byte reversed
		for (i = 0; i < pram->numBytes; i++)
		{
			//Only byte reverse numerical data, not strings
			//so this is a check if parameter is not a string
			if (pram->numBytes < MAXDATABYTES)
			{
				txbuf[PDATA_START + i] = (uint8_t)pram->paramData[pram->numBytes - i -1];
			}
			else //string parameter so just store in order received
			{
				txbuf[PDATA_START + i] = (uint8_t)pram->paramData[i];
				//For string parameters, only go until the first NULL character, except for
				//PARAM_DISPENSER_BSL_KEY which is a special case
				if (pram->paramData[i] == 0x00 &&
				    pram->code != PARAM_DISPENSER_BSL_KEY)
				{
					break;
				}
			}
		}
        buildframe(i + 1, CM_READ_PARAM_RESPONSE);
    }
}

//TODO: Add Meaningful Comment
void writepram(di_command_t * command)
{
    const di_param_t * pram;
    uint8_t i;

    //First check if this is a config parameter because they are handled differently
    if (command->paramKey >= START_OE)
    {
    	//Check if this is an input event config
    	if (command->paramKey >= START_IE)
		{
			configureInputEvent((di_paramKey_t)command->paramKey, command->paramData[0]);
		}
    	else //output event config
    	{
    		configureOutputEvent((di_paramKey_t)command->paramKey, command->paramData[0]);
    	}
    }
    //Get the parameter from the paramTable
    pram = GetParam(command->paramKey);

    //Check if the parameter was valid
    if (pram != NULL)
    {
        //write the parameter based on the access type it is
        if (pram->access == READWRITE)
        {
#ifdef DISP_UNIFIED_CHASSIS
        	//This was added as a settings lockout if a setting is pushed to the device
        	disableSettings = TRUE;
#endif	//DISP_UNIFIED_CHASSIS

        	//Copy in the parameter data. Note that the MSP430 is little endian,
			//and network byte order is big endian so multibyte data needs to be byte reversed
			for (i = 0; i < command->pDataNumBytes; i++)
			{
				//Only byte reverse numerical data, not strings
				//so this is a check if parameter is not a string
				if (pram->numBytes < MAXDATABYTES)
				{
					pram->paramData[command->pDataNumBytes - i -1] = command->paramData[i];         // copy value to flash little endian
				}
				else //string parameter so just store in order received
				{
					pram->paramData[i] = command->paramData[i];
				}
			}
        }
        else if (pram->access == RWMANFMODE)
        {
            nvParamWrite(command, pram);
        }
    }
}

static void nvParamWrite(di_command_t * command, const di_param_t *pram)
{
#ifndef DISP_EAGLE
    uint8_t i;
    //Check if we are in MFG_MODE, which means we need to erase the flash segment
    //the nVParams are in in order to program them. This also has the side effect
    //that nvParams.nVMode will no longer be NV_MFG_MODE so that the parameters can
    //only be written to once after that.
    if (nvParams.nvMode == NV_MFG_MODE)
    {
        nvParamFlashErase((uint8_t *)&nvParams.nvMode);
    }
    //Check if the parameter can still be programmed. This is only allowed if the
    //first and last byte of the parameter is 0xFF (Meaning parameter is in erased state).
    //Once it is programmed, the parameter essentially becomes read only until the firmware is programmed again.
    if (pram->paramData[0] == 0xFF &&
        pram->paramData[pram->numBytes - 1] == 0xFF)
    {
        //disable watchdog
        WDTCTL = WDTPW + WDTHOLD;

        FCTL2 = FWKEY + FSSEL_2 + FLASH_CLOCK_DIV;
        FCTL3 = FWKEY;                       // Clear Lock bit
        FCTL1 = FWKEY + WRT;                 // Set WRT bit for write operation

        //Copy in the parameter data. Note that the MSP430 is little endian,
        //and network byte order is big endian so multibyte data needs to be byte reversed
        for (i = 0; i < command->pDataNumBytes; i++)
        {
        	//Only byte reverse numerical data, not strings
        	//so this is a check if parameter is not a string
        	if (pram->numBytes < MAXDATABYTES)
        	{
        		pram->paramData[command->pDataNumBytes - i -1] = command->paramData[i];         // copy value to flash little endian
        	}
        	else //string parameter so just store in order received
        	{
        		pram->paramData[i] = command->paramData[i];
        	}
        }

        //if the data received from dispenser interface is less than the
        //size of the parameter, which can occur for string types,
        //terminate parameter with a NULL to enforce NULL terminated string.
        //This also disables subsequent writes to the parameter.
        if (command->pDataNumBytes < pram->numBytes)
        {
            pram->paramData[command->pDataNumBytes] = 0;
            pram->paramData[MAXDATABYTES - 1] = 0;
        }
        FCTL1 = FWKEY;                        // Clear WRT bit
        FCTL3 = FWKEY + LOCK;                 // Set LOCK bit
#ifdef USE_WDT
          //enable watchdog
          WDTCTL = RESET_WDT;
#endif	//USE_WDT
    }
#endif	//DISP_EAGLE
}
static void nvParamFlashErase(uint8_t *addr)
{
#ifndef DISP_EAGLE
  //disable watchdog
  WDTCTL = WDTPW + WDTHOLD;
  //Wait until flash is not busy
  while(BUSY & FCTL3);

  //Set up flash programming clock
  FCTL2 = FWKEY + FSSEL_2 + FLASH_CLOCK_DIV;
  FCTL1 = FWKEY + ERASE;               // Set Erase bit
  FCTL3 = FWKEY;                       // Clear Lock bit
  *addr = 0;                           // Dummy write to erase Flash segment
  while(BUSY & FCTL3);                 // Check if Flash being used
  FCTL1 = FWKEY;                       // Clear WRT bit
  FCTL3 = FWKEY + LOCK;                // Set LOCK bit

#ifdef USE_WDT
  //enable watchdog
  WDTCTL = RESET_WDT;
#endif	//USE_WDT
#endif	//DISP_EAGLE
}

const di_param_t * GetParam(di_paramKey_t pKey)
{
	uint8_t first = 0;
	uint8_t last = PARAM_TABLE_SIZE - 1;
	uint8_t mid = (first+last)>>1;
	const di_param_t * retVal = NULL;

   while( first <= last )
   {
	  if (diParamTable[mid].code < pKey )
	  {
		 first = mid + 1;
	  }
	  else if (diParamTable[mid].code == pKey )
	  {
		 retVal = &diParamTable[mid];
		 break;
	  }
	  else
	  {
		 last = mid - 1;
	  }

	  mid = (first + last)>>1;
   }

   return retVal;
}

void DiUpdateParams()
{
#ifdef DISP_ENMOTION_CLASSIC
    extern DispenseInfo         dispenseInfo;
    static uint8_t currentManFeedFlag = 0;

    if (dispenseInfo.ManualFeedFlag != currentManFeedFlag)
    {
        sendEvent(OE_MANUAL_FEED, dispenseInfo.ManualFeedFlag);
        currentManFeedFlag = dispenseInfo.ManualFeedFlag;
    }

#endif //DISP_ENMOTION_CLASSIC

#ifdef DISP_ENMOTION_SOAP
	//Update cam switch status
	if (CAM_SWITCH_STATE(CAM_SWITCH_CLOSED) != di_status.flags.sbits.motorSwitch)
	{
		di_status.flags.sbits.motorSwitch = CAM_SWITCH_STATE(CAM_SWITCH_CLOSED);
		sendEvent(OE_MOTOR_SWITCH_STATE_CHANGE, di_status.flags.sbits.motorSwitch);
	}

	//Update hand sensor status
	//This is handled in the main loop when the IR sensor is activated

	//Update Maintenance Switch status
	if (MAINT_SWITCH_STATE(MAINT_SWITCH_CLOSED) != di_status.flags.sbits.maintSwitch)
	{
		di_status.flags.sbits.maintSwitch = MAINT_SWITCH_STATE(MAINT_SWITCH_CLOSED);
		sendEvent(OE_MAINT_SWITCH_STATE_CHANGE, di_status.flags.sbits.maintSwitch);
	}
#endif //DISP_ENMOTION_SOAP

#ifdef DISP_UNIFIED_CHASSIS
	STROBE_ON;
	delayMs(STROBE_SETTLE_DELAY);
	if(IS_CAM_PRESSED){
		camPressed = TRUE;
	} else {
		camPressed = FALSE;
	}

	if(IS_TEAR_PRESSED){
		tearPressed = TRUE;
	} else {
		tearPressed = FALSE;
	}

	STROBE_OFF;
#endif	//DISP_UNIFIED_CHASSIS
}
