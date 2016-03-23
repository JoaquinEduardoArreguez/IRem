#ifndef IRFemDecoder_h
#define IRFemDecoder_h
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
#endif
