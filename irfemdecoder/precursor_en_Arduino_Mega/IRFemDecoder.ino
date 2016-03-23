// decoderPruebaInterruptsyTimer.ino
/*Recursos Usados:
	PB1		-> Led como monitor
	PE4		-> Lectura del receptor IR
	INT4 	-> Interrupción por hardware de alto nivel en pin PE4
	TIMER2 	-> Timer que determina el tiempo entre lecturas
	*/

#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/eeprom.h>

#define IRpin_PIN PINE
#define IRpin 4
#define maxLecturas 70
#define maxVal 450
#define REPEAT 0xffffffff // Decoded value for NEC when a repeat code is received

//*********************** Definiciones
volatile uint16_t cont;
volatile boolean edge;							//false -> Falling; true -> Rising
volatile uint16_t Lecturas[maxLecturas];
volatile uint8_t index;
boolean monitorLedFlag;
volatile boolean posibleFin;
volatile boolean lectura;
volatile uint32_t codigoDecodificado;
volatile uint32_t matchedValue;
//********************** Definiciones

/*		Definiciones de Botones	*/
uint32_t ch = 33473850;

//Se mueve por todos los valores almacenados en la eeprom hasta que encuentre uno
//que matchee o los verifique a todos. Si alguno matchea, ese valor se guarda en matchedValue.
//Devuelve true si matchea.
boolean verificarMatch(){
	for (uint8_t i=0;i<88;i=i+4){
		if (codigoDecodificado == eeprom_read_dword((uint32_t*)i)) {
			pruebaMonitorLed();
			return true;
		}
	}

	return false;

	//if (codigoDecodificado == eeprom_read_dword((uint32_t*)0)) pruebaMonitorLed();
	//if (codigoDecodificado == ch) pruebaMonitorLed();
}

boolean simple(uint8_t x){return (5<x && x<15);}
boolean doble (uint8_t x){return (27<x && x<40);}
boolean inicio(uint8_t x){return (165<x && x<185);}
boolean repeticion() {
	return (	inicio(Lecturas[1])						&&
				(Lecturas[2]<50 && Lecturas[2]>38)		&&
				simple(Lecturas[3])
				);
}
//boolean inicio(uint8_t x){return ()}

uint8_t  compare (uint8_t oldval, uint8_t newval){
	if      (newval < oldval * .8)  return 0 ;
	else if (oldval < newval * .8)  return 2 ;
	else                       return 1 ;
}

void clearLecturas(){
	for(uint8_t i=0;i<maxLecturas;i++){
		Lecturas[i]=9;
	}
}

void decodificar(){		//Leer Info_ProtocoloNEC

	uint8_t cantidadLecturas=index+1;	//--> Cantidad de lecturas que hay
	uint8_t codigo [cantidadLecturas];
	uint8_t indexCodigo=0;

	for (uint8_t i=0;i<cantidadLecturas;i++){codigo[i]=9;}	//Limpio los valores de "codigo"

	//Chequeo si es una repeticion
	if (repeticion()) {codigoDecodificado=REPEAT;}
	else {
	//anula los primeros tres valores
	for (uint8_t i=3;i<cantidadLecturas;i=i+2){					//ciclo que decodifica y guarda en "codigo"
		if (simple(Lecturas[i]) && simple(Lecturas[i+1]))		{codigo[indexCodigo]=0;}
		else if (simple(Lecturas[i]) && doble(Lecturas[i+1]))	{codigo[indexCodigo]=1;}
		indexCodigo++;
	}

	for(uint8_t i=0;i<indexCodigo;i++){
		if (codigo[i] == 0) {codigoDecodificado = ((codigoDecodificado << 1) | (0));}
		if (codigo[i] == 1) {codigoDecodificado = ((codigoDecodificado << 1) | (1));}
	}
	}

	Serial.print("\n");
	Serial.print(codigoDecodificado,DEC);

	//codigoDecodificado=0;

	clearLecturas();
}

void resumeLector(){
	lectura=false;
	codigoDecodificado=0;
	clearLecturas();
	index=0;
	encender_INT4();
	encender_TIMER2();
}

void monitorLed(){
	PORTB = (0 << PB1);		//PB1 en LOW, paso intermedio para pasar a HIGH
	DDRB  = (1 << DDB1);	//PB1 como OUTPUT
	__asm__("nop\n\t"); 	//Dejo pasar un ciclo
	if (!monitorLedFlag){	//Si el flag=flase -> Led apagado
		PORTB = (1 << PB1);		//PB1 en HIGH
	}
	monitorLedFlag=!monitorLedFlag;
}

void pruebaMonitorLed(){
	for(uint8_t i=0;i<4;i++){
		monitorLed();
		delay(100);
	}
}

void corrector(){
	for(uint8_t i=0;i<=maxLecturas;i++){
		if (170<=Lecturas[i] && Lecturas[i]<=180){Lecturas[i]=175;}
		else if (40<=Lecturas[i] && Lecturas[i]<=50){Lecturas[i]=44;}
		else if (8<=Lecturas[i] && Lecturas[i]<=15){Lecturas[i]=11;}
		else if (30<=Lecturas[i] && Lecturas[i]<=35){Lecturas[i]=33;}
	}
}

void mostrarLecturas(){
	Serial.println();
	corrector();
	for(uint8_t i=0;i<=maxLecturas-4;i=i+4){
		Serial.print(Lecturas[i],DEC);Serial.print("\t\t");
		Serial.print(Lecturas[i+1],DEC);Serial.print("\t\t");
		Serial.print(Lecturas[i+2],DEC);Serial.print("\t\t");
		Serial.print(Lecturas[i+3],DEC);Serial.print("\n");
	}
}

void init_INT4(){
	PORTE = (1 << PE4);		//PULL-UP en pin E4
	DDRE  = (0 << DDE4);	//pin E4 como INPUT
	cli();
	EICRB = (1 << ISC41);	//Trigger on Falling edge
	EIMSK = (1 << INT4);	//External Interrupt Mask para INT4
	sei();
}

void INT4_FALLING(){
	//cli();
	EICRB = (1 << ISC41);	//Trigger on Falling edge
	//sei();
}

void INT4_RISING(){
	//cli();
	EICRB = ((1 << ISC41) | (1 <<ISC40));	//Trigger on Rising edge
	//sei();
}

void init_TIMER2(uint8_t ocr2a){
	cli();					//Apago todas las interrupciones
	TCCR2B = 0;				//Apago el TIMER2
	TIFR2  = 0;				//Reseteo el Timer/counter Interrupt Flag Register por las dudas

	//Configuro el TIMER2:
	TCCR2A = 0b00000010;	//CTC Mode
	OCR2A  = ocr2a;			//Output Compare Register 2 A, valor con que se va a comparar TCNT2
	TIMSK2 = 0b00000010;	//Timer/Counter2 Output Compare Match A Interrupt Enable
	TCNT2  = 0;				//El contador inicia en 0

	TCCR2B = 0b00000010;	//Preescaler 8, y dandole al preescaler un valor !=0 se inicial el timer
	sei();					//Enciendo todas las interrupciones de nuevo
}

void apagar_TIMER2(){
	//cli();
	TCCR2B=0;
	//sei();
}

void encender_TIMER2(){
	//cli();
	TCCR2B = 0b00000010;	//Preescaler 8, y dandole al preescaler un valor !=0 se inicial el timer
	//sei();
}

void apagar_INT4(){
	//cli();
	EIMSK = (0 << INT4);			//External Interrupt Mask para INT4 apagada
	//sei();
}

void encender_INT4(){
	//cli();
	EIMSK = (1 << INT4);
	//sei();
}

void setup() {
	Serial.begin(9600);

	posibleFin=false;

	clearLecturas();

	eeprom_update_dword((uint32_t*)0,ch);

	init_INT4();
	init_TIMER2(100);	//TIMER2 cada 50 micro segundos

	monitorLedFlag=false;
	pruebaMonitorLed();
}

void loop() {
	delay(10);
	if (lectura){
		decodificar();
		verificarMatch();
		resumeLector();
	}
}

//El timer funciona todo el tiempo, es la única forma de monitorear si terminamos de leer una señal
ISR (TIMER2_COMPA_vect){
	//cli();
	cont++;								//aumento el contador de ciclos

	if (cont>460)	cont=0;		//El contador se hizo muy grande, lo reinicio para no tener valores tan altos.

	if (cont>maxVal && posibleFin){		//si hay overflow en el contador de ciclos y estamos parados sobre
		lectura=true;					//un posible fin (la última interrupción en el pin fue por "rising")
		posibleFin=false;				//entonces efectivamente terminamos de leer una señal
		apagar_INT4();
		apagar_TIMER2();
	}									//hasta que esté procesada la última lectura.
	//sei();
}

ISR(INT4_vect){
	//cli();
	Lecturas[index]=cont;				//guardo la cantidad de ciclos que cumplió el timer
	cont=0;								//reinicio el contador
	index++;							//aumento en 1 el índice


	if (index>=maxLecturas){
		//index=0;	//Si el indice es >= maxLecturas, lo vuelvo a cero.
		apagar_INT4();
		posibleFin=true;
	}	


	edge=!edge;							//cambio el edge, para saber cuál es el que sigue
	if(edge) INT4_RISING();				//Si edge==true la próxima interrupción debe ser Rising
	if(!edge) {							//Si el que sigue es Falling, la interr en que estamos se dio por Rising
		posibleFin=true;				// y si se dio por rising, puede ser la última variación de una señal.
		INT4_FALLING();					//Si edge==false la prox interrupción debe ser Rising
	}
	//sei();
}