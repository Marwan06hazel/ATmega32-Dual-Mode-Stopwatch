#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define HOUR_IN_SECONDS 3600
#define MINUTE_IN_SECONDS 60
#define ONES %10
#define TENS /10
#define SECONDS_FIRST_DIGIT ((total_seconds%HOUR_IN_SECONDS)%MINUTE_IN_SECONDS)ONES
#define SECONDS_SECOND_DIGIT ((total_seconds%HOUR_IN_SECONDS)%MINUTE_IN_SECONDS)TENS
#define MINUTES_FIRST_DIGIT ((total_seconds%HOUR_IN_SECONDS)/MINUTE_IN_SECONDS)ONES
#define MINUTES_SECOND_DIGIT ((total_seconds%HOUR_IN_SECONDS)/MINUTE_IN_SECONDS)TENS
#define HOURS_FIRST_DIGIT (total_seconds/HOUR_IN_SECONDS)ONES
#define HOURS_SECOND_DIGIT (total_seconds/HOUR_IN_SECONDS)TENS
#define DELAY_IN_MS 3

typedef unsigned long uint_32;
typedef unsigned char uint_8;
typedef signed char sint_8;

uint_32 total_seconds = 0;
uint_8 buttons_flag = 0;
sint_8 inc_dec_flag = 1;
uint_8 inner_flag = 0;
uint_8 mode = 0;
uint_8 is_paused = 0;

void INT0_Init(void);
void INT1_Init(void);
void Timer1_Init(void);
void pause_timer(void);
void resume_timer(void);
void counting(void);
void check_buttons(void);
void check_mode(void);
void buzzer_enable(void);
void buzzer_disable(void);
void stopwatch_display(void);

/*
 * Prescaler = 1024 - F_CPU = 16MHz
 * Resolution = 1024 / 16,000,000 = 0.000 064s
 * 65,536 * 0.000 064s = 4.194304s [time of all ticks]
 * 65,536 -> 4.194304s
 * ?????? -> 1s
 * Number of needed ticks = 15,625
 */

ISR(TIMER1_COMPA_vect){
	// Setup mode
	if (mode == 0) {
	    total_seconds++;
	    buzzer_disable();
	} else {
	    if (total_seconds == 0) {
	        if (!(PIND & (1<<PD1))) {
	            buzzer_disable();
	        } else {
	            buzzer_enable();
	        }
	    } else {
	        total_seconds--;
	    }
	}
}

ISR(INT0_vect){
	total_seconds = 0;
}

ISR(INT1_vect){
	pause_timer();
}

ISR(INT2_vect){
	resume_timer();
}

/* Reset INT */
void INT0_Init(void){
	DDRD &= ~(1<<PD2);
	GICR |= (1<<INT0);
	MCUCR |= (1<<ISC01);
	sei();
}

/* Pause INT */
void INT1_Init(void){
	DDRD &= ~(1<<PD3);
	MCUCR |= ((1<<ISC11) | (1<<ISC10));
	GICR |= (1<<INT1);
	sei();
}

/* Resume INT */
void INT2_Init(void){
	DDRB &= ~(1<<PB2);
	MCUCSR &= ~(1<<ISC2);
	GICR |= (1<<INT2);
	sei();
}

void Timer1_Init(void){
	TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
	OCR1A = 15624; // Counting from 0
	TIMSK = (1<<OCIE1A);
	sei();
}

void pause_timer(void){
	TCCR1B = 0;
	is_paused = 1;
}

void resume_timer(void){
	TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
	is_paused = 0;
}

void counting(void){
	check_mode();

	if (mode == 0){
		PORTD |= (1<<PD4);
		PORTD &= ~(1<<PD5);
	}else{
		PORTD |= (1<<PD5);
		PORTD &= ~(1<<PD4);
	}

	PORTA = (PORTA & 0xC0);
	PORTC = (PORTC & 0xF0) | SECONDS_FIRST_DIGIT;
	PORTA = (1 << PA5);
	_delay_ms(DELAY_IN_MS);

	PORTA = (PORTA & 0xC0);
	PORTC = (PORTC & 0xF0) | SECONDS_SECOND_DIGIT;
	PORTA = (1 << PA4);
	_delay_ms(DELAY_IN_MS);

	PORTA = (PORTA & 0xC0);
	PORTC = (PORTC & 0xF0) | MINUTES_FIRST_DIGIT;
	PORTA = (1 << PA3);
	_delay_ms(DELAY_IN_MS);

	PORTA = (PORTA & 0xC0);
	PORTC = (PORTC & 0xF0) | MINUTES_SECOND_DIGIT;
	PORTA = (1 << PA2);
	_delay_ms(DELAY_IN_MS);

	PORTA = (PORTA & 0xC0);
	PORTC = (PORTC & 0xF0) | HOURS_FIRST_DIGIT;
	PORTA = (1 << PA1);
	_delay_ms(DELAY_IN_MS);

	PORTA = (PORTA & 0xC0);
	PORTC = (PORTC & 0xF0) | HOURS_SECOND_DIGIT;
	PORTA = (1 << PA0);
	_delay_ms(DELAY_IN_MS);
}

void check_buttons(void){
	if (!(PINB & (1 << PB5))) {
		_delay_ms(20);
		if (!(PINB & (1 << PB5)) && buttons_flag == 1){
			if (total_seconds > 0) total_seconds -= 1;
			buttons_flag = 0;
		}
	} else if (!(PINB & (1 << PB6))) {
		_delay_ms(20);
		if (!(PINB & (1 << PB6)) && buttons_flag == 1){
			total_seconds += 1;
			buttons_flag = 0;
		}
	} else if (!(PINB & (1 << PB3))) {
		_delay_ms(20);
		if (!(PINB & (1 << PB3)) && buttons_flag == 1){
			if (total_seconds >= MINUTE_IN_SECONDS) total_seconds -= MINUTE_IN_SECONDS;
			buttons_flag = 0;
		}
	} else if (!(PINB & (1 << PB4))) {
		_delay_ms(20);
		if (!(PINB & (1 << PB4)) && buttons_flag == 1){
			total_seconds += MINUTE_IN_SECONDS;
			buttons_flag = 0;
		}
	} else if (!(PINB & (1 << PB0))) {
		_delay_ms(20);
		if (!(PINB & (1 << PB0)) && buttons_flag == 1){
			if (total_seconds >= HOUR_IN_SECONDS) total_seconds -= HOUR_IN_SECONDS;
			buttons_flag = 0;
		}
	} else if (!(PINB & (1 << PB1))) {
		_delay_ms(20);
		if (!(PINB & (1 << PB1)) && buttons_flag == 1){
			total_seconds += HOUR_IN_SECONDS;
			buttons_flag = 0;
		}
	}else{
		total_seconds += 0;
		buttons_flag = 1;
	}
}

void check_mode(void){
    if(!(PINB & (1 << PB7))){
        _delay_ms(20);
        if(!(PINB & (1 << PB7)) && inc_dec_flag == 1) mode = 1;
        else if (!(PINB & (1 << PB7)) && inc_dec_flag == -1) mode = 0;
        inner_flag = 1;
    }else if (inner_flag == 1){
        inc_dec_flag *= -1;
        inner_flag = 0;
    }
}

void buzzer_enable(void){
	PORTD |= (1<<PD0);
}

void buzzer_disable(void){
	PORTD &= ~(1<<PD0);
}

void stopwatch_display(void){
	if (!(PINC & (1<<PC4))){
		PORTA = (PORTA & 0xC0);
		PORTD &= ~((1<<PD4) | (1<<PD5));
		TIMSK &= ~(1<<OCIE1A);
		TCCR1B = 0;
		total_seconds = 0;
		buzzer_disable();
		is_paused = 0;

		while(!(PINC & (1<<PC4)));
	}else if (!is_paused){
		resume_timer();
		TIMSK |= (1<<OCIE1A);
	}
}

int main(void){
	DDRB = 0X00;
	PORTB = 0XFF;

	DDRD |= (1<<PD4) | (1<<PD5) | (1<<PD0);
	PORTD &= ~((1<<PD4) | (1<<PD5) | (1<<PD0));

	DDRD &= ~(1<<PD1);
	PORTD |= (1<<PD1);

	DDRC &= ~((1<<PC4) | (1<<PC5));
	PORTC |= (1<<PC4);
	PORTC &= ~(1<<PC5);

	DDRC |= ((1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3));
	PORTC &= ~((1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3));

	DDRA = 0x3F;

	Timer1_Init();
	INT0_Init(); /* Reset INT */
	INT1_Init(); /* Pause INT */
	INT2_Init(); /* Resume INT */

	for(;;){
		counting();
		check_buttons();
		stopwatch_display();
	}

	return 0;
}
