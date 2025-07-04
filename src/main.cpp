#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
char c=0;

void configtimer_2(void){ // para el pid
  TCCR2A|=(1<<WGM21);
  TCCR2B|=(1<<CS22);
  TIMSK2|=(1<<OCIE2A);
  OCR2A=250;
}

volatile unsigned long time_ms=0;
volatile float dt=0;

ISR(TIMER2_COMPA_vect){
  time_ms++;
  TCNT2=0;
}

void config_USART(void){
  UBRR0=103;//9600 baudios
  UCSR0B|=(1<<TXEN0);
  UCSR0C|=(1<<UCSZ01)|(1<<UCSZ00);
}

//void enviar_char(char c){
 // while(!(UCSR0A&(1<<UDRE0)));
 // UDR0=c; 
//}

void enviar_texto(const char* texto){
  while(*texto){
    //enviar_char(*texto++);
    c=*texto++;
    UCSR0B|=(1<<UDRIE0);    
     _delay_ms(10);
  }
}

ISR(USART_UDRE_vect){
  UDR0=c;
  UCSR0B&=~(1<<UDRIE0);
}
void float_a_texto(float valor){
  int parte_entera=(int)valor;
  int parte_decimal=(int)((valor-parte_entera)*10);
  UCSR0B|=(1<<UDRIE0);
  if(parte_entera>=100) c='0'+(parte_entera/100);
   UCSR0B|=(1<<UDRIE0);
  if(parte_entera>=10) c=('0'+((parte_entera/10)%10));
  else c=('0');
  UCSR0B&=~(1<<UDRIE0);
  _delay_ms(8);
  UCSR0B|=(1<<UDRIE0);
  c=('0'+(parte_entera%10));
  UCSR0B&=~(1<<UDRIE0);
  _delay_ms(8);
  UCSR0B|=(1<<UDRIE0);
  c=('.');
  UCSR0B&=~(1<<UDRIE0);
  _delay_ms(8);
  UCSR0B|=(1<<UDRIE0);
  c=('0'+parte_decimal);
  UCSR0B&=~(1<<UDRIE0);
  _delay_ms(8);
  UCSR0B|=(1<<UDRIE0);
}

void config_ADC(void){ //para potenciometro
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
  return 50*volt-100;
}

void config_timer(void){
  TCCR0A|=(1<<WGM01);
  TCCR0B|=(1<<CS02)|(1<<CS00);
}

void regulador(void){
  OCR0A=20;
  TCNT0=0;
  while(!(TIFR0&(1<<OCF0A)));
  TIFR0|=(1<<OCF0A);
}

void config_intext(void){ //del foco
  EIMSK|=(1<<INT0);
  EICRA|=(1<<ISC01);
  DDRD&=~(1<<PD2);
}

ISR(INT0_vect){
  PORTB|=0x01;
  TCNT0=0;
  TCCR0B|=(1<<CS02)|(1<<CS00);
  while(!(TIFR0&(1<<OCF0A)));
  TIFR0|=(1<<OCF0A);
  PORTB&=~0x01;
  TCCR0B&=~((1<<CS02)|(1<<CS00));
  UCSR0B|=(1<<UDRIE0);
}

int main(void){
  DDRB|=(1<<PB0);//PB0 como salida (foco)
  config_USART();
  config_ADC();
  config_timer();
  config_intext();
  configtimer_2();
  sei();
  while(1){
    dt=time_ms/1000.0;
    time_ms=0;
    float volt_deseada=voltaje_ADC(leer_ADC(0));//PC0=potenciómetro
    float volt_medida=voltaje_ADC(leer_ADC(1));//PC1=PT100
    float temp_ref=temp_deseada(volt_deseada);
    float temp_pt=temp_medida(volt_medida);

    float error_ant=0;
    float kp=1;
    float kd=1;
    float ki=1;
    float integral=0;
    float derivada=0;
    float error=temp_ref-temp_pt; integral+=error;
    if(dt==0){
      derivada=0;
    }
    else{
      derivada=(error-error_ant)/dt;
    }

    float u=kp*error+kd*derivada+ki*integral*dt;
    error_ant=error;
    if(u<=0){
      u=0;
    }

    float uf=u;
    if(uf>255){
      uf=255;
    }
    OCR0A=(unsigned char)(uf);
    
    enviar_texto("Deseada:");
    float_a_texto(temp_ref);
    enviar_texto("C | Medida:");
    float_a_texto(temp_pt);
    enviar_texto("C | Tiempo:");
    float_a_texto(dt);
    enviar_texto("seg | PID:");
    float_a_texto(u);
    enviar_texto(" | foco:");
    float_a_texto(uf);
    enviar_texto("\r\n");
  }
}