#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

void config_USART(void){
  UBRR0=103;//9600 baudios
  UCSR0B|=(1<<TXEN0);
  UCSR0C|=(1<<UCSZ01)|(1<<UCSZ00);
}

void enviar_char(char c){
  while(!(UCSR0A&(1<<UDRE0)));
  UDR0=c; 
}

void enviar_texto(const char* texto){
  while(*texto){
    enviar_char(*texto++);
  }
}

void float_a_texto(float valor){
  int parte_entera=(int)valor;
  int parte_decimal=(int)((valor-parte_entera)*10);

  if(parte_entera>=100) enviar_char('0'+(parte_entera/100));
  if(parte_entera>=10) enviar_char('0'+((parte_entera/10)%10));
  else enviar_char('0');
  enviar_char('0'+(parte_entera%10));
  enviar_char('.');
  enviar_char('0'+parte_decimal);
}

void config_ADC(void){
  ADMUX|=(1<<REFS0);
  ADCSRA|=(1<<ADEN)|(1<<ADPS2);
}

unsigned int leer_ADC(unsigned char canal){
  ADMUX=(ADMUX&0xF0)|(canal&0x0F);
  ADCSRA|=(1<<ADSC);
  while(ADCSRA&(1<<ADSC));
  return ADC;
}

float voltaje_ADC(unsigned int adc){
  return adc*(5.0/1023.0);
}

float temp_deseada(float volt){
  return 20.0+volt*26.0;//0V=20°C, 5V=150°C(potenciómetro)
}

float temp_medida(float volt){
  return 50 * volt - 100;
}



void config_timer(void){
  TCCR0A|=(1<<WGM01);
  TCCR0B|=(1<<CS01);
}

void regulador(void){
  OCR0A=20;
  TCNT0=0;
  while(!(TIFR0&(1<<OCF0A)));
  TIFR0|=(1<<OCF0A);
}

unsigned char b=50;

void pwm(unsigned char brillo){
  PORTB|=0x01;
  for(int i=0; i<100+brillo; i++)regulador();
  PORTB&=~0x01;
  for(int i=0; i<1900-brillo; i++)regulador();
}

void config(void){
  EICRA|=(1<<ISC00)|(1<<ISC01); //Se activa con el flanco de bajada
  EIMSK|=(1<<INT0);
  DDRD&=~(0X04);
}

int a=0;
ISR(INT0_vect){
  if (a>60){
    a=0;
    PORTB^=0x01;
   }
 a++;
}

int main(void){
  DDRB|=(1<<PB0);//PB0 como salida (foco)
  config_USART();
  config_ADC();
  config_timer();

  while(1){
    float volt_deseada=voltaje_ADC(leer_ADC(0));//PC0=potenciómetro
    float volt_medida=voltaje_ADC(leer_ADC(1));//PC1=PT100
    float temp_ref=temp_deseada(volt_deseada);
    float temp_pt=temp_medida(volt_medida);

    pwm(b);//control del foco
    enviar_texto("Deseada:");
    float_a_texto(temp_ref);
    enviar_texto("C | Medida:");
    float_a_texto(temp_pt);
    enviar_texto("C\r\n");
    _delay_ms(100);//tiempo envio
  }
}