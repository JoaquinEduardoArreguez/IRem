// decoderPruebaInterruptsyTimer.ino
/*Recursos Usados:
	PB1		-> Led como monitor
	PC0		-> Lectura del receptor IR
	PCINT8 	-> Interrupción de medio nivel en pin PC0
	TIMER2 	-> Timer que determina el tiempo entre lecturas
	*/

#define F_CPU 16000000UL

//#include "IRFemDecoder.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/eeprom.h>

#define maxLecturas 70
#define maxVal 450
#define REPEAT 0xFF2FFFFF // Decoded value for NEC when a repeat code is received

//*********************** Definiciones
volatile uint16_t cont;				//Dentro de ISR (TIMER2_COMPA_vect)
volatile uint16_t Lecturas[maxLecturas];	//Dentro de ISR(PCINT8_vect)
volatile uint8_t indexLecturas;			//Dentro de ISR(PCINT8_vect)
uint32_t codigoDecodificado;		//No pertenece a ninguna ISR
//********************** Definiciones

//********************** FLAGS ***********
volatile uint8_t posibleFin;	//Dentro de ISR(PCINT8_vect)
volatile uint8_t lectura;	//Dentro de ISR (TIMER2_COMPA_vect)
uint8_t monitorLedFlag;
//********************** FLAGS ***********

/*		Definiciones de Botones			*/
/*
uint8_t aumentarEepromPointer(uint8_t i){return (i+4);}
void valuarBotones(){
	uint8_t i=0;
	eeprom_update_dword((uint32_t*)i,0x1FF44BA); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FEC53A); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FFC43A); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FE45BA); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FE05FA); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FF847A); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FFC03E); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FF50AE); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FF20DE); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FED12E); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FF30CE); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FF609E); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FE619E); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FE31CE); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FEF50A); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FE21DE); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FE718E); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FEB54A); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FE857A); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FE956A); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0x1FEA55A); i=aumentarEepromPointer(i);
	eeprom_update_dword((uint32_t*)i,0xFF2FFFFF); i=aumentarEepromPointer(i);
}*/
/*		Definiciones de Botones			*/

void init_PCINT8(){
	PORTC = (1 << PC0);		//PULL-UP en pin C0
	DDRC  = (0 << DDC0);		//pin C0 como INPUT
	cli();
	PCICR = (1 << PCIE1);	//Pin Change Interrupt Control Register -> enabled de 14:8
	PCMSK1 = (1 << PCINT8);	//PCMSK1 – Pin Change Mask Register 1;Pin Change Enable Mask 14...8
	sei();
}
void apagar_PCINT8()	{ PCMSK1 = (0 << PCINT8);}
void encender_PCINT8()	{ PCMSK1 = (1 << PCINT8);}

void init_TIMER2(uint8_t ocr2a){
	cli();					//Apago todas las interrupciones
	TCCR2B = 0;				//Apago el TIMER2
	TIFR2  = 0;				//Reseteo el Timer/counter Interrupt Flag Register por las dudas

	//Configuro el TIMER2:
	TCCR2A = 0b00000010;	//CTC Mode
	OCR2A  = ocr2a;		//Output Compare Register 2 A, valor con que se va a comparar TCNT2
	TIMSK2 = 0b00000010;	//Timer/Counter2 Output Compare Match A Interrupt Enable
	TCNT2  = 0;		//El contador inicia en 0

	TCCR2B = 0b00000010;	//Preescaler 8, y dandole al preescaler un valor !=0 se inicial el timer
	sei();					//Enciendo todas las interrupciones de nuevo
}

void apagar_TIMER2(){
	
	TCCR2B=0;
	
}

void encender_TIMER2(){
	
	TCCR2B = 0b00000010;	//Preescaler 8, y dandole al preescaler un valor !=0 se inicial el timer
	
}


void monitorLed(){
	PORTB = (0 << PB0);	//PB0 en LOW, paso intermedio para pasar a HIGH
	DDRB  = (1 << DDB0);	//PB0 como OUTPUT
	__asm__("nop\n\t"); 	//Dejo pasar un ciclo
	if (!monitorLedFlag){	//Si el flag=flase -> Led apagado
		PORTB = (1 << PB0);		//PB0 en HIGH
	}
	monitorLedFlag=!monitorLedFlag;
}

void pruebaMonitorLed(){
	uint8_t i;
	for(i=0;i<4;i++){
		monitorLed();
		_delay_ms(10);
	}
}

//Se mueve por todos los valores almacenados en la eeprom hasta que encuentre uno
//que matchee o los verifique a todos. Si alguno matchea, ese valor se guarda en matchedValue.
//Devuelve true si matchea.
uint8_t verificarMatch(){
	uint8_t i;
	for (i=0;i<88;i=i+4){
		if (codigoDecodificado == eeprom_read_dword((uint32_t*)i)) {
			pruebaMonitorLed();
			return 1;
		}
	}
	return 0;
}

uint8_t simple(uint8_t x){return (5<x && x<15);}
uint8_t doble (uint8_t x){return (27<x && x<40);}
uint8_t inicio(uint8_t x){return (165<x && x<185);}
uint8_t repeticion() {
	return (	inicio(Lecturas[1])				&&
			(Lecturas[2]<50 && Lecturas[2]>38)		&&
			simple(Lecturas[3])
				);
}

void clearLecturas(){
	uint8_t i;
	for(i=0;i<maxLecturas;i++){
		Lecturas[i]=9;
	}
}

void decodificar(){		//Leer Info_ProtocoloNEC

	uint8_t cantidadLecturas=indexLecturas+1;	//--> Cantidad de lecturas que hay
	uint8_t codigo [cantidadLecturas];
	uint8_t indexCodigo=0;
	uint8_t i;
	for (i=0;i<cantidadLecturas;i++){codigo[i]=9;}	//Limpio los valores de "codigo"

	//Chequeo si es una repeticion
	if (repeticion()) {codigoDecodificado=REPEAT;}
	else {
	//anula los primeros tres valores
	for (i=3;i<cantidadLecturas;i=i+2){					//ciclo que decodifica y guarda en "codigo"
		if (simple(Lecturas[i]) && simple(Lecturas[i+1]))		{codigo[indexCodigo]=0;}
		else if (simple(Lecturas[i]) && doble(Lecturas[i+1]))	{codigo[indexCodigo]=1;}
		indexCodigo++;
	}
	
	for(i=0;i<indexCodigo;i++){
		if (codigo[i] == 0) {codigoDecodificado = ((codigoDecodificado << 1) | (0));}
		if (codigo[i] == 1) {codigoDecodificado = ((codigoDecodificado << 1) | (1));}
	}
	}
	clearLecturas();
}

void resumeLector(){
	lectura=0;
	codigoDecodificado=0;
	clearLecturas();
	indexLecturas=0;
	encender_PCINT8();
	encender_TIMER2();
}

void inicializarSistema(){
	//valuarBotones();

	posibleFin=0;

	clearLecturas();

	init_PCINT8();
	init_TIMER2(100);	//TIMER2 cada 50 micro segundos

	monitorLedFlag=0;
	pruebaMonitorLed();
}

//El timer funciona todo el tiempo, es la única forma de monitorear si terminamos de leer una señal
ISR (TIMER2_COMPA_vect){
	
	cont++;								//aumento el contador de ciclos

	if (cont>460)	cont=0;		//El contador se hizo muy grande, lo reinicio para no tener valores tan altos.

	if (cont>maxVal && posibleFin){		//si hay overflow en el contador de ciclos y estamos parados sobre
		lectura=1;					//un posible fin (la última interrupción en el pin fue por "rising")
		posibleFin=0;				//entonces efectivamente terminamos de leer una señal
		apagar_PCINT8();
		apagar_TIMER2();
	}									//hasta que esté procesada la última lectura.
	
}

ISR(PCINT1_vect){
	Lecturas[indexLecturas]=cont;				//guardo la cantidad de ciclos que cumplió el timer
	cont=0;								//reinicio el contador
	indexLecturas++;							//aumento en 1 el índice
	posibleFin=1;


	if (indexLecturas>=maxLecturas){
		indexLecturas=0;	//Si el indice es >= maxLecturas, lo vuelvo a cero.
		posibleFin=1;
	}
}
