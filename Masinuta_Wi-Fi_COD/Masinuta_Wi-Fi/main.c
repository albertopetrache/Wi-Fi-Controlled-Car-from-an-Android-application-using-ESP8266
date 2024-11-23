#define F_CPU 14745600 // frecventa de ceas
#define BAUD 115200    // rata de baud

#define LCD_dir  DDRA
#define LCD_port PORTA
#define RS PA0	//pinul RS (Register Select)
#define EN PA1 	//pinul EN (Enable)

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <avr/interrupt.h>

#define HC_SR04_NUM_ITERATIONS 		1
static int flag = 0;
static int count = 0;
static int saved_tcnt0 = 0;
static int saved_count = 0;

// 1 daca am primit comenzi
static int date_primite = 0;
// size-ul bufferului
static int rx_size=50;
// buffer in care salvez mesajele primite de la modulul wireless
static char rx_buffer[50];
// var care marcheaza finalul mesajului primit
static int index_buffer = 0;
static char received_byte;

static int inainte=0;
static int inapoi=0;
// 1 daca masina ia un viraj, 0 altfel
static int drifting = 0;
static int direction = -1; // 0 = viraj stanga, 1 = viraj dreapta
// factorul vechi de umplere al motorului pana sa ia virajul
static int saved = 0;
static int avarii = 0;
static int sem_st=0;
static int sem_dr=0;


void Comanda_LCD(unsigned char comanda)
{
	LCD_port = (LCD_port & 0x0F) | (comanda & 0xF0); // se trimit cei mai semnificativi 4 biti
	LCD_port &= ~ (1<<RS);							 // setam RS la 0 pentru transmiterea comenzii
	LCD_port |= (1<<EN);							 // se activeaza pinul EN
	_delay_us(1);
	LCD_port &= ~ (1<<EN);							 // se dezactiveaza pinul EN

	_delay_us(200);

	LCD_port = (LCD_port & 0x0F) | (comanda << 4);   // se trimit cei mai putin semnificativi 4 biti
	LCD_port |= (1<<EN);							 // se activeaza pinul EN
	_delay_us(1);
	LCD_port &= ~ (1<<EN);							 // se dezactiveaza pinul EN
	_delay_ms(2);
}

void LCD_send_char(unsigned char data)
{
	LCD_port = (LCD_port & 0x0F) | (data & 0xF0);	// se trimit cei mai semnificativi 4 biti
	LCD_port |= (1<<RS);							// setam pinul RS la 1 pentru a indica ca transmitem date
	LCD_port |= (1<<EN);							// se activeaza pinul EN
	_delay_us(1);
	LCD_port &= ~ (1<<EN);							// se dezactiveaza pinul EN

	_delay_us(200);

	LCD_port = (LCD_port & 0x0F) | (data << 4);		// se transmit cei mai putin semnificativi 4 biti
	LCD_port |= (1<<EN);							// se activeaza pinul EN
	_delay_us(1);
	LCD_port &= ~ (1<<EN);							// se dezactiveaza pinul EN
	_delay_ms(2);
}

void LCD_send_string (char *str)
{
	int i;
	for(i=0;str[i]!=0;i++)			// se transmite sirul de caractere pana la intalnirea caracterului NULL
	{
		LCD_send_char(str[i]);
	}
}

void initializare_LCD (void)
{
	LCD_dir = 0xFF;					// seteaza ca port de iesire
	_delay_ms(20);
	
	Comanda_LCD(0x02);				// trecere la modul de 4 biti (datele sunt transmise pe 4 linii de date, D4-7)
	Comanda_LCD(0x28);              // initializare LCD in modul de 4 biti si afisare pe 2 linii
	Comanda_LCD(0x0c);              // ascunde cursorul si dezactiveaza efectul de blink
	Comanda_LCD(0x06);              // se seteaza modul de afisare pentru deplasare la dreapta
	Comanda_LCD(0x01);              // sterge ecranul
	_delay_ms(2);
}

void LCD_Clear()
{
	Comanda_LCD (0x01);		// sterge ecranul
	_delay_ms(2);
	Comanda_LCD (0x80);		// se seteaza cursorul pe prima pozitie a primei linii
}

void timer0_init()
{
	

	
	TCCR0B |= (1 << CS02); //se seteaza prescaler-ul la 256 => T = 16us
	TIMSK0 |= (1 << TOIE0);
	
	
	
}

void timer1_init()
{
	// Fast PWM pe 8 biti non-inverting cu top la 0x00FF, prescaler de 1, output atat pe canalul A cat si pe B
	TCCR1A |= (1 << WGM10) | (1 << COM1A1) | (1 << COM1B1);
	TCCR1B |= (1 << WGM12) | (1 << CS10);

	
	OCR1A = 0;
	OCR1B = 0;
}

void timer2_init()
{
	// Fast PWM cu top la 0xFF, prescaler 128
	OCR2B = 255;
	TCCR2A |= (1 << COM2B1) | (1 << WGM20) | (1 << WGM21);
	TCCR2B |= (1 << CS22) | (1 << CS20);
}

void timer3_init()
{
	// FRECVENTA 2HZ, semnal si pe canalul A si pe canalul B pentru fiecare semnalizare
	ICR3 = 57599;
	OCR3A = ICR3 + 1;
	OCR3B = ICR3 + 1;
	// CTC cu top la ICR1, prescaler 64
	TCCR3B |= (1 << WGM33) | (1 << WGM32) | (1 << CS32);
	// enable la intreruperi
	TIMSK3 |= (1 << OCIE3B) | (1 << OCIE3A);
}

void USART0_init()
{
	//setam baud rate-ul
	UBRR0=7;
	
	UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);
	UCSR0C = (3<<UCSZ00);
}


void USART0_transmit(char data)
{
	// se asteapta pana cand buffer-ul e gol
	while(!(UCSR0A & (1<<UDRE0)));

	// se pun datele in buffer, transmisia pornind automat in urma scrierii
	UDR0 = data;
}

//Functie pentru transmiterea unui sir de caractere prin USART
void USART0_print(const char *data)
{
	while(*data != '\0')
	USART0_transmit(*data++);
}


void executare_comanda()
{
	cli();
	
	if(strstr(rx_buffer,"inainte")){
		
		LCD_Clear();
		LCD_send_string("inainte");
		PORTB &= ~(1 << PB0);
		PORTB &= ~(1 << PB1);
		OCR1A=151;
		OCR1B=151;
		inainte=1;
		goto finish;
		
	}
	
	else if(strstr(rx_buffer,"inapoi")){
		
		LCD_Clear();
		LCD_send_string("inapoi");
		PORTB |= 1 << PB0;
		PORTB |= 1 << PB1;
		OCR1A=151;
		OCR1B=151;
		inapoi=1;
		
		goto finish;
		
	}
	
	
	else if(strstr(rx_buffer,"frana")){
		LCD_Clear();
		LCD_send_string("frana");
		PORTB &= ~(1 << PB0);
		PORTB &= ~(1 << PB1);
		OCR1A=0;
		OCR1B=0;
		inapoi=0;
		inainte=0;
		drifting=0;
		goto finish;
		
	}
	
	
	else if (strstr(rx_buffer, "stanga")) {
		LCD_Clear();
		LCD_send_string("stanga");
		// daca masina nu se afla in miscare, nu fac nimic
		if (inainte == 0 && inapoi==0)
		goto finish;
		// incep virajul, salvez vechea valoare la OCR-ului si directia in care se face virajul
		if (drifting == 0)
		{
			
			saved = OCR1B;
			OCR1B = 0;
			drifting = 1;
			direction = 0;
			
		}
		else
		{
			// daca virajul nu a fost inceput in aceasta directie, nu fac nimic
			if (direction != 0)
			goto finish;
			// opresc virajul, refac valoarea OCR-ului
			OCR1B = saved;
			drifting = 0;
			direction = -1;
		}
		
	}
	
	
	else if (strstr(rx_buffer, "dreapta")) {
		
		LCD_Clear();
		LCD_send_string("dreapta");
		// analog ca mai sus, doar ca pe dos
		if (inainte == 0 && inapoi == 0)
		goto finish;
		if (drifting == 0)
		{
			
			saved = OCR1A;
			OCR1A = 0;
			drifting = 1;
			direction = 1	;
			
		}
		else {
			if (direction != 1)
			goto finish;
			OCR1A = saved;
			drifting = 0;
			direction = -1;
		}
	}
	
	else if (strstr(rx_buffer, "speedup")) {
		
		LCD_Clear();
		LCD_send_string("speedup");
		if(OCR1A == 255 && OCR1B == 255)
		goto finish;
		if(inainte == 1 || inapoi == 1)
		{
			OCR1A = OCR1A + 26;
			OCR1B = OCR1B + 26;
		}
		
		
	}
	else if (strstr(rx_buffer, "slowdown")) {
		
		LCD_Clear();
		LCD_send_string("slowdown");
		if(OCR1A == 151 && OCR1B == 151)
		goto finish;
		if(inainte == 1 || inapoi == 1)
		{
			OCR1A = OCR1A - 26;
			OCR1B = OCR1B - 26;
			
		}
		
		
	}
	else if(strstr(rx_buffer,"faruri")){
		
		LCD_Clear();
		LCD_send_string("faruri");
		PORTC ^= (1 << PC0);
	}
	
	else if(strstr(rx_buffer,"avarii"))
	{
		
		LCD_Clear();
		LCD_send_string("avarii");
		//daca avariile sunt oprite
		if (avarii == 0)
		{
			
			avarii=1;
			TCNT3 = 0;
			OCR3A = ICR3 / 2;
			OCR3B = ICR3 / 2;
			
		}
		else if (avarii == 1)
		{
			
			TCNT3 = 0;
			OCR3A = ICR3 + 1;
			OCR3B = ICR3 + 1;
			PORTD &= ~(1 << PD2);
			PORTD &= ~(1 << PD3);
			avarii=0;
		}
		
		
	}
	
	else if(strstr(rx_buffer,"sem_dr")){
		LCD_Clear();
		LCD_send_string("sem_dr");
		if (avarii == 1)
		goto finish;
		// daca semnalizarea era oprita
		if (sem_st == 0)
		{
			TCNT3 = 0;
			// opresc semnalizarea stanga
			OCR3B = ICR3 + 1;
			// pornesc semnalizarea dreapta
			OCR3A = ICR3 / 2;
			sem_st=1;
		}
		else
		{
			// altfel opresc semnalizarea
			TCNT3 = 0;
			OCR3A = ICR3 + 1;
			PORTD &= ~(1 << PD2);
			PORTD &= ~(1 << PD3);
			sem_st=0;
		}
		
	}
	
	
	
	else if (strstr(rx_buffer, "sem_st")) {

		LCD_Clear();
		LCD_send_string("sem_st");
		if (avarii == 1)
		goto finish;
		// daca semnalizarea era oprita
		if (sem_dr == 0)
		{
			// opresc semnalizrea dreapta
			TCNT3 = 0;
			OCR3A = ICR3 + 1;
			// pornesc semnalizarea stanga
			OCR3B = ICR3 / 2;
			sem_dr=1;
		}
		else
		{
			// altfel opresc semnalizarea
			TCNT3 = 0;
			OCR3B = ICR3 + 1;
			PORTD &= ~(1 << PD2);
			PORTD &= ~(1 << PD3);
			sem_dr=0;
		}
		
	}
	
	else if (strstr(rx_buffer, "claxon")) {
		LCD_Clear();
		LCD_send_string("claxon");
		
		OCR2B = 255 + 128 - OCR2B;
	}
	
	finish:
	sei();
}

ISR(USART0_RX_vect)
{
	
	received_byte = UDR0;
	// Retin caracterele primite de la modul, bufferul se goleste cand se fac citiri din el
	if (index_buffer < rx_size)
	rx_buffer[index_buffer++] = received_byte;
	
	rx_buffer[index_buffer] = '\0';

	// Setez alerta de receive
	date_primite = 1;
	
}

// toggle la semnalizarea dreapta
ISR(TIMER3_COMPA_vect) {
	PORTD ^= (1 << PD2);
}

// toggle la semnalizarea stanga
ISR(TIMER3_COMPB_vect) {
	PORTD ^= (1 << PD3);
}


ISR(TIMER0_OVF_vect)
{
	count++;
}

ISR(PCINT0_vect)
{
	if ((PINA & (1 << PA3)) != 0) {
		TCNT0 = 0;
		count = 0;
	}
	else {
		saved_tcnt0 = TCNT0;
		saved_count = count;
		flag = 1;
	}
}


void initializare_HCSR04()
{
	//Pini pentru Echo si Trigger
	DDRA |= (1 << PA2); // Trigger
	DDRA &= ~(1 << PA3); // Echo

	PORTA &= ~(1 << PA2);
	
	//activare intrerupere de tip pin change pe pinul Echo PA3
	PCICR |= (1 << PCIE0);
	PCMSK0 |= (1 << PCINT3);
}

void initializare_motoare()
{
	DDRD |= (1 << PD4) | (1 << PD5);
	DDRB |= (1 << PB0) | (1 << PB1);
}

void initializare_leduri()
{
	DDRD |= (1 << PD2) | (1 << PD3); //semnalizari si avarii
	PORTD &= ~(1 << PD2);
	PORTD &= ~(1 << PD3);
	
	DDRC |= (1 << PC0);	//faruri
}

void initializare_buzzer()
{
	DDRD |= (1 << PD6);
	PORTD |= (1 << PD6);
}

float get_distance()
{
	double sum = 0;
	for (int i = 1; i <= HC_SR04_NUM_ITERATIONS; i++) {
		flag = 0;
		
		//Reset Trigger
		PORTA &= ~(1 << PA2);
		_delay_us(5);

		//Trimite semnal catre Trigger
		PORTA |= (1 << PA2);
		_delay_us(11);
		PORTA &= ~(1 << PA2);
		
		//Asteapta pentru semnalul de la Echo
		while(flag == 0 && count <= 5);
		
		sum = sum + (((int)saved_tcnt0 + 255 * saved_count) * 16.0) * 0.014;
		
		if (i < HC_SR04_NUM_ITERATIONS)
		_delay_ms(75);
	}

	return sum / HC_SR04_NUM_ITERATIONS;
}


void initializare_wifi()
{
	
	sei();
	
	USART0_print("AT+CWMODE=3\r\n");
	_delay_ms(100);
	
	USART0_print("AT+CIPMODE=0\r\n");
	_delay_ms(100);
	
	USART0_print("AT+CIPMUX=1\r\n");
	_delay_ms(100);
	
	USART0_print("AT+CWJAP=\"Galaxy\",\"123456789\"\r\n");
	_delay_ms(7000);
	
	USART0_print("AT+CIPSERVER=1,101\r\n");
	_delay_ms(100);
	
	index_buffer=0;
	USART0_print("AT+CIPSTA?\r\n");
	_delay_ms(3000);
	
	char *adresa_ip = strchr(rx_buffer, '"');
	if (adresa_ip)
	{
		adresa_ip += 1;
		//cauta adresa caracterului " cand acesta apare prima data in sir
		char *finish = strchr(adresa_ip, '"');
		int pozitie = finish-adresa_ip;
		adresa_ip[pozitie]='\0';
		
	}
	LCD_Clear();
	LCD_send_string(adresa_ip);
	
	cli();

}

int main()
{
	
	
	USART0_init();
	timer0_init();
	timer1_init();
	timer2_init();
	timer3_init();
	initializare_LCD();
	
	initializare_motoare();
	initializare_leduri();
	initializare_buzzer();
	
	_delay_ms(1000);
	initializare_wifi();
	initializare_HCSR04();
	sei();
	while (1)
	{
		_delay_ms(100);
		float dist = get_distance();
		
		if (dist > 0 && dist < 20){
			
			PORTB &= ~(1 << PB0);
			PORTB &= ~(1 << PB1);
			OCR1A=0;
			OCR1B=0;
			
			
		}
		
		
		if (date_primite == 1)
		{
			index_buffer=0;
			executare_comanda();
			
			date_primite = 0;
		}
	}
	

	return 0;
}





