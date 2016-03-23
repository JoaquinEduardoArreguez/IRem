// decoderPruebaInterruptsyTimer.ino
/*Recursos Usados:
	PB1		-> Led como monitor
	PD3		-> Lectura del receptor IR
	INT1 	-> Interrupción por hardware de alto nivel en pin PE4
	TIMER2 	-> Timer que determina el tiempo entre lecturas
	*/

#define F_CPU 16000000UL

#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/eeprom.h>

#define IRpin_PIN PIND 		//Definición del pin register a usar
#define IRpin 2 			//Número de pin a usar
#define maxLecturas 70
#define maxVal 450
#define REPEAT 0xffffffff // Decoded value for NEC when a repeat code is received

//*********************** Definiciones
volatile uint16_t cont;
volatile uint16_t Lecturas[maxLecturas];
volatile uint8_t indexLecturas;
volatile uint32_t codigoDecodificado;
volatile uint32_t matchedValue;
//********************** Definiciones

//********************** FLAGS ***********
volatile uint8_t edge;							//false -> Falling; true -> Rising
volatile uint8_t posibleFin;
volatile uint8_t lectura;
uint8_t monitorLedFlag;
//********************** FLAGS ***********


/*		Definiciones de Botones	*/
uint32_t ch = 33473850;

void init_INT1(){
	PORTD = (1 << PD3);		//PULL-UP en pin D3
	DDRD  = (0 << DDD3);		//pin D3 como INPUT
	cli();
	EICRA = (1 << ISC11);	//Trigger on Falling edge
	EIMSK = (1 << INT1);	//External Interrupt Mask para INT1
	sei();
}

void INT1_FALLING(){
	//cli();
	EICRA = (1 << ISC11);	//Trigger on Falling edge
	//sei();
}

void INT1_RISING(){
	//cli();
	EICRA = ((1 << ISC11) | (1 <<ISC10));	//Trigger on Rising edge
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

void apagar_INT1(){
	//cli();
	EIMSK = (0 << INT1);			//External Interrupt Mask para INT1 apagada
	//sei();
}

void encender_INT1(){
	//cli();
	EIMSK = (1 << INT1);
	//sei();
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
		_delay_ms(100);
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
	return (	inicio(Lecturas[1])						&&
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
	//uint8_t i;
	for (i=3;i<cantidadLecturas;i=i+2){					//ciclo que decodifica y guarda en "codigo"
		if (simple(Lecturas[i]) && simple(Lecturas[i+1]))		{codigo[indexCodigo]=0;}
		else if (simple(Lecturas[i]) && doble(Lecturas[i+1]))	{codigo[indexCodigo]=1;}
		indexCodigo++;
	}
	
	//uint8_t i;
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
	encender_INT1();
	encender_TIMER2();
}

int main(){

	posibleFin=0;

	clearLecturas();

	eeprom_update_dword((uint32_t*)0,ch);

	init_INT1();
	init_TIMER2(100);	//TIMER2 cada 50 micro segundos

	monitorLedFlag=0;
	pruebaMonitorLed();

	while(1){

		_delay_ms(10);
		if (lectura){
			decodificar();
			verificarMatch();
			resumeLector();
		}
	}


}

//El timer funciona todo el tiempo, es la única forma de monitorear si terminamos de leer una señal
ISR (TIMER2_COMPA_vect){
	//cli();
	cont++;								//aumento el contador de ciclos

	if (cont>460)	cont=0;		//El contador se hizo muy grande, lo reinicio para no tener valores tan altos.

	if (cont>maxVal && posibleFin){		//si hay overflow en el contador de ciclos y estamos parados sobre
		lectura=1;					//un posible fin (la última interrupción en el pin fue por "rising")
		posibleFin=0;				//entonces efectivamente terminamos de leer una señal
		apagar_INT1();
		apagar_TIMER2();
	}									//hasta que esté procesada la última lectura.
	//sei();
}

ISR(INT1_vect){
	//cli();
	Lecturas[indexLecturas]=cont;				//guardo la cantidad de ciclos que cumplió el timer
	cont=0;								//reinicio el contador
	indexLecturas++;							//aumento en 1 el índice


	if (indexLecturas>=maxLecturas){
		//indexLecturas=0;	//Si el indice es >= maxLecturas, lo vuelvo a cero.
		apagar_INT1();
		posibleFin=1;
	}	


	edge=!edge;							//cambio el edge, para saber cuál es el que sigue
	if(edge) INT1_RISING();				//Si edge==true la próxima interrupción debe ser Rising
	if(!edge) {							//Si el que sigue es Falling, la interr en que estamos se dio por Rising
		posibleFin=1;				// y si se dio por rising, puede ser la última variación de una señal.
		INT1_FALLING();					//Si edge==false la prox interrupción debe ser Rising
	}
	//sei();
}
