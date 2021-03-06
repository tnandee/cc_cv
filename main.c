/* MAIN.C file
 * 
 * Copyright (c) 2002-2005 STMicroelectronics
 */

#include <stdio.h>
#include <iostm8s.h>

unsigned int readAda(int);
void myDelay(unsigned int);

void myDelay(unsigned int t)
{
	 while(t-- > 0); 
}

void GPIO_Config(void)
{ 
		 PC_DDR |= 1<<3 | 1<<4 | 1<<5 | 1<<6 | 1<<7; //output
     PC_CR1 |= 1<<3 | 1<<4 | 1<<5 | 1<<6 | 1<<7; //push pull output
     PD_DDR |= 1<<1 | 1<<4; //output
     PD_CR1 |= 1<<1 | 1<<4; //push pull output
		 
		 PA_DDR |= 1<<1 | 1<<2 | 1<<3;
		 PA_CR1 |= 1<<1 | 1<<2 | 1<<3;
}

unsigned int readAda(int ch) 
{
     unsigned int val=0;
		 unsigned char channel = (unsigned char)ch;
     //using ADC in single conversion mode
		 unsigned char tmp = (unsigned char)channel & 0x0F;
		 ADC_CSR = tmp; // select channel
     ADC_CR2 |= (1<<3); // Right Aligned Data
     ADC_CR1 |= (1<<0); // ADC ON 
     ADC_CR1 |= (1<<0); // ADC Start Conversion
     while((unsigned char)((ADC_CSR)&(1<<7))== 0); // Wait till EOC
     val = (unsigned int)ADC_DRL;
     val = (unsigned int)ADC_DRH<<8 | val;
     ADC_CR1 &= (unsigned char)~(1<<0); // ADC Stop Conversion
     val &= 0x03ff;
		 return val;
		 // return (unsigned int)(val);
}


unsigned char asc2seg7(unsigned char d)
{
	unsigned char segments[] = {//abcdefgp
															0b11111100, // 0
															0b01100000, // 1
															0b11011010, // 2
															0b11110010, // 3
															0b01100110, // 4
															0b10110110, // 5
															0b10111110, // 6
															0b11100000, // 7
															0b11111110, // 8
															0b11110110, // 9
															0b00000000, // space
															0b11101110, // A
															0b00111110, // b
															0b10011100, // C
															0b01111010, // d
															0b10011110, // E
															0b10001110, // F
															0b10111110, // G
															0b01101110, // H
															0b00001100, // I
															0b11110000, // J
															0b01101110, // K
															0b00011100, // L
															0b01101110, // M
															0b00101010, // n
															0b00111010, // o
															0b11001110, // P
															0b11111101, // Q
															0b00001010, // r
															0b10110110, // S, 5
															0b00011110, // t
															0b00111000, // u
															0b01111100, // V, U
															0b01111100, // W, V
															0b01101110, // X
															0b01001110, // y
															0b11011010, // Z, 2

															};
	if (d >= '0' && d <='9'){ d = d - '0'; }
	else if (d == ' '){ d = 10; }
	else if (d >= 'A' && d <= 'Z'){ d = d - 'A' + 11; }
	else if (d >= 100) {return segments[d - 100] | 0x01;}
	return segments[d];
}

void display_digit(unsigned char digit)
{
	char *odrs[] = {(char*)0x500a, // PC 
									(char*)0x500a, // PC
									(char*)0x500f, // PD
									(char*)0x500f, // PD
									(char*)0x500a, // PC
									(char*)0x500a, // PC
									(char*)0x500a, // PC
								  (char*)0x5000};// PA
	
	char pins[] = {1<<3, // PC3
								 1<<5, // PC5
								 1<<1, // PD1		
								 1<<4, // PD4	
								 1<<6, // PC6	
								 1<<4, // PC4  
								 1<<7, // PC7
							   1<<3}; // PA3						
	int i;
	unsigned char mask = 0x80;
	digit = asc2seg7(digit);
	
	for(i=0; i<=7; i++){
		if ((unsigned char)(digit & mask) > 0){
			*odrs[i] &= (unsigned char)~pins[i];
		}
		else{
			*odrs[i] |= (unsigned char)pins[i];
		}
		digit = (unsigned char)(digit << 1);
	}
}

void refreshDisplay(unsigned char* digitVals)
{
	unsigned char digitId;
	unsigned char map[] = {4,5,7,6,0,1,3,2};
	
	PA_ODR |= (unsigned char)1<<2; // DSA, DSB
	for(digitId = 0; digitId <= 7; digitId++){
		PA_ODR &= (unsigned char)~(1<<1); // CP
		display_digit(' ');
		myDelay(10);
		PA_ODR |= (unsigned char)(1<<1); // CP
		myDelay(1);
		PA_ODR &= (unsigned char)~(1<<2); // DSA, DSB
		display_digit(*(digitVals + map[digitId]));
		myDelay(1);
	  display_digit(' ');
		myDelay(10);
	}
}

void formatDisplay(char* in_str, int offset, unsigned char* out_data)
{
			int i = 0;
			while(*in_str){
				if(*in_str == '.') 
					{*(out_data + offset + i - 1) += 100; in_str++;} // 100 is just a fake number
				else 
					{*(out_data + offset + i) = *in_str - '0'; i++; in_str++;}
			}
}


int main(void)
{
		unsigned int sum_volt_raw = 0;
		float volt;
		unsigned int sum_amp_raw = 0;
		float amp;
		unsigned char displayData[] = {'H', 'A', 'L', 'I', 'N', 'A', 'N', 'D'};
		unsigned int i = 0;
		unsigned int nAvg = 16;
		char str_volt[10];
		char str_amp[10];
		char *ptr;
		int refreshInterval = 50;
		int refreshTimer = 0;
		

		GPIO_Config();
 
		while(1){
			sum_volt_raw = sum_amp_raw = 0;
			for(i=0; i<nAvg; i++){
				sum_volt_raw += readAda(0x03);
				sum_amp_raw += readAda(0x04);
				myDelay(1);
			}
			sum_volt_raw = (int)(sum_volt_raw / nAvg);
			sum_amp_raw = (int)(sum_amp_raw / nAvg);
			
			volt = (float)(((float)sum_volt_raw * 0.0403) + 0.13);
			sprintf(str_volt, "%05.2f", (float)volt);
				
			amp = (float)(((float)sum_amp_raw * 0.0076) - 0.03);
			if (amp > 0.99) sprintf(str_amp, "%05.2f", (float)amp);
			else sprintf(str_amp, "%05.3f", (float)amp);
				
			if (refreshTimer >= refreshInterval){
				formatDisplay(str_volt, 0, displayData);
				formatDisplay(str_amp, 4, displayData);
				refreshTimer = 0;
			}
			else {
				refreshTimer += 1;
			}
			refreshDisplay(displayData);
		}
}