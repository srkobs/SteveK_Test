/*
 * Dispenser Interface diReadMe.txt
 *
 *  Created on: Jun 12, 2014
 *      Author: Dave Murphy
 *
 * © 2014. Confidential and proprietary information of Georgia-Pacific
 * Consumer Products LP. All rights reserved.
 */
 
Using the source code library:
To incorporate this library into a project perform the following steps:
 
1. In the directory that your source files are in for your project, create
a subdirectory, "di":
<PROJDIR>/di
 
2. Checkout the dispenser interface source files from subversion into this
directory.
 
3. Include these files in the build of your project, which happens
automatically for Code Composer Studio
 
4. Configure the DI source headers for your project as follows:
di.h:
	a. Define the appropriate dispenser type. If this is a new dispenser that is
	unique from all the others, then a new type should be created
	
	b.define FLASH_CLOCK_DIV to the appropriate constant based on SMCLK for
	your project
	
	c. Update the SW revison, build number, and SW revision string
 
di_params.h:
	a. define di_statusBits_t based on your project
di_params.c:
	a. add a new rONvParams to specify the revisions for your dispenser. This needs to be
	done for every new release of software!
	
	b. Add a new diParamTable for the new dispenser setup with the correct varibles
	
di_events.h:
	a. Modify GPIO port defines if your project has a different pinout than standard
	Note: any modifications to the standard pinouts must be enclosed in the 
	conditional compile for your dispenser type created in di.h before incorporating
	these changes back into the source code library

di_com.c:
	a. Modify readCommMode() for any differences from the standard pinout for your
	dispenser.
	
5. Add new dispenser specific parameters (See Adding New Parameters in this document)
6. Add new dispenser specific input and output events (See Adding New Events in this document)
7. Add call to HandleDiRx() to your main loop. This function will handle all the characters
received and check for incoming commands on the dispenser interface. 
8. Add call to readCommMode() to power up initialization code. This function will determine
what communications mode the dispenser interface is in and set up the ports accordingly.
9. Add calls to sendEvent(di_paramKey_t evt, uint16_t evtData) in your code where ever
appropriate for output events.
10. Modify the Input Event Functions found in di_events.c for your particular dispenser.
11. Add call to DiUpdateParams() to main loop to periodically update the state of 
parameters.

//***************************************************************************
Adding a New Parameter:
1. di.h: Add a new di_paramKey_t to the enumeration at the appropriate place.
The placement in this enumeration will define the parameter code for the parameter and
needs to follow the DiMasterTable.xls. It should be placed in the area for dispenser
specific parameters.

2. di_params.c: Add the new parameter to diParamTable at the same place you added the 
parameter enumeration from step 1 above. !!!This table has to be in the same order as the enumeration
because a binary search is performed on it to do parameter lookup!!! Use existing parameters as a guideline
for correctly defining your parameter. The parameter should be enclosed in conditional compiles
for the dispenser type defined in di.h.

3. Your code is responsible for maintaining and updating the values of the parameters as
appropriate. You can use the function DiUpdateParams() to do this from a single point
in your main loop if desired.

//***************************************************************************
Adding a New Event:
1. di.h: Add a new di_paramKey_t to the enumeration at the appropriate place.
The placement in this enumeration will define the parameter code for the parameter and
needs to follow the DiMasterTable.xls. It should be placed in the area for input
events or output events depending on which type of event you are creating.

2. di_params.c: Add a new event config parameter to diParamTable at the same place you added the 
parameter enumeration from step 1 above. !!!This table has to be in the same order as the enumeration
because a binary search is performed on it to do parameter lookup!!! Use existing event config 
parameters as a guideline for correctly defining your parameter. The event config parameter should be enclosed
in conditional compiles for the dispenser type defined in di.h.

3. For Output Events Only: Add a case to the switch statement in sendEvent for your event. Use
existing events as guidline for how to do this.

4. For Input Events Only: Add a function to the section Input Event Functions in di_event.c. This function
will be called when the Input event is received over the dispenser interface. Also add the newly created
function to inputEvtFn in di_event.c, in the order that the parameter enumeration is defined in step 1
above.
5. Initialize the event configuration in inputEventConfigs for Input Events, or outputEventConfigs
for output events. These will be the default event configurations for the dispenser on 
powerup. The configuration must be in the same order as the parameter enumeration
defined in step 1 above.