// decoderPruebaInterruptsyTimer.ino
/*Recursos Usados:
	PB1		-> Led como monitor
	PC0		-> Lectura del receptor IR
	PCINT8 	-> Interrupción de medio nivel en pin PC0
	TIMER2 	-> Timer que determina el tiempo entre lecturas
	*/
	
#define F_CPU 16000000UL

#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/eeprom.h>

#define maxLecturas 70
#define maxVal 450
#define REPEAT 0xffffffff // Decoded value for NEC when a repeat code is received

//*********************** Definiciones
volatile uint16_t cont;
volatile uint16_t Lecturas[maxLecturas];
volatile uint8_t indexLecturas;
volatile uint32_t codigoDecodificado;
//********************** Definiciones

//********************** FLAGS ***********
volatile uint8_t posibleFin;
volatile uint8_t lectura;
uint8_t monitorLedFlag;
//********************** FLAGS ***********
uint8_t aumentarEepromPointer(uint8_t i);
void valuarBotones();
void init_PCINT8();
void apagar_PCINT8();
void encender_PCINT8();
void init_TIMER2(uint8_t ocr2a);
void apagar_TIMER2();
void encender_TIMER2();
void monitorLed();
void pruebaMonitorLed();
uint8_t verificarMatch();
uint8_t simple(uint8_t x);
uint8_t doble (uint8_t x);
uint8_t inicio(uint8_t x);
uint8_t repeticion();
void clearLecturas();
void decodificar();
void resumeLector();
void inicializarSistema();
ISR (TIMER2_COMPA_vect);
ISR(PCINT1_vect);
