#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include "lcd.h"
#include "millis.h"
#define F_CPU 16000000UL

//Comunicao serie////
#define EVEN 0
#define ODD 1
#define NONE 2
#define FALSE 0
#define TRUE 1
unsigned char cont_buffer;
char buffer_tx[50];
////////////////////////

char buffer[10],buffer_a0[10],buffer_a1[10],buffer_a2[10],buffer_capacidade[10],buffer_temp[10],buffer_current[10], buffer_cut[10], buffer_term[10], buffer_rint[10];;
char buffer_voc[10], buffer_vload[10];
double res_term;
double temp_term;
float voltage_a0;
float voltage_a1;
float temp_a2;
double capacidade = 0.0;
float current = 0.0;
float R_mosfet = 0.044; //ohm - datasheet
float V_cutoff = 0;
double R_int;
float Voc; 
float Vload= 0.0;
int R_load = 1;
int escolha;
int acum=0;
unsigned long temp1 = 0;
long int cnt1s;
long int cnt1m;
unsigned long temporizadorms;
unsigned int runMillis;
unsigned int previousMillis = 0;
unsigned int millisPassed = 0;
int minutos; // tempo decorrido para mostrar no display
unsigned long comecoms;
float n;
int x=-1;
int teste;
int inicio;
int cap = 0;
int sw11,sw22,sw33;
uint16_t adc_result_a2, adc_result_a3;
float adc_result_a0, adc_result_a1;
int percent = 0;

void InitializeUART0(int baud,char AsyncDoubleSpeed, char DataSizeInBits, char ParityEVENorODD, char StopBits, char USARTinterruptEnable)
{
	uint16_t UBBRValue = lrint( ( F_CPU / (16L * baud) ) - 1);
	if (AsyncDoubleSpeed ==1) UCSR0A = (1 << U2X0);
	UBRR0H = (unsigned char) (UBBRValue >> 8);
	UBRR0L = (unsigned char) UBBRValue;
	UCSR0B = (1 << TXEN0);
	if (USARTinterruptEnable) UCSR0B |=(1 <<RXCIE0);
	if (StopBits == 2) UCSR0C = (1 << USBS0);
	if (ParityEVENorODD == EVEN) UCSR0C |= (1 << UPM01);
	if (ParityEVENorODD == ODD) UCSR0C |= (3 << UPM00);
	if (DataSizeInBits ==6) UCSR0C |= (1 << UCSZ00);
	if (DataSizeInBits ==7) UCSR0C |= (2 << UCSZ00);
	if (DataSizeInBits ==8) UCSR0C |= (3 << UCSZ00);
	if (DataSizeInBits ==9) UCSR0C |= (7 << UCSZ00);
}

void TX_buffer(void)
{
	cont_buffer=0;
	while(buffer_tx[cont_buffer] !=0){
		while((UCSR0A & 0x20) == 0);
		UDR0 = buffer_tx[cont_buffer];
		cont_buffer++;
	}
}

void apagar_variaveis(void)
{
	teste = 0;
	inicio = 1;
	sw11 = 0;
	sw22 = 0;
	sw33 = 0;
	V_cutoff = 0.0;
	acum = 0;
	capacidade=0.0;
	Vload=0;
	Voc=0;
	current= 0.0;
	millisPassed= 0;
	runMillis=0;
	previousMillis=0;
	comecoms=0;
	voltage_a0=0;
	cap=0;
	escolha=0;
}

//*************************************INICIALIZACAO******************************
void init (){
	inicio=1;
	teste=0;
	Vload = 0.0;
	DDRB &= ~(1<<0);// SW1 como entrada
	DDRB &= ~(1<<2);// SW2 como entrada
	DDRC |= (1<<4);// LED verde como saida; Para atmega 1Mhz: DDRB |= (1<<6);
	DDRC |= (1<<5);// LED vermelho como saida; Para atmega 1Mhz: DDRB |= (1<<7);
	DDRB |= (1<<3);
	PORTC |= (1<<4);// LED verde ligado
	PORTC &= ~(1 << 5); // LED vermelho desligado
	PORTB &= ~(1 << 3); // Teste começa desligado
	DDRC &= ~(1<<0);
	DDRC &= ~(1<<1);
	DDRC &= ~(1<<2);
	DDRC &= ~(1<<3);
	////////////////////////// TIMER0
	TIMSK0 |= 0b00000010;
	TCCR0A = 0b10000010;
	TCCR0B = 0b00000010;
	OCR0A=243;
	//////////////////////
	millis_init(); //Inicia MILLIS
	InitializeUART0(9600, 0, 8, NONE, 2, TRUE); //Inicia comunicacao serie
	lcd_init(LCD_DISP_ON); // Inicia LCD
	lcd_clrscr();
	lcd_home(); 
	lcd_puts("TESTE DE PILHAS");      
	lcd_gotoxy(6,1);         
	lcd_puts("ISEP"); 
	_delay_ms(80000);
	lcd_clrscr();
	sei(); // SREG |= 0x80;
	cnt1s=8000; // Timer 1s para piscar LED; cnt1s=500 para atmega 1Mhz
	cnt1m=480000; // Timer 1m para UART;	cnt1m=60000 para atmega 1Mhz
}

void InitADC()
{
	ADMUX=(1<<REFS0);  // For Aref=AVcc;
	ADCSRA=(1<<ADEN)|(0<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //Prescaler div factor =8
}

uint16_t ReadADC(uint8_t ch)
{	
	ADMUX=(1<<REFS0); // Aref=AVcc;
	ADCSRA=(1<<ADEN)|(0<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //Prescaler div factor =8
	ch=ch&0b00000111;//Seleciona canal ADC
	ADMUX|=ch;
	ADCSRA|=(1<<ADSC);
	while(!(ADCSRA & (1<<ADIF)));
	ADCSRA|=(1<<ADIF);
	return(ADC);
}

ISR(TIMER0_COMPA_vect)
{
	if (inicio == 1 && teste == 0)
		{
			adc_result_a0 = ReadADC(0);
			voltage_a0 = (adc_result_a0*5)/1024;
			Voc = voltage_a0; //// Obtem valor Voc para calculo da resistência interna
			
			//adc_result_a1=ReadADC(1);			// A1 - canal 1 do ADMUX
			//voltage_a1 = (adc_result_a1*5)/1024;
			//current = ( voltage_a1)*1000 ; //Formula Corrente em mA!!!

			//adc_result_a2=ReadADC(2); //Temperatura - canal 2 do ADMUX
			//temp_a2=((adc_result_a2)*0.48876);//temp_a2=((adc_result_a2)/1024)*5*1000/10... Como 10mV = 1ºC divide por 10;
			
			//adc_result_a3 = ReadADC(3);
		}
		
	if (teste==1 && inicio == 0)
	{
		cnt1s--;
		cnt1m--;
		
			if(cnt1s==0) // Timer para piscar LED
				{
				PORTC=PORTC^0b00010000;	//Altera estado do LED
	
				adc_result_a0 = ReadADC(0); // A0 - canal 0 do ADMUX
				voltage_a0 = (adc_result_a0*5)/1024;

				adc_result_a1 = ReadADC(1);			// A1 - canal 1 do ADMUX
				voltage_a1 = (adc_result_a1*5)/1024;
				//current = ( voltage_a1)*1000 ; //Formula Corrente em mA!!!

				adc_result_a2 = ReadADC(2); //Temperatura - canal 2 do ADMUX
				temp_a2=((adc_result_a2)*0.38876);//temp_a2=((adc_result)/1024)*5*1000;
				
				adc_result_a3 = ReadADC(3);
				
				cnt1s=8000;			
				}
				
			if (cnt1m==0) // Timer para comunicação porta série
				{	
					if (Vload==0) ///////////// Guarda o valor de Vload para calculo da resistência interna antes do início do teste.
					{						
						Vload = voltage_a0;
					}		
								
					//sprintf(buffer_tx,"%.2f V | %.0f mA | %.0f 'C | %.0f mAh | %d min\r\n",voltage_a0, current, temp_a2, capacidade, minutos );
					sprintf(buffer_tx,"%s V | %s mA | %s 'C | %s mAh | %d min\r\n",buffer_a0, buffer_current, buffer_temp, buffer_capacidade, minutos );
					TX_buffer();
					cnt1m=480000;
				}
		}

	}

int main(void)
{
	init();
	InitADC();
	while (1) 
    {
      while ((inicio == 1) && (teste == 0))
	  {
		while(voltage_a0 < 0.8)//////// TESTE DE PILHA INSERIDA ///////////////////
		{        
			lcd_home();  
			lcd_puts(" INSERIR PILHA");
			////////////////////////// prints para testar leituras //////////////////////
			//lcd_gotoxy(0, 1);
			//dtostrf(voltage_a0,3,2,buffer_a0);
			//lcd_puts(buffer_a0);
			
			//lcd_gotoxy(6, 1);
			//current = ( voltage_a1)*1000 ;
			//dtostrf(current,3,2,buffer_current);
			//lcd_puts(buffer_current);
						
			//temp_term = log(((10240000/adc_result_a3) - 4700));
			//temp_term = 1 / (0.001129148 + (0.000234125 * temp_term) + (0.0000000876741 * temp_term * temp_term * temp_term));
			//temp_term = temp_term - 273.15;           // Convert Kelvin to Celcius
			//lcd_gotoxy(0, 1);
			//dtostrf(temp_term,4,0,buffer_term);
			//lcd_puts(buffer_term);
			////////////////////////////////////////////////////////////////////////////////
					while (voltage_a0 > 0.6 && voltage_a0 < 0.8) {
					//lcd_gotoxy(0, 1);
					//dtostrf(voltage_a0,3,2,buffer_a0);
					//lcd_puts(buffer_a0);
					
					lcd_gotoxy(0,1);
					lcd_puts("--DESCARREGADA--");
					PORTC |= (1<<5);
					escolha=0;
								}
			PORTC &= ~(1<<5);
			lcd_gotoxy(0,1);
			lcd_puts("                ");
			sw11 = 0;
		}
		escolha=1;
		if ((sw11 == 0) && (sw22 == 0) && (sw33 == 0) && (voltage_a0 > 0.8) && (escolha=1))
			{
				lcd_clrscr();
				sw11=1;										
				lcd_gotoxy(1,0);
				lcd_puts("ESCOLHER TIPO");
				
				//lcd_gotoxy(0, 1);
				//dtostrf(voltage_a0,3,2,buffer_a0);
				//lcd_puts(buffer_a0);
								
				}
		 ////////////////// PRIMEIRO SWITCH TIPO///////////////
		if ((PINB&(1<<0)) && sw11 == 1)
			{
			_delay_ms(800);
			if(!(PINB&(1<<0))){
			_delay_ms(800);}
			acum = acum % 3;
			switch (acum) {
			case 0:  // ALCALINA
			lcd_gotoxy(0, 1);
			lcd_puts("            ");
			lcd_gotoxy(4, 1);
			lcd_puts("Alcalina");
			acum++;
			//if (voltage_a0 > 2.5) {
				//V_cutoff = 4.8; // ALCALINA 9V
			 //} else {
				V_cutoff = 0.8;
					// }
			_delay_ms(1600);
			break;

			case 1: // NI-MH / NI-CD
			lcd_gotoxy(0, 1);
			lcd_puts("        ");
			lcd_gotoxy(1, 1);
			lcd_puts("Ni-Mh / Ni-Cd");
			acum++;
			//if (voltage_a0 > 2.5) {
			 // V_cutoff = 7; // NIQUEL 9V
		//	} else {
			  V_cutoff = 1;
		//	}
			_delay_ms(1600);
			break;

			case 2:  // ZINCO
			lcd_gotoxy(0, 1);
			lcd_puts("                ");
			lcd_gotoxy(5, 1);
			lcd_puts("Zinco");
			acum++;
			//if (voltage_a0 > 2.5) {
			 // V_cutoff = 7; // ZINCO 9V
			//} else {
			  V_cutoff = 0.9;
		//	}
			_delay_ms(1600);
			break;
		}
		}

		if ((PINB & (1<<2)) && (V_cutoff != 0) && sw11 == 1 ) { //////////////// SEGUNDO SWITCH TIPO //////////
			_delay_ms(800);
	 		if(!(PINB&(1<<2))){ 
 			_delay_ms(800);
			 }  
			lcd_clrscr();
			sw11 = 0;
			sw22 = 1;
			sw33 = 0;
		}
	
		if ((sw11 == 0) && (sw22 == 1) && (sw33 == 0) && (V_cutoff != 0))
				{
				lcd_gotoxy(2, 0);
				lcd_puts("Capacidade:");
				}
				
		if ((PINB&(1<<0)) && sw22 == 1) {	////////////////// PRIMEIRO SWITCH CAPACIDADE ///////////////
					_delay_ms(800);
					if(!(PINB&(1<<0))){ 
					_delay_ms(800);
					}
					cap +=100;
					lcd_gotoxy(2, 1);
					dtostrf(cap,4,0,buffer);
					lcd_puts(buffer);
					lcd_gotoxy(8, 1);
					lcd_puts("mAh");
					_delay_ms(1600);
					}
					
		if ((PINB & (1<<2)) && (sw22 == 1) && (cap !=0) ) { //////////////// SEGUNDO SWITCH CAPACIDADE ////
					_delay_ms(800);
					if(!(PINB&(1<<2))){
					_delay_ms(800);
					}		
					lcd_clrscr();
					sw11 = 0;
					sw22 = 0;
					PORTB |= (1 << 3); // Começa teste!!!
					lcd_home();
					lcd_puts("A iniciar teste");
					_delay_ms(8000);
					lcd_gotoxy(6, 1);
					lcd_puts(".");
					_delay_ms(8000);
					lcd_gotoxy(7, 1);
					lcd_puts(".");
					_delay_ms(8000);
					lcd_gotoxy(8, 1);
					lcd_puts(".");
					_delay_ms(8000);
					lcd_clrscr();
					teste = 1;
					inicio = 0;
					sw11 = 0;
					sw22 = 0;
					sw33 = 0;
					comecoms = millis_get(); // Obtem milisegundos desde o arranque do microcontrolador		
		}		
		}

		//////////////////////////////////// FIM DO INICIO ////////////////////////////////////
		//////////////////////////////////// COMEÇA TESTE //////////////////////////////////////
		
      while ((teste == 1) && (inicio == 0)) {      		
			if (Vload==0)
			{	
			  	sprintf(buffer_tx,"------TESTE INICIADO------\r\n");
			  	TX_buffer(); 						
		  	}

			///////////// A0 ////////////
			lcd_home();
			dtostrf(voltage_a0,3,2,buffer_a0);
			lcd_puts(buffer_a0);
			
			lcd_gotoxy(5,0);
			lcd_puts("V");
			///////////// A1 ////////////
			current = ( voltage_a1)*1000 ; //Formula Corrente em mA!!!
			lcd_gotoxy(10, 0);	
			dtostrf(current,3,0,buffer_current);
			lcd_puts(buffer_current);
			
			lcd_gotoxy(14,0);
			lcd_puts("mA");	
			///////////// CAPACIDADE ////////////
			runMillis = millis_get() - comecoms;
			temporizadorms = millis_get() - comecoms;
			millisPassed=runMillis-previousMillis;
			capacidade = (capacidade + (current) * (millisPassed / 3600000.0)); // mAh = mAh + mAmperes * Horas
			previousMillis=runMillis;
			minutos= temporizadorms / 60000; // Calcula min
			
			dtostrf(capacidade,4,0,buffer_capacidade);
			lcd_gotoxy(0, 1);
			lcd_puts(buffer_capacidade);
			lcd_gotoxy(5,1);
			lcd_puts("mAh"); // Mostra acumulador de capacidade ate o teste acabar
			///////////// TEMPERATURA ////////////
			lcd_gotoxy(10, 1);
			dtostrf(temp_a2,3,0,buffer_temp);
			lcd_puts(buffer_temp);
			lcd_gotoxy(14,1);
			lcd_putc(223);
			lcd_gotoxy(15,1);
			lcd_puts("C");
			
			res_term = log(((10240000/adc_result_a3) - 10000)); //res_term =  R_7.4k * (ADC_1024 / adc_result3)-4.7k)
			temp_term = 1 / (0.001129148 + (0.000234125 * res_term) + (0.0000000876741 * res_term * res_term * res_term));
			temp_term = temp_term - 273.15;           // Convert Kelvin to Celcius
			
			//lcd_gotoxy(12, 1);
			//dtostrf(temp_term,2,0,buffer_term);
			//lcd_puts(buffer_term);
			
			if (temp_term > 70)
			{
				teste = 0;
				PORTC &= ~(1 << 4); // LED verde desliga
				PORTC |= (1<<5); // LED vermelho liga
				lcd_clrscr();
				lcd_gotoxy(2, 0);
				lcd_puts("TEMPERATURA");
				lcd_gotoxy(4, 1);
				lcd_puts("ELEVADA");
				//////////////////Comunicação pela porta serie ///////////////////
				sprintf(buffer_tx,"------TESTE INTERROMPIDO------\r\n");
				TX_buffer();
				/////////////////////////////////////
				_delay_ms(5000);
				lcd_clrscr();
				apagar_variaveis();
				PORTC &= ~(1 << 5); // LED vermelho desliga
				main();
			}
		
				if (voltage_a0 < V_cutoff) // TESTA SE FOR TIRADA A PILHA
				{    
				teste=0;
				lcd_clrscr();
				_delay_ms(16000);
				while(1){
						if (voltage_a0 < 0.3) //<       /////////////// TIRARAM PILHA ///////////////
								{
								  PORTC &= ~(1 << 4); // LED verde desliga
								  PORTC |= (1<<5); // LED vermelho liga
	 							  PORTB &= ~(1 << 3); // Pin 3 goes low - ACABA TESTE!!!!
								  //////////////////Comunicação pela porta serie ///////////////////
								  sprintf(buffer_tx,"------TESTE INTERROMPIDO------\r\n");
								  TX_buffer();
								  ////////////////// ///////////////////
								  lcd_gotoxy(6, 0);
								  lcd_puts("ERRO");
								  lcd_gotoxy(2, 1);
								  lcd_puts("Insira Pilha");
								  _delay_ms(80000);
								  lcd_clrscr();
								  apagar_variaveis();
								  PORTC &= ~(1 << 5); // LED vermelho desliga
								  main();
 									 }
      								else {    //////////////// TESTE COMPLETO //////////////////
										lcd_clrscr();
	  									PORTC |= 0b00010000;	//LED verde ligado
	   									PORTB &= ~(1 << PB3); // ACABA TESTE!!!!
	   									acum = 1;
										percent = (capacidade/cap)*100; // percentagem = (capacidade resultante do teste / capacidade inserida) * 100
										R_int = ((((Voc - Vload)))/Vload)*1000; // Calcula resistência interna
										dtostrf(R_int,3,0,buffer_rint);
	    								lcd_gotoxy(1, 0);
	   									lcd_puts("TESTE COMPLETO");
										lcd_gotoxy(0, 1);
									    dtostrf(capacidade,4,0,buffer_capacidade);
	 									lcd_puts(buffer_capacidade);
									    lcd_gotoxy(5,1);
									    lcd_puts("mAh");
									    lcd_gotoxy(11,1);
									    dtostrf(percent,2,0,buffer);
									    lcd_puts(buffer);
										lcd_gotoxy(14,1);
										lcd_puts("%");
										//////////////////Comunicação pela porta serie ///////////////////
										sprintf(buffer_tx,"------TESTE COMPLETO------\r\n");
										TX_buffer();
										sprintf(buffer_tx,"%s mAh | %d min | %s mOhm | %s 'C\r\n",buffer_capacidade, minutos, buffer_rint, buffer_temp);
										TX_buffer();
										/////////////////////////////////////////////////////////////////
										while (teste == 0 && inicio == 0) { ///////////// PERCORRE OS 2 ECRAS /////////////////
											if ((PINB&(1<<0))) ///////////// PRIMEIRO SWITCH //////////////
											{
											_delay_ms(1600);
											if(!(PINB&(1<<0))){ 
											_delay_ms(1600);
											}
											acum = acum % 2;
											switch (acum) {
													case 0:
													lcd_clrscr();
													lcd_gotoxy(1, 0);  
													lcd_puts("TESTE COMPLETO");
													lcd_gotoxy(0, 1);
													dtostrf(capacidade,4,0,buffer);
													lcd_puts(buffer);
													lcd_gotoxy(5,1);
													lcd_puts("mAh");
													lcd_gotoxy(9,1);
													dtostrf(percent,4,0,buffer);
													lcd_puts(buffer);
													lcd_gotoxy(14,1);
													lcd_puts("%");
													acum++;
													_delay_ms(4800);
													break;

													case 1:
													lcd_clrscr();
													lcd_home(); 
													dtostrf(R_int,4,0,buffer_rint);
													lcd_puts(buffer_rint);
													lcd_gotoxy(5,0);
													lcd_puts("mOhm");   // Mostra Resistencia Interna
													lcd_gotoxy(1, 1);
													dtostrf(minutos,3,0,buffer);
													lcd_puts(buffer);
													lcd_gotoxy(5,1);
													lcd_puts("min."); // Mostra minutos
													dtostrf(temp_a2,3,0,buffer);
													lcd_gotoxy(11, 1);
													lcd_puts(buffer); // Mostra temperatura
													lcd_gotoxy(14,1);
													lcd_putc(223);
													lcd_gotoxy(15,1);
													lcd_puts("C");
													acum++;
													_delay_ms(4800);
													break;
												 }
								}
								
							if ((PINB & (1<<2)) && (V_cutoff != 0) ) { //////////////// SEGUNDO SWITCH /////////
							_delay_ms(1600);
							if(!(PINB&(1<<2))){
										_delay_ms(1600);
										}
										lcd_clrscr();
										apagar_variaveis();
										main();
							}
						}
					}
				}
			}
		}
	}
}