/*
 * di_commands.h
 *
 *  Created on: May 29, 2014
 *      Author: Dave Murphy
 *
 * © 2014. Confidential and proprietary information of Georgia-Pacific Consumer Products LP. All rights reserved.
 *
 */

#ifndef DI_COMMANDS_H_
#define DI_COMMANDS_H_

typedef enum
{
    CM_PING,
    CM_EVENT_OCCURRED,
    CM_READ_PARAM = 0x20,
    CM_READ_PARAM_RESPONSE,
    CM_WRITE_PARAM,
    CM_RESERVED_ESC = 0x7D,
    CM_RESERVED_SOFEOF = 0x7E,
    CM_DAUGHTERBOARD = 0x7F,
    CM_PING_RESPONSE = 0x80,
    CM_HIGH_PRIORITY_EVENT
}di_commandKey_t;

typedef struct
{
	di_commandKey_t comKey;
	di_paramKey_t paramKey;
	uint8_t * paramData;
	uint8_t pDataNumBytes;
} di_command_t;

uint8_t performDiCommand(uint8_t sof, uint8_t eof);
void SendFactoryReset();
#endif // DI_COMMANDS_H_
