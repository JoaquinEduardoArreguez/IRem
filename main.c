#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  
#define F_CPU 16000000UL
#include <util/delay.h>     

#include <avr/pgmspace.h>   
#include "usbdrv.h"
#include <avr/eeprom.h>

#define maxLecturas 70
#define maxVal 450
#define REPEAT 0xFF2FFFFF // Decoded value for NEC when a repeat code is received


volatile uint16_t cont;
volatile uint16_t Lecturas[maxLecturas];
volatile uint8_t indexLecturas;
uint32_t codigoDecodificado;


volatile uint8_t posibleFin;	
volatile uint8_t lectura;


uint8_t aumentarEepromPointer(uint8_t i){return (i+4);}

/*
void valuarBotones(){
	uint32_t i=0;
	eeprom_update_dword(&i,0x1FF44BA); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FEC53A); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FFC43A); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FE45BA); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FE05FA); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FF847A); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FFC03E); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FF50AE); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FF20DE); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FED12E); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FF30CE); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FF609E); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FE619E); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FE31CE); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FEF50A); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FE21DE); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FE718E); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FEB54A); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FE857A); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FE956A); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0x1FEA55A); i=aumentarEepromPointer(i);
	eeprom_update_dword(&i,0xFF2FFFFF); i=aumentarEepromPointer(i);
}

*/

void init_PCINT0(){
	PORTB = (1 << PB0); 
	DDRB = (0 << DDB0); 

	PCMSK = (1 << PCINT0);	
	GIMSK |= (1 << PCIE);	
}
void apagar_PCINT0()	{ PCMSK = (0 << PCINT0);}
void encender_PCINT0()	{ PCMSK = (1 << PCINT0);}

void init_TIMER1(uint8_t ocr1a){
	TCCR1 = 0;	
	TIFR  = 0;	

	
	
	
	OCR1A = ocr1a;			
	TIMSK = (1 << OCIE1A);		

	TCNT1 = 0;			
	TCCR1 = 0b00000100;		
}

void apagar_TIMER1(){TCCR1=0;}

void encender_TIMER1(){TCCR1 = 0b00000100;}		

uint8_t verificarMatch(){

	uint32_t i;
	for (i=0;i<88;i=i+4){
		if (codigoDecodificado == eeprom_read_dword(&i)) {
			
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

void decodificar(){		

	uint8_t cantidadLecturas=indexLecturas+1;	
	uint8_t codigo [cantidadLecturas];
	uint8_t indexCodigo=0;
	uint8_t i;
	for (i=0;i<cantidadLecturas;i++){codigo[i]=9;}	


		if (repeticion()) {codigoDecodificado=REPEAT;}
	else {

		for (i=3;i<cantidadLecturas;i=i+2){					
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
	encender_PCINT0();
	encender_TIMER1();
}

void inicializarSistema(){
	

	posibleFin=0;

	clearLecturas();

	init_PCINT0();
	init_TIMER1(100);	
}

ISR (TIM1_COMPA_vect){
	
	cont++;								

	if (cont>460)	cont=0;			

	if (cont>maxVal && posibleFin){		
		lectura=1;			
		posibleFin=0;			
		apagar_PCINT0();
		apagar_TIMER1();
	}					
	
	TCNT1 = 0;	
}

ISR(PCINT0_vect){
	Lecturas[indexLecturas]=cont;				
	cont=0;								
	indexLecturas++;							
	posibleFin=1;


	if (indexLecturas>=maxLecturas){
		indexLecturas=0;	
		posibleFin=1;
	}
}

const PROGMEM char usbHidReportDescriptor[95] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x4b,                    //   REPORT_ID (75)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x26, 0x81, 0x00,              //   LOGICAL_MAXIMUM (129)
    0x1b, 0x00, 0x00, 0x07, 0x00,  //   USAGE_MINIMUM (Keyboard:Reserved (no event indicated))
    0x29, 0x81,                    //   USAGE_MAXIMUM (Keyboard Volume Down)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0,                          // END_COLLECTION
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x85, 0x4d,                    //     REPORT_ID (77)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x09, 0x38,                    //     USAGE (Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};
/*[97] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x4b,                    //   REPORT_ID (75)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x1b, 0x00, 0x00, 0x07, 0x00,  //   USAGE_MINIMUM (Keyboard:Reserved (no event indicated))
    0x2b, 0x65, 0x00, 0x07, 0x00,  //   USAGE_MAXIMUM (Keyboard:Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0,                          // END_COLLECTION
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x85, 0x4d,                    //     REPORT_ID (77)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x09, 0x38,                    //     USAGE (Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};*/

typedef struct{
	uint8_t report_id;		//1
	uint8_t modificador;
	uint8_t key;
} report_keyboard_t;

typedef struct{
	uint8_t report_id;		//2
	uint8_t botones;		
	int8_t    X;		
	int8_t    Y;
	int8_t    wheel;
} report_mouse_t;



static report_mouse_t 		mouseReport;
static report_keyboard_t	keyboardReport;
uint8_t sendMouse=0;
uint8_t sendKeyboard=0;

static uint8_t idle_rate = 0;
static uint8_t protocol_version = 0;

uint8_t precisionesMouse[4]={5,15,30,60};	//Precision Mouse 1 = 15, 2 = 30 ...
uint8_t precisionMouse = 1;

uint8_t mouseAKeys=0;

void limpiar_reports(){
	keyboardReport.report_id 	= 75;
	keyboardReport.modificador 	= 0;
	keyboardReport.key 			= 0;

	mouseReport.report_id 	= 77;
	mouseReport.botones 	= 0;
	mouseReport.X 			= 0;
	mouseReport.Y 			= 0;
	mouseReport.wheel 		= 0;
}

void llenar_report_mouse(uint8_t botones_ , uint8_t X_ , uint8_t Y_ , uint8_t wheel_ ){
	mouseReport.botones = botones_;
	mouseReport.X 	 	= X_;
	mouseReport.Y 		= Y_;
	mouseReport.wheel 	= wheel_;
	sendMouse 			= 1;
}

void llenar_report_keyboard(uint8_t modificador_ , uint8_t key_ ){
	keyboardReport.modificador 	= modificador_;
	keyboardReport.key 			= key_;
	sendKeyboard 				= 1;
}



void enviar_report(void *data, uchar len)
{
	usbSetInterrupt(data, len);

	limpiar_reports();

	while(1)							
	{
		wdt_reset(); 
		
		usbPoll();
		if (usbInterruptIsReady())
		{
			usbSetInterrupt(data, len);
			break;
		}
	}
}

usbMsgLen_t usbFunctionSetup(uint8_t data[8])
{
	
	usbRequest_t *rq = (void *)data;

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) != USBRQ_TYPE_CLASS)
		return 0; 

	
	switch (rq->bRequest)
	{
		case USBRQ_HID_GET_IDLE:

		usbMsgPtr = idle_rate;
		return 1; 
		case USBRQ_HID_SET_IDLE:
		idle_rate = rq->wValue.bytes[1]; 
		return 0; 
		case USBRQ_HID_GET_PROTOCOL:

		usbMsgPtr = protocol_version; 
		return 1; 
		case USBRQ_HID_SET_PROTOCOL:
		protocol_version = rq->wValue.bytes[1];
		return 0; 
		case USBRQ_HID_GET_REPORT:

		if (rq->wValue.bytes[0] == 75)
		{
			usbMsgPtr = (intptr_t) &keyboardReport;
			return sizeof(keyboardReport);
		}
		else if (rq->wValue.bytes[0] == 77)
		{
			usbMsgPtr = (intptr_t) &mouseReport;
			return sizeof(mouseReport);
		}
		else
		{
			return 0; 
		}
		case USBRQ_HID_SET_REPORT: 
		return 0; 
		default: 
		return 0; 
	}
}


int main() {

	uint8_t i;

	wdt_enable(WDTO_1S); 

	usbInit();

	usbDeviceDisconnect(); 
	for(i = 0; i<250; i++) { 
		wdt_reset(); 
		_delay_ms(2);
	}
	usbDeviceConnect();

	inicializarSistema();   

	limpiar_reports();

	sei();

	uint32_t codigoAnterior=0;

	keyboardReport.report_id 	= 75;
	mouseReport.report_id 		= 77;

	while(1) {

		wdt_reset();

		if(lectura){
			decodificar();

			if (codigoDecodificado == 0xFF2FFFFF){codigoDecodificado=codigoAnterior;}
			else{codigoAnterior=codigoDecodificado;}

			switch(codigoDecodificado){

            	case 0x1FE718E:               //click
            	llenar_report_mouse( 1 , 0 , 0, 0);
            	break;

                case 0x1FE31CE:               //mouse arriba
                llenar_report_mouse( 0 , 0 , -(precisionesMouse[precisionMouse]) , 0);
                break;

            	case 0x1FEB54A:              //mouse abajo
            	llenar_report_mouse( 0 , (precisionesMouse[precisionMouse]) , 0 , 0);
            	break;

                case 0x1FE956A:               //mouse derecha
                llenar_report_mouse( 0 , 0 , (precisionesMouse[precisionMouse]) , 0);
                break;

                case 0x1FE21DE:               //mouse izquierda
                llenar_report_mouse( 0 , -(precisionesMouse[precisionMouse]) , 0 , 0);
                break;

                case 0x1FEA55A:               //wheel arriba
                llenar_report_mouse( 0 , 0 , 0 , -5);
                break;

                case 0x1FEF50A:               //wheel abajo
                llenar_report_mouse( 0 , 0 , 0 , 5);
                break;



                case 0x1FF50AE:				//Volume +
                llenar_report_keyboard(0b00000000, 0x80);
                break;

                case 0x1FFC03E:				//Volume -
                llenar_report_keyboard(0b00000000, 129);
                break;

                case 0x1FE45BA:				//Play/Pause
                llenar_report_keyboard(0b00000000, 72);
                break;

                case 0x1FFC43A:				// Ch+ (arrow derecha)
                llenar_report_keyboard(0b00000000, 79);
                break;

                case 0x1FF44BA:				//Ch- (arrow izquierda)
                llenar_report_keyboard(0b00000000, 80);
                break;

                case 0x1FF20DE:				// Play / pause con Barra Espaciadora
                llenar_report_keyboard(0b00000000, 44);
                break;
                
                case 0x1FE857A:				//(7) Alt + F4
                llenar_report_keyboard(0b00000100, 61);
                break;

                case 0x1FED12E:				// 200+ (Menos Precision Mouse)
                if (precisionMouse<3) precisionMouse++;
                break;

                case 0x1FF30CE:				// 100+ (Más precision Mouse)
                if (precisionMouse>0) precisionMouse--;
                break;

                case 0x1FF609E:				// 0, mouse a Keys
                if (mouseAKeys) mouseAKeys=0;
                else if (!mouseAKeys) mouseAKeys=1;
                break;
/*
                case :				//
                llenar_report_keyboard(0b00000000, 0x);
                break;
*/








            }
            
            resumeLector();
        }

        wdt_reset();

    	usbPoll();

    	if (usbInterruptIsReady()){
    		
    		if (sendMouse){
    			enviar_report((void*)&mouseReport,sizeof(mouseReport));
    			sendMouse=0;
    		}

    		if (sendKeyboard){
    			enviar_report((void*)&keyboardReport,sizeof(keyboardReport));
    			sendKeyboard=0;
    		}

    	}

    }	// Fin del loop

    return 0;		// Fin del programa, nunca debe alcanzarse
}
