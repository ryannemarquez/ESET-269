#include "msp.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//Functions
void UARTInit(void)
{
	EUSCI_A0 -> CTLW0 |= 1;
	EUSCI_A0 -> MCTLW = 0;
	EUSCI_A0 -> CTLW0 |= 0x80;
	EUSCI_A0 -> BRW = 0x34;
	EUSCI_A0 -> CTLW0 &= ~0x01;
	P1 -> SEL0 |= 0x0C;
	P1 -> SEL1 &= ~0x0C;
	
	return;
}
void TX(char text[])
{
	int i =0;
	while(text[i] != '\0')
	{
		EUSCI_A0 ->TXBUF = text[i];
		while((EUSCI_A0 ->IFG & 0x02) == 0)
		{
			//wait until character sent
		}
		i++;
	}
	return;
}
int RX(void)
{
	int i = 0;
	char command[2];
	char x;
	while(1)
	{
		if((EUSCI_A0 ->IFG & 0x01) !=0) //data in RX buffer
		{
			command[i] = EUSCI_A0 ->RXBUF;
			EUSCI_A0 ->TXBUF = command[i]; //echo
			while((EUSCI_A0 ->IFG & 0x02)==0); //wait
			if(command[i] == '\r')
			{
				command[i] = '\0';
				break;
			}
			else
			{
				i++;
			}
		}
	}
	x = atoi(command);
	TX("\n\r");
	return x;
}
void ADCInit(void)
{
	//Ref_A settings
	REF_A ->CTL0 &= ~0x8; //enable temp sensor
	REF_A ->CTL0 |= 0x30; //set ref voltage
	REF_A ->CTL0 &= ~0x01; //enable ref voltage
	
	//do ADC stuff
	ADC14 ->CTL0 |= 0x10; //turn on the ADC
	ADC14 ->CTL0 &= ~0x02; //disable ADC
	ADC14 ->CTL0 |=0x4180700; //no prescale, mclk, 192 SHT
	ADC14 ->CTL1 &= ~0x1F0000; //configure memory register 0
	ADC14 ->CTL1 |= 0x800000; //route temp sense
	ADC14 ->MCTL[0] |= 0x100; //vref pos int buffer
	ADC14 ->MCTL[0] |= 22; //channel 22
	ADC14 ->CTL0 |=0x02; //enable adc
		
	return;
} 
float tempRead(void)
{
	float temp; //temperature variable
	uint32_t cal30 = TLV ->ADC14_REF2P5V_TS30C; //calibration constant
	uint32_t cal85 = TLV ->ADC14_REF2P5V_TS85C; //calibration constant
	float calDiff = cal85 - cal30; //calibration difference
	ADC14 ->CTL0 |= 0x01; //start conversion
	while((ADC14 ->IFGR0) ==0) 
	{
		//wait for conversion
	}
	temp = ADC14 ->MEM[0]; //assign ADC value
	temp = (temp - cal30) * 55; //math for temperature per manual
	temp = (temp/calDiff) + 30; //math for temperature per manual
	
	return temp; //return temperature in degrees C
}
void timer32(int x)
{
	TIMER32_1 -> LOAD = (3000000 * x) - 1 ; //3000000 = 1 sec
	TIMER32_1 -> CONTROL |= 0x42;  //0100 0010
	TIMER32_1 -> CONTROL |= 0x80; // enable timer
	while ((TIMER32_1 -> RIS & 1) != 1)
	{
		// and we wait
	}
	TIMER32_1 -> INTCLR &= ~0x01; // sets int clear to 0 after count is reached
	TIMER32_1 -> CONTROL &= ~0x80; //disable timer
}
void systick( )
{
	SysTick -> LOAD = 3000000 - 1; // delay of 1 sec
	SysTick -> VAL = 0;
	SysTick -> CTRL = 0x5;
	
	while((SysTick -> CTRL & 0x10000) == 0) // while COUNTFLAG not set
	{
		//wait
	}
	SysTick -> CTRL = 0; //disable SysTick
}
	
// START OF MAIN CODE

int main(void)
{
	UARTInit();
	ADCInit();
	
	// initializing variables
	int menu = 1, choice;
	int numLED, time, blinks, i = 1; // choice 1
	int result, z; // choice 2
	//choice 3
	int NumTemp, j = 1; 
	float tempC, tempF; 
	char tempStr[50]; 
	
	TX("\r\n\n\n");
	
	while (menu == 1)
	{
		TX("\rMSP432 Menu\n\n");
		TX("\r1. RGB Control\n\r2. Digital Input\n\r3. Temperature Reading\n\r");
		choice = RX();
		
		switch(choice)
		{
			
			case(1):
				TX("\rEnter Combination of RGB (1-7):");
				numLED = RX();
			
				if ((numLED > 7) || (numLED < 1))
				{
					TX("\rInput invalid, defaulting input to 7\n");
					numLED = 7;
				}
				
				TX("\rEnter Toggle Time:");
				time = RX();
				TX("\rEnter Number of Blinks:");
				blinks = RX();
				TX("\rBlinking LED...\n");
				
				// run the timer
				P2 -> SEL0 &= ~numLED;
				P2 -> SEL1 &= ~numLED;
				P2 -> DIR |= numLED; // output pin
				P2 -> OUT &= ~numLED;
				
				while(i <= blinks)
				{
					P2 -> OUT |= numLED;
					timer32(time);
					P2 -> OUT &= ~numLED;
					timer32(time);
					i++;
				}
				TX("\rDone\n");	
				break;
					
			case(2):
				// set up
				P1 -> DIR &= ~0x12; // seting pin 1 and pin 4 to input
				P1 -> REN |= 0x12; // enable internal resistor for pin 1 and pin 4
				P1 -> OUT |= 0x12; // set internal resistor to pull up for pin 1 and 4
		
				z = P1 -> IN & 0x12; // both
				if (z == 0x12)
				{
					TX("\rNo button pressed.\n\n");
				}
				else if(z == 0x10)
				{
					TX("\rButton 1 is pressed.\n\n");
				}
				else if(z == 0x02)
				{
					TX("\rButton 2 is pressed.\n\n");
				}
				else
				{
					TX("\rBoth Button 1 and Button 2 are pressed.\n\n");
				}
				break;
			
			case(3):
				TX("\rEnter Number of Temperature Reading (1 - 5):");
				NumTemp = RX();
				if ((NumTemp > 5) || (NumTemp < 1))
				{
					TX("\rInput invalid, defaulting input to 5\n");
					NumTemp = 5;
				}
				j = 1;
				while (j <= NumTemp)
				{
					tempC = tempRead();
					tempF = (tempC * 1.8) + 32;
					
					snprintf(tempStr, sizeof(tempStr), "\rReading %d: %.2f C & %.2f F\n", j, tempC, tempF);
					TX(tempStr);
					systick();
					j++;
				}
				break;
			
			default:
				TX("\rNot a valid option\n");
				break;
		}
	}
	
	return 0;
}