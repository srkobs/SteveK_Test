/*
 * di_params.h
 *
 *  Created on: Feb 13, 2014
 *      Author: Dave Murphy
 *
 * © 2014. Confidential and proprietary information of Georgia-Pacific Consumer Products LP. All rights reserved.
 *
 */

#ifndef DI_PARAMS_H_
#define DI_PARAMS_H_

#ifndef NULL
#define NULL (void*)0
#endif
/********** STATUS FLAGS **********/
/****** Motor Status *****/
#define MOTOR_IDLE								0
#define	MOTOR_DISPENSING						1

/***** Cover Status ******/
#define COVER_OPEN								0
#define	COVER_CLOSED							1

/***** Maintenance Switch Status ******/
#define MAINT_ACTIVE							0
#define	MAINT_INACTIVE							1

/***** Product Presence Status ******/
#define PRODUCT_MISSING							0
#define PRODUCT_PRESENT							1

typedef enum
{
    NO_FAULT,
    TEAR_BAR_FAULT,
    MOTOR_FAULT,
    TEAR_BAR_FAULT_CLEAR = 0x81,
    MOTOR_FAULT_CLEAR
}FAULT_CODE;
// States the battery can be in.
typedef enum
{
    BATTERY_STARTUP,
	BATTERY_NORMAL,
	BATTERY_LOW,
	BATTERY_SHUTDOWN,
	NUMBER_OF_BATTERY_STATES
}BATTERY_STATE;

typedef enum
{
	USER_SENSOR_INACTIVE,
	USER_SENSOR_ACTIVE,
	USER_SENSOR_OCCLUDED
}userSensorStates_t;

/// board/module identification structure
typedef struct
{
	uint8_t vendor_id;					///< vendor ID code
	uint8_t module_id;					///< module/board ID code
}di_moduleId_t;


#ifdef DISP_ENMOTION_CLASSIC
typedef struct
{

     /****** 1 byte ******/
         //Unused
         uint8_t : 1;

         //  Motor Status:
         //  0-No user detected   1-User detected
         uint8_t userSensor     :1;

         //  Motor Status:
         //  0-Not Running   1-Running
         uint8_t motor          :   1;

         //  Tear Bar Position:
         //  0-Activated     1-Home
         uint8_t TearBarPosition     :   1;

         //  Cover Door Status
         //  0-Cover Open    1-Cover Closed
         //
         uint8_t doorswitch          :   1;


         //  Tear Bar Exception
         //  Behavior is slightly different between On Demand and Hang modes.
         //
         //  Each mode will trigger this flag when the requisite conditions
         //  are met.
         //
         //  0-No Exception  1-Exception
         uint8_t TearBarException        :   1;


         //  This flag is turned on if it is determined
         //  we have a low battery
         //  1-TRUE          0-FALSE
         uint8_t LowBatteryState     :   1;


         //  Reports whether a motor fault has occured
         //  Set by the individual dispense modes
         //  Read by the LED logic to blink or not
         //  0 - No Motor Fault
         //  1 - Motor Presently in Fault state
         uint8_t     MotorFault          :   1;
} di_statusBits_t;
#endif //DISP_ENMOTION_CLASSIC

#if defined(DISP_ENMOTION_SOAP_CAP)
typedef struct
{
	 // XXXXX
	 // 0-		1-
	 uint8_t : 1;

	 // Product presence detection status
	 uint8_t productPresent : 1;

	 //  User Status:
	 //  0-No user detected   1-User detected 2-Occluded
	 userSensorStates_t userSensor     :2;

	 //  Motor Status:
	 //  0-Not Running   1-Running
	 uint8_t motor          :   1;

	 //  Cover Door Status
	 //  0-Cover Open    1-Cover Closed
	 uint8_t doorswitch          :   1;

	 //  Maintenance Switch Status
	 //  0-Switch Open    1-Switch Closed
	 uint8_t maintSwitch          :   1;

	 //  Motor Switch Status
	 //  0-Switch Open    1-Switch Closed
	 uint8_t motorSwitch          :   1;
} di_statusBits_t;
#endif //DISP_ENMOTION_SOAP_CAP

#ifdef DISP_ENMOTION_SOAP
typedef struct
{
	 //Unused bits to make a full byte
	 uint8_t : 2;

	 //  User Status:
	 //  0-No user detected   1-User detected 2-Occluded
	 userSensorStates_t userSensor     :2;

	 //  Motor Status:
	 //  0-Not Running   1-Running
	 uint8_t motor          :   1;

	 //  Cover Door Status
	 //  0-Cover Open    1-Cover Closed
	 uint8_t doorswitch          :   1;

	 //  Maintenance Switch Status
	 //  0-Switch Open    1-Switch Closed
	 uint8_t maintSwitch          :   1;

	 //  Motor Switch Status
	 //  0-Switch Open    1-Switch Closed
	 uint8_t motorSwitch          :   1;
} di_statusBits_t;
#endif //DISP_ENMOTION_SOAP

#if defined (DISP_SAFEHAVEN_SOAP)
typedef struct
{
	 // HHC badge detected state
	 uint8_t badgeDetected : 1;

	 // Product presence detection status
	 uint8_t productPresent : 1;

	 //  User Status:
	 //  0-No user detected   1-User detected 2-Occluded
	 userSensorStates_t userSensor     :2;

	 //  Motor Status:
	 //  0-Not Running   1-Running
	 uint8_t motor          :   1;

	 //  Cover Door Status
	 //  0-Cover Open    1-Cover Closed
	 uint8_t doorswitch          :   1;

	 //  Maintenance Switch Status
	 //  0-Switch Open    1-Switch Closed
	 uint8_t maintSwitch          :   1;

	 //  Motor Switch Status
	 //  0-Switch Open    1-Switch Closed
	 uint8_t motorSwitch          :   1;
} di_statusBits_t;
#endif //DISP_SAFEHAVEN_SOAP

typedef struct
{
    union
    {
        di_statusBits_t sbits;
        uint8_t sbyte;
    }flags;

	/****** 1 byte ******/
 	uint8_t activegpio;			// Appears to only be Range: 0 to 7, but will keep as full uint8_t

 	/*** UNGROUPED ***/
 	///	Number of times the product has dispensed
 	uint32_t numDispenses;

 	///	Number of times the cover has been opened (more technically, closed)
 	uint16_t numCoverOpens;

 	/// Current battery state.
 	BATTERY_STATE batteryState;
 	/// % battery remaining.
 	uint8_t batteryRemaining;
 	///% of product remaining
 	int8_t productRemaining;			// needs to be signed so we can send negative values
 	di_dispenserState_t dispState;
 	/// Product ID configuration bytes (\see product ID bit defs for data details)
 	uint32_t productID;
 	/// low threshold %product level
 	uint8_t lowProductThresh;
 	/// dispenser internal temperature degC
 	uint8_t TemperatureC;
#ifdef DISP_SAFEHAVEN_SOAP
 	/// Module ID configuration bytes (\see di_moduleId_t)
 	di_moduleId_t moduleID;
#endif //DISP_SAFEHAVEN_SOAP

} di_Status_t;

typedef struct
{
	uint8_t  swRev[MAXDATABYTES];
	uint8_t  disIntRev[MAXDATABYTES];
}rONvParams_t;

typedef struct __attribute__((__packed__))
{
    uint16_t shadowCRC;    //Init to all FF's (erased). Set to CRC value after CRC run on image.
                           //When image transferred, this byte changed back to FF's
    uint16_t CRC;          //CCRC-16-CCITT (polynomial 0x1021) with initial value = 0x0000
                           // (Xmodem) of this image of Len bytes less the shadow and crc bytes
    uint16_t mfgCode;      //Fixed value for GP
    uint8_t imageType;     //0 - 3, reserved, 4 - 31 corresponds to the dispenser type
    uint8_t imageVer;      // Format: 0xMmBB where M = Major, m = minor
    uint8_t build;         //Build number
    uint32_t imageLen;     //# of bytes in fw image (including header)
    uint8_t description[32];
    rONvParams_t rONvParams;
}fileHeader_t; //This file header is used by the firmware upgrade code.

typedef struct
{
	uint8_t dispenserType[NVMAXDATABYTES];
	uint8_t dispenserSerialNum[NVMAXDATABYTES];
	uint8_t dispenserRevision[NVMAXDATABYTES];
	uint8_t nvMode;
}nvParams_t;

typedef enum
{
	READONLY,
	READWRITE,
	RWMANFMODE
}di_paramAccess_t;
typedef struct
{
	const di_paramKey_t code;
	const uint8_t numBytes;
	uint8_t * const paramData;
	const di_paramAccess_t access;
}di_param_t;


extern const di_param_t diParamTable[PARAM_TABLE_SIZE];
extern di_Status_t di_status;

int16_t parameterCompare(const void * a, const void * b);
void readpram(di_paramKey_t pKey);
void writepram(di_command_t * command);
void DiUpdateParams();

#endif /* ENMOTION_GPIO_H_ */
