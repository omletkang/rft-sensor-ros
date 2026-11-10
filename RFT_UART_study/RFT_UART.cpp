#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "RFT_UART.h"

void *p_function(void * pSerial)
{
	( (RFT_UART *)pSerial )->rcvPacket();

	return NULL;
};

////////////////////////////////////////////

int PROGRAM::linux_getch(void)
{
	struct termios oldt, newt;
    int ch;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);            // CANONICAL과 ECHO 끔
    newt.c_cc[VMIN] = 1;                      	// 최소 입력 문자 수를 1로 설정
    newt.c_cc[VTIME] = 0;                     	// 최소 읽기 대기 시간을 0으로 설정
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);	// 터미널에 설정 입력
    ch = getchar();								// 키보드 입력 읽음
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);	// 원래의 설정으로 복구
  
    return ch;
}


///////////////////////////////////////////////////

BOOL RFT_UART::openPort( char* devname, int portnum, unsigned long buad, unsigned long byte_size )
{
    char port[20];
    sprintf(port,"%s%d", devname, portnum);
    printf("port is opening...");
    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY );
    if (fd == -1 )
    {
        printf("cannot open the serial port");
        return FALSE;
    }
    printf("we opened serial port: %s\n", port);
    
    portConnected = TRUE;
    
    usleep(350);


    // port setting
    struct termios oldtio, newtio;

	tcgetattr(fd,&oldtio); /* save current serial port settings */
	bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */

	newtio.c_cflag = buad | byte_size | CLOCAL | CREAD;
	tcsetattr(fd,TCSANOW,&newtio);

    /* THREAD */
    int result = pthread_create( &Thread1, NULL, p_function, (void *)this );
	if( result != 0 ) 
	{
		printf("\nCan't create read thread");
		closePort();
	}
    else
        printf("THREAD YES\n");

    return TRUE;
}

BOOL RFT_UART::closePort(void)
{
    close(fd);
    portConnected = FALSE; // flag
    return TRUE;
}

BOOL RFT_UART::setDivider(float g_force_divider, float g_torque_divider)
{
    force_divider = g_force_divider;
    torque_divider = g_torque_divider;
    return TRUE;
}

BOOL RFT_UART::sendPacket(void)
{   
    command_on = TRUE;
    // WRITE YOUR COMMAND
    //while( portConnected )
    //{
        memset(txPacket, 0x00, txPacket_size * sizeof(char));
        // COMMAND ID = 11(0x0B)
        txPacket[0] = 0x55; // SOP
        txPacket[1] = 0x0B; // START F/T DATA OUTPUT
        txPacket[txPacket_size-2] = calcChecksum(txPacket, txPacket_size);
        txPacket[txPacket_size-1] = 0xAA; // EOP

        int n_byte_temp = 0;
        do{
            int n_byte = write(fd, txPacket, sizeof(txPacket));
            n_byte_temp = n_byte;
            printf("\nwrite: %d bytes\n",n_byte_temp);
            usleep(100);
        }while( n_byte_temp < sizeof(txPacket)); // 11
        printf("send packet yes\n");


    //}

    return TRUE;
}

BOOL RFT_UART::rcvPacket(void)
{
    memset(rxbuff, 0x00, rxbuff_size * sizeof(char));
    memset(rxPacket, 0x00, rxPacket_size * sizeof(char));
    int rxbuff_index = 0;
    //unsigned char rxbuff_temp[1024*2] = { };
    #define BUFF_SIZE 1024
    char 	   buff[BUFF_SIZE];

    while( portConnected )
    {   
        // check command
        if (!command_on )
        {
            printf("\ngive me command");
            sleep(1);
            continue;
        }

        int n_byte = read(fd, buff, BUFF_SIZE);
		if( n_byte <= 0 )
		{
			//usleep( 1000 ); // just sleep.
			continue;
		}  

		// data copy
		for( int i = 0; i < n_byte; i++)
		{
			rxbuff[rxbuff_index + i] = buff[i];
		}
		rxbuff_index += n_byte;

		// wait packet.... 
		if (rxbuff_index < rxPacket_size)
		{
			printf("RECEIVED: %d\n", rxbuff_index);
			continue;
		}

		// find SOP
		int found_idx = -1;
		for (int i = 0; i < rxbuff_index; i++)
		{
			if (rxbuff[i] == 0x55) // SOP
			{
				found_idx = i;      // usually 1~10
				break;
			}
		}

		if (found_idx == -1)
		{
			rxbuff_index = 0;
			printf("SOP NOT FOUNDED\n");
			continue;
		}
		// if the index of SOP is not first(0), shift received data....
		if (found_idx != 0) 
		{
			for (int i = 0; i < (rxbuff_index - found_idx); i++)
			{
				rxbuff[i] = rxbuff[i + found_idx];
			}
			printf("SHIFT\n");
			rxbuff_index = rxbuff_index - found_idx;
		}
        //printf("%d\n", rxbuff_index);

		// packet handling....
		if (rxbuff_index >= rxPacket_size )
        {

            // transfer 
            int pkt_size = rxPacket_size;
            int num_of_rxPkt = rxbuff_index / pkt_size;
            int num_of_next_byte = rxbuff_index % pkt_size;
            int transfer_data_size;
            int restart_idx;
            // if the number of received packet is more than 1, 
            // move the last packet data to first location of buffer to interprete the last received data.
            if (num_of_rxPkt > 1) 
            {
                transfer_data_size = pkt_size + num_of_next_byte;
                restart_idx = (num_of_rxPkt - 1) * pkt_size;

                for (int i = 0; i < transfer_data_size; i++)
                    rxbuff[i] = rxbuff[restart_idx + i];

                //printf("DATA TRANSFER - BEFORE\n");
            }
 
            if (num_of_rxPkt >= 1 )
            {

                for ( int i = 0; i < rxPacket_size; i++)
                {
                    rxPacket[i] = rxbuff[i];
                }

                //if ( rxPacket == 0)
                    //continue; //break;

                unsigned char checksum = calcChecksum(rxPacket, rxPacket_size);
                if ((rxPacket[0] == 0x55) // SOP
                    && (rxPacket[rxPacket_size - 2] == checksum)
                    && (rxPacket[rxPacket_size - 1] == 0xAA)) // EOP
                {
                    //printf("RIGHT");
                    //sleep(2);
                    convertPacket_To_Force(rxPacket);
                }
                else
                    printf("WRONG");
                    //continue; //break;


                if( data_processed_check )
                {

                    if (num_of_next_byte)
                    {
                        transfer_data_size = num_of_next_byte;
                        restart_idx = pkt_size;
                        for (int i = 0; i < transfer_data_size; i++)
                            rxbuff[i] = rxbuff[restart_idx + i];

                        //printf("DATA TRANSFER - AFTER\n");

                        rxbuff_index = num_of_next_byte;
                    }
                    else
                    {
                        rxbuff_index = 0; 
                    }

                }


            }

            
        }



    }
    
    return TRUE;
}

void RFT_UART::convertPacket_To_Force(unsigned char* rxPacket)
{


    short raw; // rxPacket_size - 3 = 16
    unsigned short temp;

    for (int i = 0; i < 6; i++)
    {
        temp = rxPacket[2 + i*2]*256 + rxPacket[3 + i*2];
        raw = (signed short)temp;
        
        if( i < 3 )
        {
            // force
            force[i] = ((float)raw) / force_divider;
        }
        else
        {   
            // torque
            force[i] = ((float)raw) / torque_divider;
        }
    }

    printf("%0.3f %0.3f %0.3f %0.3f %0.3f %0.3f\n", force[0], force[1], force[2], force[3], force[4], force[5]);
    
    data_processed_check = TRUE;
}



// functions for UART communication, checksum calculation, UART Packet generation
unsigned char RFT_UART::calcChecksum(unsigned char *pkt_buff, int pkt_size)
{
	unsigned char checksum = 0;

	for (int idx = 1; idx < (pkt_size - 2); idx++) // except SOP, CHECKSUM and EOP
		checksum += pkt_buff[idx];

	return checksum;
}
