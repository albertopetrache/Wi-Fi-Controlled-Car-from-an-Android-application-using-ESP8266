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

// se declara variabila count care va numara
// de cate ori timer 0 ajunge la valoarea maxima
static int count = 0;
static int saved_tcnt0 = 0;
static int saved_count = 0;
static int flag = 0;

// 1 daca am primit comenzi 
static int date_primite = 0;
// dimensiunea bufferului
static int rx_size=50;
// buffer in care se salveaza mesajele primite de la modulul wireless
static char rx_buffer[50];
// var care marcheaza finalul mesajului primit
static int index_buffer = 0;
static char received_byte;

static int inainte=0;
static int inapoi=0;
// 1 daca masina ia un viraj, 0 altfel

static int viraj = -1; // 0 = viraj stanga, 1 = viraj dreapta
// factorul vechi de umplere al motorului pana sa ia virajul
static int saved = 0;
static int avarii = 0;
static int sem_st=0;
static int sem_dr=0;


void Comanda_LCD(unsigned char comanda)
{	// se trimit cei mai semnificativi 4 biti
	LCD_port = (LCD_port & 0x0F) | (comanda & 0xF0); 
	// setam RS la 0 pentru transmiterea comenzii
	LCD_port &= ~ (1<<RS);
	// se activeaza pinul EN							 
	LCD_port |= (1<<EN);								 
	_delay_us(1);
	// se dezactiveaza pinul EN
	LCD_port &= ~ (1<<EN);							 

	_delay_us(200);
	
	// se trimit cei mai putin semnificativi 4 biti
	LCD_port = (LCD_port & 0x0F) | (comanda << 4); 
	// se activeaza pinul EN  
	LCD_port |= (1<<EN);							 
	_delay_us(1);
	 // se dezactiveaza pinul EN
	LCD_port &= ~ (1<<EN);							
	_delay_ms(2);
}

void LCD_send_char(unsigned char data)
{	
	// se trimit cei mai semnificativi 4 biti
	LCD_port = (LCD_port & 0x0F) | (data & 0xF0);
	// setam pinul RS la 1 pentru a indica ca transmitem date	
	LCD_port |= (1<<RS);
	// se activeaza pinul EN							
	LCD_port |= (1<<EN);								
	_delay_us(1);
	// se dezactiveaza pinul EN
	LCD_port &= ~ (1<<EN);	
							

	_delay_us(200);
	
	// se transmit cei mai putin semnificativi 4 biti
	LCD_port = (LCD_port & 0x0F) | (data << 4);
	// se activeaza pinul EN		
	LCD_port |= (1<<EN);							
	_delay_us(1);
	// se dezactiveaza pinul EN
	LCD_port &= ~ (1<<EN);							
	_delay_ms(2);
}

void LCD_send_string (char *str)
{
	int i;
	
	for(i=0;str[i]!=0;i++)			
	{
		LCD_send_char(str[i]);
	}
}

void initializare_LCD (void)
{
	// seteaza ca port de iesire
	LCD_dir = 0xFF;					
	_delay_ms(20);
	// trecere la modul de 4 biti (datele sunt transmise pe 4 linii de date, D4-7)
	Comanda_LCD(0x02);	
	// initializare LCD in modul de 4 biti si afisare pe 2 linii, matrice 5x7			
	Comanda_LCD(0x28);
	// ascunde cursorul si dezactiveaza efectul de blink             
	Comanda_LCD(0x0c);  
	// se seteaza modul de afisare pentru deplasare la dreapta            
	Comanda_LCD(0x06); 
	// sterge ecranul            
	Comanda_LCD(0x01); 
	            
	_delay_ms(2);
}

void LCD_Clear()
{	// sterge ecranul
	Comanda_LCD (0x01);		
	_delay_ms(2);
	// se seteaza cursorul pe prima pozitie a primei linii
	Comanda_LCD (0x80);		
}

void timer0_init()
{
	// se seteaza prescaler-ul la 256 => T = 1/0.0576 = 17.361us	
	TCCR0B |= (1 << CS02);
	// se activeaza intreruperea de overflow
	TIMSK0 |= (1 << TOIE0); 
		
}

void timer1_init()
{
	// Fast PWM pe 8 biti non-inverting cu top la 0x00FF, prescaler de 1, output atat pe canalul A cat si pe B
	TCCR1A |= (1 << WGM10) | (1 << COM1A1) | (1 << COM1B1);
	TCCR1B |= (1 << WGM12) | (1 << CS10);
	// se initializeaza valoarea registrelor OCR cu 0
	OCR1A = 0;
	OCR1B = 0;
}

void timer2_init()
{
	// Fast PWM cu top la 0xFF, prescaler 128, output pe canalul B
	OCR2B = 255;
	TCCR2A |= (1 << COM2B1) | (1 << WGM20) | (1 << WGM21);
	TCCR2B |= (1 << CS22) | (1 << CS20);
}

void timer3_init()
{
	// Frecventa de 2Hz
	ICR3 = 28799;
	OCR3A = ICR3 + 1;
	OCR3B = ICR3 + 1;
	// CTC cu top la ICR3, prescaler 256	
	TCCR3B |= (1 << WGM33) | (1 << WGM32) | (1 << CS32);
	// se activeaza intreruperile Output Compare A Match si B Match
	TIMSK3 |= (1 << OCIE3B) | (1 << OCIE3A);
}


void USART0_init()
{
	// se seteaza baud rate-ul
	UBRR0=7;
	// se activeaza receptorul, transmitatorul si intreruperile pentru receptia USART
	UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0); 
	// se seteaza formatul frame-ului la 8 biti de date, 1 bit de stop, fara paritate
	UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
}

// functie pentru trimiterea unui caracter prin USART
void USART0_transmit(char data)
{
	// se asteapta pana cand buffer-ul e gata sa primeasca date
	while(!(UCSR0A & (1<<UDRE0)));

	// se incarca caracterul in buffer, fiind gata sa fie trimis prin USART
	UDR0 = data;
}

// functie pentru transmiterea unui sir de caractere prin USART
void USART0_print(const char *data)
{
	while(*data != '\0')
	{
		USART0_transmit(*data++);
	}
	
}


void executare_comanda()
{
	cli();
	
	if(strstr(rx_buffer,"inainte")){
		
		LCD_Clear();
		LCD_send_string("Mers înainte");
		PORTB &= ~(1 << PB0);
		PORTB &= ~(1 << PB1);
		OCR1A=151;
		OCR1B=151;
		inainte=1;
		goto finish;
		
	}
	
	else if(strstr(rx_buffer,"inapoi")){
		
		LCD_Clear();
		LCD_send_string("Mers înapoi");
		PORTB |= 1 << PB0;
		PORTB |= 1 << PB1;
		OCR1A=151;
		OCR1B=151;
		inapoi=1;
		
		goto finish;
		
	}
	
	
	else if(strstr(rx_buffer,"frana")){
		
		LCD_Clear();
		LCD_send_string("Frân?");
		PORTB &= ~(1 << PB0);
		PORTB &= ~(1 << PB1);
		OCR1A=0;
		OCR1B=0;
		inapoi = 0;
		inainte = 0;
		viraj = -1;
		goto finish;
		
	}
	
	
	else if (strstr(rx_buffer, "stanga")) {		
		LCD_Clear();
		LCD_send_string("Viraj stanga");
		// daca masina nu se afla in miscare, se face un salt la eticheta finish
		if (inainte == 0 && inapoi==0)
			goto finish;
		
		// daca virajul nu a fost inceput 
		if (viraj == -1)
		{
			// se salveaza valoarea veche a OCR-ului
			saved = OCR1B;
			// pentru inceperea virajului la stanga se opresc rotile de pe partea dreapta
			OCR1B = 0;
			viraj = 1; // viraj stanga
			
		}
		else
		{
			// daca virajul nu a fost inceput in aceasta directie
			if (viraj != 1)
				goto finish;
			// se opreste virajul si se reface valoarea OCR-ului
			OCR1B = saved;
			viraj = -1; 
		}		
	}
	
	
	else if (strstr(rx_buffer, "dreapta")) {
		
		LCD_Clear();
		LCD_send_string("Viraj dreapta");
		// analog ca mai sus, doar ca pe dos
		if (inainte == 0 && inapoi == 0)
			goto finish;
		if (viraj == -1)
		{
			
			saved = OCR1A;
			OCR1A = 0;
			viraj = 0; //viraj dreapta
			
		}
		else {
			// daca virajul nu a fost inceput in aceasta directie
			if (viraj != 0)
				goto finish;
			// se opreste virajul si se reface valoarea OCR-ului
			OCR1A = saved;
			viraj = -1;
		}
	}
	
	else if (strstr(rx_buffer, "speedup")) {
				
		LCD_Clear();
		LCD_send_string("Speed Up");
		// daca motoarele se rotesc la viteza maxima
		if(OCR1A == 255 && OCR1B == 255)
			goto finish;
		// daca masina merge in directia inainte sau inapoi 	
		if(inainte == 1 || inapoi == 1)
		{
			OCR1A = OCR1A + 26;
			OCR1B = OCR1B + 26;
		}		
	}
	
	
	else if (strstr(rx_buffer, "slowdown")) {
		
		LCD_Clear();
		LCD_send_string("Slow Down");
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
		LCD_send_string("Aprindere faruri");
		PORTC ^= (1 << PC0);
	}
	
	else if(strstr(rx_buffer,"avarii"))
	{
		
		LCD_Clear();
		LCD_send_string("Avarii");
		// daca avariile sunt oprite
		if (avarii == 0)
		{
			
			avarii=1;
			sem_st=1;
			sem_dr=1;			
			OCR3A = ICR3 / 2;
			OCR3B = ICR3 / 2;
			
		}
		else if (avarii == 1)
		{
				
			OCR3A = ICR3 + 1;
			OCR3B = ICR3 + 1;			
			avarii=0;
			sem_st=0;
			sem_dr=0;
		}
		
		
	}
	
	else if(strstr(rx_buffer,"sem_st")){
		LCD_Clear();
		LCD_send_string("Semnalizare stânga");
		if (avarii == 1)
			goto finish;
		// daca semnalizarea era oprita
		if (sem_st == 0)
		{
			
			// opresc semnalizarea dreapta
			OCR3B = ICR3+1;
			PORTD &= ~(1 << PD2);
			// pornesc semnalizarea stanga
			OCR3A = ICR3 / 2;
			sem_st=1;
		}
		else
		{
			// altfel opresc semnalizarea
			
			OCR3A = ICR3+1;		
			sem_st=0;
		}
		
	}
	
	
	
	else if (strstr(rx_buffer, "sem_dr")) {

		LCD_Clear();
		LCD_send_string("Semnalizare dreapta");
		if (avarii == 1)
			goto finish;
		// daca semnalizarea era oprita
		if (sem_dr == 0)
		{
			// opresc semnalizarea stanga
			
			OCR3A = ICR3 + 1;
			PORTD &= ~(1 << PD3);
			// pornesc semnalizarea dreapta
			OCR3B = ICR3 / 2;
			sem_dr=1;
			
		}
		else
		{
			// altfel opresc semnalizarea			
			OCR3B = ICR3 + 1;						
			sem_dr=0;
		}
		
	}
	
	else if (strstr(rx_buffer, "claxon")) {
		LCD_Clear();
		LCD_send_string("Claxon");
		
		OCR2B = 255 + 114 - OCR2B;
	}
	
	finish:
	sei();
}

ISR(USART0_RX_vect)
{
	
	received_byte = UDR0;
	// se introduc in sirul de caractere rx_buffer caracterele primite de la modulul Wi-Fi
	if (index_buffer < rx_size)
	{
		rx_buffer[index_buffer++] = received_byte;
	}
	// se marcheaza sfarsitul sirului de caractere
	rx_buffer[index_buffer] = '\0';	
	date_primite = 1;
	
}

// toggle la semnalizarea dreapta
ISR(TIMER3_COMPB_vect)
{
	//se comuta starea pinului PD2
	PORTD ^= (1 << PD2);

}

// toggle la semnalizarea stanga
ISR(TIMER3_COMPA_vect) 
{
	// se comuta starea pinului PD3	
	PORTD ^= (1 << PD3);	
}


ISR(TIMER0_OVF_vect)
{
	
	count++;
}

ISR(PCINT0_vect)
{
	//daca pinul ECHO e pe HIGH
	if (PINA & (1 << PA3)) {
		// se reseteaza registrul TCNT0 si variabila count si se incepe
		// o noua masuratoare
		TCNT0 = 0;
		count = 0;
		flag=0;
	}
	else {
		//daca pinul ECHO e pe LOW (undele au fost reflectate inapoi de la obiect)
		//si s-a incheiat masuratoarea 
		//se salveaza valorile registrului TCNT0 si variabilei count
		saved_tcnt0 = TCNT0;
		saved_count = count;
		flag=1;
	}
}


void initializare_HCSR04()
{
	// Trigger (port de iesire)
	DDRA |= (1 << PA2);
	// Echo   (port de intrare)
	DDRA &= ~(1 << PA3); 
	PORTA &= ~(1 << PA2);
	
	//activare intrerupere de tip pin change pentru grupul PCINT[7:0]
	PCICR |= (1 << PCIE0);
	//activare intrerupere de tip pin change pe pinul Echo PA3	
	PCMSK0 |= (1 << PCINT3);
}

void initializare_motoare()
{
	DDRD |= (1 << PD4) | (1 << PD5);
	DDRB |= (1 << PB0) | (1 << PB1);
}

void initializare_leduri()
{	//semnalizari si avarii
	DDRD |= (1 << PD2) | (1 << PD3); 
	PORTD &= ~(1 << PD2);
	PORTD &= ~(1 << PD3);
	//faruri
	DDRC |= (1 << PC0);	
}

void initializare_buzzer()
{
	DDRD |= (1 << PD6);
	PORTD |= (1 << PD6);
}

float get_distance()
{
	double distanta=0;
	
	//Reset Trigger
	PORTA &= ~(1 << PA2);
	_delay_us(5);

	//Trimite semnal catre Trigger
	PORTA |= (1 << PA2);
	_delay_us(11);
	PORTA &= ~(1 << PA2);
	
	//se asteapta pana se incheie masuratoarea
	while (flag == 0);
	
	//viteza sunetului in aer la 20C = 343m/s
	distanta = ((unsigned int)saved_tcnt0 + 255 * saved_count) * (17150*17.361*1E-6);
	
	return distanta;
	
	
}


void initializare_wifi()
{
	// se activeaza intreruperile globale
	sei();	
	// se configureaza modulul ESP-01 ca statie 
	USART0_print("AT+CWMODE=1\r\n");
	_delay_ms(100);	
	// se dezactiveaza modul de transfer transparent al datelor 
	USART0_print("AT+CIPMODE=0\r\n");
	_delay_ms(100);	
	// se permit mai multe conexiuni simultane
	USART0_print("AT+CIPMUX=1\r\n");
	_delay_ms(100);	
	// se conecteaza modululul la reteaua Wi-Fi
	USART0_print("AT+CWJAP=\"Galaxy\",\"123456789\"\r\n");
	_delay_ms(7000);	
	// se configureaza modulul astfel incat sa asculte pe portul 101 
	USART0_print("AT+CIPSERVER=1,101\r\n");
	_delay_ms(100);		
	// se obtine adresa IP atribuita modulului
	USART0_print("AT+CIPSTA?\r\n");
	_delay_ms(1000);	
	index_buffer=0;
	// se cauta prima aparitie a caracterului "
	char *adresa_ip = strchr(rx_buffer, '"');
	if (adresa_ip)
	{
		adresa_ip += 1;	
		//se cauta a doua aparitie a caracterului "	
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
	
	char string[10];
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
		dtostrf(dist, 2, 2, string);
		LCD_Clear();
		LCD_send_string(string);
		if (date_primite == 1)
		{
			index_buffer=0;
			executare_comanda();
			
			date_primite = 0;
		}
		
		
		if(sem_st == 0)
		{					
			PORTD &= ~(1 << PD3);							
		}
		
		if(sem_dr == 0)
		{
			PORTD &= ~(1 << PD2);
		}
				
		
				
		
	}
	

	return 0;
}





