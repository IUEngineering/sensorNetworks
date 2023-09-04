#define F_CPU 32000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <math.h>

#include "nrf24spiXM2.h"
#include "nrf24L01.h"

#include "clock.h"
#include "serialF0.h"
#define DEBOUNCE_PERIOD_MS   10

uint8_t  pipe[5] = "ARM01";       // pipe address "HvA01"
uint8_t  packet[32];

float t = 1;

void set_adcch_input(ADC_CH_t *ch, uint8_t pos_pin_gc, uint8_t neg_pin_gc)
{
	ch->MUXCTRL = pos_pin_gc | neg_pin_gc;
	ch->CTRL    = ADC_CH_INPUTMODE_DIFF_gc;
}	

void init_adc(void)
{
	PORTA.DIRCLR     = PIN3_bm|PIN2_bm|PIN1_bm|PIN0_bm;     // PA3..0 are input
	set_adcch_input(&ADCA.CH0, ADC_CH_MUXPOS_PIN0_gc, ADC_CH_MUXNEG_GND_MODE3_gc);
	set_adcch_input(&ADCA.CH1, ADC_CH_MUXPOS_PIN1_gc, ADC_CH_MUXNEG_GND_MODE3_gc);
	set_adcch_input(&ADCA.CH2, ADC_CH_MUXPOS_PIN2_gc, ADC_CH_MUXNEG_GND_MODE3_gc);
	set_adcch_input(&ADCA.CH3, ADC_CH_MUXPOS_PIN3_gc, ADC_CH_MUXNEG_GND_MODE3_gc);
	ADCA.CTRLB       = ADC_RESOLUTION_12BIT_gc |
	ADC_CONMODE_bm |
	ADC_FREERUN_bm;                      // free running mode
	ADCA.REFCTRL     = ADC_REFSEL_INTVCC_gc;
	ADCA.PRESCALER   = ADC_PRESCALER_DIV16_gc;
	ADCA.CTRLA       = ADC_ENABLE_bm;
	ADCA.EVCTRL      = ADC_SWEEP_0123_gc|                   // sweep ch. 0,1,2,3
	ADC_EVSEL_0123_gc|                   // default, no trigger only sweep
	ADC_EVACT_NONE_gc;                   // no internal or external trigger

}

uint16_t read_adc(void)
{
	uint16_t res;

	ADCA.CH0.CTRL |= ADC_CH_START_bm;                    // start ADC conversion
	while ( !(ADCA.CH0.INTFLAGS & ADC_CH_CHIF_bm) ) ;    // wait until it's ready
	res = ADCA.CH0.RES;
	ADCA.CH0.INTFLAGS |= ADC_CH_CHIF_bm;                 // reset interrupt flag

	return res;                                          // return measured value
}

int16_t zAC = 0;
int16_t z = 0;
int16_t x = 0;
int16_t y = 0;

int button_pressed(void)
{
	if ( bit_is_clear(PORTD.IN,PIN0_bp) ) {
		_delay_ms(DEBOUNCE_PERIOD_MS);
		if ( bit_is_clear(PORTD.IN,PIN0_bp) ) return 1;
	}
	return 0;
}

void init_nrf(void)
{
	nrfspiInit();                                        // Initialize SPI
	nrfBegin();                                          // Initialize radio module

	nrfSetRetries(NRF_SETUP_ARD_1000US_gc,               // Auto Retransmission Delay: 1000 us
	NRF_SETUP_ARC_8RETRANSMIT_gc);						 // Auto Retransmission Count: 8 retries
	nrfSetPALevel(NRF_RF_SETUP_PWR_6DBM_gc);             // Power Control: -6 dBm
	nrfSetDataRate(NRF_RF_SETUP_RF_DR_250K_gc);          // Data Rate: 250 Kbps
	nrfSetCRCLength(NRF_CONFIG_CRC_16_gc);               // CRC Check
	nrfSetChannel(54);                                   // Channel: 54
	nrfSetAutoAck(1);                                    // Auto Acknowledge on
	nrfEnableDynamicPayloads();                          // Enable Dynamic Payloads

	nrfClearInterruptBits();                             // Clear interrupt bits
	nrfFlushRx();                                        // Flush fifo's
	nrfFlushTx();

	// Interrupt
	PORTF.INT0MASK |= PIN6_bm;
	PORTF.PIN6CTRL  = PORT_ISC_FALLING_gc;
	PORTF.INTCTRL   = (PORTF.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_LO_gc;
	
	nrfOpenWritingPipe(pipe);                            // Pipe for sending
	nrfOpenReadingPipe(0, pipe);                         // Necessary for acknowledge
	nrfStartListening();
}

uint16_t check_button(void)
{
	uint16_t res;
	if (button_pressed())	{
		res = 4000;
	}
	else	{
		res = 0;		
	}

	return res;                                          // return measured value
}

/*void ledtimer(void){
	
		float t = 1;
		TCE0.CTRLA    = TC_CLKSEL_DIV64_gc;    // prescaling 64x
		TCE0.PER      = 0XFFFF;
		
	if (TCE0.CNT >= 15625 && t <= 4){ // 64 * 15625 / 2000000 = 0.5 sec		
		PORTD.OUTTGL = PIN2_bm;
		t++;
		TCE0.CNT = 0;             // Reset timer
	}
}
*/

void trilmotor(void)
{
	PORTD.OUTSET = PIN2_bm;
	_delay_ms(2000);
	PORTD.OUTCLR = PIN2_bm;
}

/*void blinker()
{
	PORTD.OUTSET = PIN3_bm;
	_delay_ms(400);
	PORTD.OUTCLR = PIN3_bm;
	_delay_ms(100);
	
	PORTD.OUTSET = PIN3_bm;
	_delay_ms(400);
	PORTD.OUTCLR = PIN3_bm;
	_delay_ms(100);
	
	PORTD.OUTSET = PIN3_bm;
	_delay_ms(400);
	PORTD.OUTCLR = PIN3_bm;
	_delay_ms(100);
	
	PORTD.OUTSET = PIN3_bm;
	_delay_ms(400);
	PORTD.OUTCLR = PIN3_bm;
	
}*/
ISR(ADCA_CH0_vect)
{
	zAC = ADCA.CH0.RES;
	z = ADCA.CH1.RES;
	x = ADCA.CH2.RES;
	y = ADCA.CH3.RES;

	ADCA.CH0.CTRL |= ADC_CH_START_bm;           // starts conversion again
}

ISR(PORTF_INT0_vect)
{
	uint8_t  tx_ds, max_rt, rx_dr;

	nrfWhatHappened(&tx_ds, &max_rt, &rx_dr);

	if ( rx_dr ) {
		nrfRead(packet, 2);
		if (packet[0] && packet[1] != 0)
		{
			//blinker();
			PORTD.OUTSET = PIN3_bm;
			_delay_ms(2000);
			PORTD.OUTCLR = PIN3_bm;
			trilmotor();
			printf("%i", packet);
		}
		else
		{
		}
	}
}

int main(void)
{
	int16_t result;

	init_stream(F_CPU);
	check_button();
	init_nrf();
	init_adc();
	init_clock();
	sei();
	
	PMIC.CTRL |= PMIC_LOLVLEN_bm;
	sei();
		
	PORTD.DIRSET = PIN3_bm;
	PORTD.DIRSET = PIN2_bm;
	
	ADCA.CH0.CTRL |= ADC_CH_START_bm;
			
	while (1) {
		zAC = ADCA.CH0.RES;
		z = ADCA.CH1.RES;
		x = ADCA.CH2.RES;
		y = ADCA.CH3.RES;
		
		float xr;
		float yr;
		float zr;
		
		xr = x/10;
		yr = y/10;
		zr = z/10;

		if (xr>180 || xr<20 || yr>180 || yr<20 || zr>180 || zr<20)
		{
			nrfStopListening();
			result = read_adc();
			nrfWrite( (uint8_t*)&result, sizeof(uint16_t) );  // little endian: low byte is sent first
			printf("%i\n", result);
			_delay_ms(20);
			nrfStartListening();
		}
		
		if (button_pressed())
		{
			nrfStopListening();
			result = check_button();
			nrfWrite( (uint8_t*)&result, sizeof(uint16_t) );  // little endian: low byte is sent first
			_delay_ms(20); 
			nrfStartListening();
		}
		else {}		
	}
	return 0;
}