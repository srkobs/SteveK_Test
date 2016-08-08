/*
 * di_commands.c
 *
 *  Created on: May 29, 2014
 *      Author: Dave Murphy
 *
 * © 2013. Confidential and proprietary information of Georgia-Pacific Consumer Products LP. All rights reserved.
 *
 */

#include <stdint.h>
#include <msp430.h>
#include "di.h"
#ifdef DISP_ENMOTION_CLASSIC
#include "../UserSettings.h"
#endif //DISP_ENMOTION_CLASSIC

//------------------------------------------------------------------------------
//
//   Function: performDiCommand
//
//   Description:
//
//      This function will parse out a command from the buffer given the
// start of frame and end of frame. It will also strip out any escape characters
// from the frame.
//
//   Inputs:
//
//      None
//
//   Returns:
//
//      None
//
//------------------------------------------------------------------------------

uint8_t performDiCommand(uint8_t sof, uint8_t eof)
{
	di_command_t command;
	int8_t numbytes = 0; //
	uint8_t i = 0;
	uint8_t j = 0;
	uint8_t escapedBuff[MAX_FRAME_BYTES];
	uint8_t retVal = FALSE;
	uint16_t receivedCrc;

	//Calculate number of bytes in rxbuffer, not including eof, sof bytes
	if (eof > sof)
	{
		numbytes = eof - sof - 1;
	}
	else //rx buffer wraps around
	{
		numbytes = sizeof(rxbuf) - sof + eof - 1;
	}
	if (numbytes >= (MIN_FRAME_BYTES - 2))
	{
        //Remove all escape characters
        while (j++ < numbytes)
        {
            //Skip if ESC is found and store the next byte after ESC
            if (rxbuf[nextindex (j, sof)] == ESC)
            {
                escapedBuff[i] = rxbuf[nextindex (++j, sof)];
            }
            else //no ESC so just store next byte
            {
                escapedBuff[i] = rxbuf[nextindex (j, sof)];
            }
            i++; //Note that i will be equal to size of escapedBuff + 1 when loop terminates
        }
        //Calculate the CRC of the frame. escapedBuff should be i-1 bytes long, with last two bytes being crc
        crc_ccitt_init();
        for (j = 0; j < (i - 2); j++)
        {
            crc_ccitt_update(escapedBuff[j]);
        }
        //last two bytes of escaped buff should have crc in MSB order
        receivedCrc = escapedBuff[i-2] << 8 | escapedBuff[i-1];
        //Check if CRC is correct
        if (crc_ccitt_crc() == receivedCrc)
        {
        	//This function updates the varibles for on a UART recieve
        	DiUpdateParams();

            //We have a good frame so parse out the command
            command.comKey = (di_commandKey_t)escapedBuff[CMD_INDEX - 1];
            command.paramKey = (di_paramKey_t)escapedBuff[CDATA_START - 1];
            command.paramData = &escapedBuff[PDATA_START - 1];

            command.pDataNumBytes = i - 4;	//subtract out comKey, paramKey, and the 2 CRC bytes

            switch (command.comKey)
            {
            case CM_PING:
                buildframe(0, CM_PING_RESPONSE);
              break;
            case CM_EVENT_OCCURRED:
            case CM_HIGH_PRIORITY_EVENT:
              event(&command);
              break;
            case CM_READ_PARAM:
              readpram(command.paramKey);
              break;
            case CM_WRITE_PARAM:
              writepram(&command);
            #ifdef DISP_ENMOTION_CLASSIC
              // Read user switches because they may have been updated
            readUISwitches();
            readDispenseMode();
            #endif //DISP_ENMOTION_CLASSIC
              break;
            //default:
            }

            retVal = TRUE;
        }
	}
    return retVal;
}

void SendFactoryReset()
{
    txbuf[CDATA_START] = DB_FACTORY_RESET;
    buildframe(1,CM_DAUGHTERBOARD);
}
