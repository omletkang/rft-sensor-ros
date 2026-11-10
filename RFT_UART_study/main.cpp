


#include <stdio.h>
#include <termios.h>

// Linux
#include <unistd.h>

#include "RFT_UART.h"

#define portnum (1)
unsigned long buad = B921600;
unsigned long byte_size = CS8;

float		g_force_divider 	= 50.0f;    // for more information to refer the RFT sensor manual
float		g_torque_divider 	= 2000.0f;  // for more information to refer the RFT sensor manual

// class variable
RFT_UART RFT_SENSOR;
PROGRAM PRO1;

int main()
{

	bool isGo = true;

	char devName[] = "/dev/ttyUSB";

	printf("RFT TEST SOFWARE\n");

	RFT_SENSOR.openPort( devName, portnum, buad, byte_size );
	// initialize force/torque divider
	RFT_SENSOR.setDivider(g_force_divider, g_torque_divider); // V1.1
	usleep(1000000);  // 1 second
	
	while( isGo )
	{
		char cmd = PRO1.linux_getch();
        if( cmd == 0x1B ) // esc
			isGo = false;
		
		else if( cmd == 'M' || cmd == 'm' ) // measure
			RFT_SENSOR.sendPacket();
		else
		{
			if( cmd != '\r' && cmd != '\n' )
				printf("UNKNOWN COMMAND\n");
		}
        
	}


	RFT_SENSOR.closePort();
	
	return 0;
}

///////////////////////
// End of This File