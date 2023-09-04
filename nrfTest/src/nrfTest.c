/*
 * nrftester.c
 *
 * Created: 18/03/2021 21:13:47
 * Author : Jochem Leijenhorst
 
 Dit programma heeft variabele pipes. Deze voer je in op tera term/putty
 Er zijn 4 commandos:
 
 *	help
	print deze lijst
 
 *	send <waarde>
	verstuurt wat je invoert op waarde naar de geselecteerde pipe
	
 *	writingpipe <pipenaam>
	verander de writing pipe
	
 *	readingpipe <index, pipenaam>
	verander de reading pipes
	
	Het programma print continu uit wat hij ontvangt.
 */ 

#define F_CPU	32000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include "serialF0.h"
#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include "clock.h"

char received_packet[NRF_MAX_PAYLOAD_SIZE+1];

void nrfInit(uint16_t channel) {
	nrfspiInit();
	nrfBegin();
	
	nrfSetRetries(NRF_SETUP_ARD_1000US_gc, NRF_SETUP_ARC_8RETRANSMIT_gc);
	nrfSetPALevel(NRF_RF_SETUP_PWR_6DBM_gc);
	nrfSetDataRate(NRF_RF_SETUP_RF_DR_250K_gc);
	nrfSetCRCLength(NRF_CONFIG_CRC_16_gc);
	nrfSetChannel(channel);
	nrfSetAutoAck(1);
	nrfEnableDynamicPayloads();
	
	nrfClearInterruptBits();
	nrfFlushRx();
	nrfFlushTx();
	
	//initialiseert de interrupt die runt wanneer er een signaal ontvangen wordt
	PORTF.INT0MASK |= PIN6_bm;
	PORTF.PIN6CTRL = PORT_ISC_FALLING_gc;
	PORTF.INTCTRL |= (PORTF.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_LO_gc;
	
	nrfOpenReadingPipe(0, (uint8_t *)"HVA01");
	
	nrfPowerUp();
	nrfStartListening();
}


int main(void) {
	init_clock();
    init_stream(F_CPU);
    
    PMIC.CTRL |= PMIC_LOLVLEN_bm;
    sei();
    clear_screen();
	
	printf("Welkom bij de nrftester\nGemaakt door Jochem Leijenhorst.\n\nTyp help voor een lijst met commando's.");
	printf("\nTyp een channel en druk op enter om te beginnen.\n\nBELANGRIJK: Ga in tera term naar setup -> terminal en klik op \"local echo\".\nZo kan je zien wat je typt. Je kan dit opslaan door naar setup -> save settings te gaan en daar op \"save\" te drukken.\n\n");
	uint16_t channel = 125;
	scanf("%d", &channel);
	nrfInit(channel);
	printf("Gestart met channel %d. Geen idee of dat legaal is, maar dat is jouw probleem.\n\n", channel);
    
    while (1) {
		char command[4]   = "";
		char attribute[NRF_MAX_PAYLOAD_SIZE+1] = "";
		
		scanf("%4s", command);
		if(strncmp(command, "help", 4) == 0) {
			printf("\n\nEr zijn 4 commandos:\n*	help\n\tprint deze lijst\n\n*	send <waarde>\n\tverstuurt wat je invoert op waarde naar de geselecteerde pipe\n\n*	wpip <pipenaam>\n\tverander de writing pipe\n");
			printf("\n*	rpip <index> <pipenaam>\n\tverander de reading pipes. Index is welke van de 6 pipes je wilt aanpassen (0 t/m 5).\n\nHet programma print continu uit wat het ontvangt.");
		}
		else if(strncmp(command, "send", 4) == 0) {
			scanf("%s", attribute);
			nrfStopListening();
			
			printf("\n\nVerzonden: %s\nAck ontvangen: %s\n", attribute, nrfWrite((uint8_t *) attribute, strlen(attribute))>0?"JA":"NEE");
			_delay_ms(5);
			nrfStartListening();
		}
		else if(strncmp(command, "wpip", 4) == 0) {
			scanf("%s", attribute);
			nrfOpenWritingPipe((uint8_t *)attribute);
			printf("\n\nWriting pipe %s geopend.\n", attribute);
		}
		else if(strncmp(command, "rpip", 4) == 0) {
			int index = 0;
			scanf("%5s %d", attribute, &index);
			nrfStopListening();
			nrfOpenReadingPipe(index, (uint8_t *) attribute);
			nrfStartListening();
			printf("\n\nReading pipe %d, %s geopend.\n", index, attribute);
			if(index > 1)  printf("Onthoud goed dat voor pipes 2 tot 5 alleen het laatste karakter wordt gebruikt. In dit geval is dat %c\n", attribute[4]);
		}
		else printf("\n\nDat commando ken ik niet.\nBackspace werkt niet trouwens, omdat ik te lui was om dat te laten werken. Alles in 1 keer goed typen dus.\n");
	
    }
}


ISR(PORTF_INT0_vect) {
	uint8_t	packet_length;
	if(nrfAvailable(NULL)) {						// is er iets nuttigs binnengekomen??
		packet_length = nrfGetDynamicPayloadSize();	// kijk hoe groot het is
		nrfRead(received_packet, packet_length);	// lees wat er is gestuurt
		received_packet[packet_length] = '\0';		// zet laatste karakter van de array naar null
		
		printf("Ontvangen: %s\n", received_packet);
	}	
}