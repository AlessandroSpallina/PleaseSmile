#include <stm32f10x.h> 
#include <stm32f10x_rcc.h> 
#include <stm32f10x_gpio.h> //input/output
#include <stm32f10x_exti.h> //interrupt
#include <stm32f10x_usart.h>//usart
#include <misc.h> //varie


/*Signature di funzione che esplicitiamo in seguito alla fine*/
void Delay(uint32_t nTime);
void halve_pause();

static int pause = 2000; /*Valore di tempo di partenza tra il led accesso e spento*/
static int ledval = 0; /*ledval indica se il led è accesso o spento -> 0=OFF 1=ON*/ 


void EXTI15_10_IRQHandler(void){ 
    if (EXTI_GetITStatus(EXTI_Line13) != RESET){
		halve_pause();
		EXTI_ClearITPendingBit(EXTI_Line13); 
    } 
}


int main(void) {
	/*questi due parametri mode e baud sono le modalità e velocità con cui useremo la usart(velocità espressa in bit/s) */
	uint32_t mode  = USART_Mode_Rx | USART_Mode_Tx;  //operatore OR bitwise per attivare entrambe le modalità ricezione/trasmissione
	uint32_t baud = 19200; 
   
     
	/*questa struttura GPIO_InitStructture che è di tipo GPIO_InitTypeDef deve essere dichiarata,
	perchè essa contiene tutti i parametri che dobbiamo andare a configurare successivamente per
	fare funzionare le periferiche di I/O interessate*/ 
    GPIO_InitTypeDef GPIO_InitStructure;
	
	/*Prima di andare a implementare la funzione del progetto, dobbiamo prima abilitare tutte le periferiche attraverso TRE STEPS fondamentali e cioè:
	
	1)abilitazione dei clock delle periferiche che si intendono utilizzare e i moduli che essi necessitano (ricordate: ogni periferica dell'STM32 ha 
	un suo clock locale  e vanno abilitati singolarmente per  ridurre la  dissipazione potenza dinamica cioè dovuta agli switch dei transistor)
	esempio una UART(è un dispositivo hardware di uso generale o dedicato. Converte flussi di bit di dati da un formato parallelo a un formato 
	seriale asincrono o viceversa. Praticamente ogni famiglia di microprocessori ha la sua UART/USART dedicata.)
	necessita del modulo UART ma richiede l'accensione del modulo GPIO che controlla l'Input/Output dei pin, del modulo Alternate Function AF che
	gestisce le risorse condivise.
	
	2)configurazione dei pin che useremo. (nel nostro caso stiamo usando pin PA5 e pin PC13)
	
	3)Inizializzazione periferica (le periferiche sono tutti i componenti che vedete nella board: USART,NVIC,GPIO,DMA,SPI,TIMER etc.)*/

    
	/*STEP 1: abilitazione clock*/
	/* Abilito bus A e C poichè utiliziamo PA6(led) e PC13(button) */
	/*I pin PA6 e PC13 sono pin per l'input/output, quindi sono gestiti dalla periferica/modulo GPIO; e guardando i pin della scheda e tramite il manuale vediamo che  essi
	appartengono al GPIOA e GPIOC, moduli del GPIO che sono collegati al bus APB2. 
	Ricordate che le periferiche comunicano fra loro tramite bus(sono attaccate a due bus APB1 e APB2)--> in STM32 si hanno diversi bus tra cui APB1,APB2,AHB.	
	Se aprite le slide STM32_02_introduction_to_STM32_F1, nella slide 5 troverete una figura che vi chiarirà la disposizione dei bus e periferiche.*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // (1)   
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); // (1)
	/* RCC_APB2Periph_GPIOA/C: è una costante definita nella libreria --> rappresenta l'identificatore della periferica */
	
	
	/*STEP 2: configurazione pin*/
	/*siccome dobbiamo modificare GPIO_Initstructure che è la struttura dichiarata precedentemente, dobbiamo impostare i parametri; 
	quindi dobbiamo passare l'indirizzo della struttura alla funzione GPIO_StructInit*/
    GPIO_StructInit(&GPIO_InitStructure); 	
	/*avviata la funzione possiamo direttamente configurare i parametri dei pin tramite la notazione puntata.
	ricordate che se si deve modificare il campo di una struttura in linguaggio C adottiamo la sintassi
	
	nome_struttura.campo_struttura= assegnamento; 	*/
	
    /*PA5(led) avviamo il pin 5 in modalità output con frequenza 2Mhz (esistono altre frquenza standard, ma di norma si adotta
	la frequenza più bassa che supporta la board per un fatto di risparmio energetico)*/
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;  
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //PP -> PUSH PULL
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* PC13 (Button)	IDEM per il pin 13 che stavolta è in modalità input, e inoltre non impostiamo frequenze perchè è il bottone user(blu)*/
    GPIO_StructInit(&GPIO_InitStructure); 
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13; 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //IN_FLOATING -> C'è non c'è
	
	
	/*STEP 3: Inizializzazione*/
	GPIO_Init(GPIOC, &GPIO_InitStructure); // passo la struttura alla funzione init che inizializza il pin opportuno con le modalità definite 
	/*GPIO_Init() è la funzione a cui passiamo la struttura e definiamo anche a quale GPIO ci stiamo riferendo, se A,B o C (nel nostro caso è A e C)*/

	
	/*eseguiti i 3 step standard vi è un'altra istruzione che deve essere sempre presente, ed è il Systick_Config().
	Perchè finora abbiamo solo configurato i clock, ma dobbiamo programmarli, cioè definire la loro granularità.
	In poche parole quanto deve durare un CICLO di CLOCK?
	basta applicare (SysTick_Config(SystemCoreClock/numero di clock))--> in questo caso lo abbiamo impostato a 1(frequenza di clock)/1000 = 0.001s
	il while(1) successivo serve a far funzionare il clock ciclicamente per tutta l'esecuzione del programma*/
	if (SysTick_Config(SystemCoreClock / 1000)) while (1);
	
	
	
	
	//////////////////////////////////// USART ////////////////////////////////////
	
	/*la USART è una periferica che serve per la comunicazione seriale fra le periferiche INTERNE alla board stm32, ma anche per le comunicazioni
	con periferiche esterne alla board (arduino, monitor seriale, screen LCD e tanti altri dispositivi).
	USART è costituito da due lati: 
	lato di trasmissione TX e lato di ricezione RX---> ogni lato è costituito da un buffer/registro detto DATA REGISTER
	che è la locazione in cui si conserveranno i dati da trasmettere/inviare.
	L'STM32 ha 3 USART.
	Come visto per la periferica GPIO e i suoi 3 step di inizializzazione, anche per la USART bisogna applicare lo stesso procedimento che verrà illustrato
	dopo la definizione delle seguenti funzioni (mettere prima o dopo le funzioni non cambia niente)*/
	
	/*FUNZIONI USART/
	
	/*questa funzione scrive sul DATA REGISTER di trasmissione;
	assieme ad ogni DATA REGISTER c'è uno STATUS REGISTER che è un flag che serve ad indicare se il dataregister è pieno o vuoto.
	Il while non fa altro che vedere ciclicamente se il flag_txe (dove X sta per IS, E sta per EMPTY) della USARTx(nostro caso usart2) è vuoto o meno.
	Fintanto che il flag==RESET e cioè pari a 0 allora rimaniamo nel while. Appena il flag diventa 1 (significa che è pieno) si esce dal ciclo e non scriviamo più su USART */
	
	int uart_putc(int c, USART_TypeDef* USARTx){
		while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET); //guarda il DR e mi prende il bit denominato TXE e se == RESET rimane nel while
		USARTx->DR = (c & 0xff); //siccome il data register DR è di 32 bit, mentre un carattere è solamente di 8 bit--> abbiamo fatto un &(bit a bit)con un registro vuoto di 32 bit
		return 0; //in modo da evitare la conversione del carattere di 8bit in una variabile da 32-->questo passaggio contorto è dovuto ad un fatto di prestazioni, in quanto una istruzione di conversione è onerosa per un processore embedded
	}


	/*Stesso discorso per uart_getc ; in questo caso invece il ricevitore deve andare a leggere il carattere presente nel DATA REGISTER
	e prima di andare a leggere va a vedere, tramite lo STATUS REGISTER, se il dataregister non è vuoto (cioè se ci sono dati da leggere)*/
	
	int uart_getc(USART_TypeDef* USARTx){
		while (USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == RESET); 
		return USARTx->DR & 0xff;
	}


	/*I 3 STEPS li abbiamo implementati direttamente in un unica funzione [int uart_open(USART_TypeDef* USARTx, uint32_t baud, uint32_t flags)], 
	cosicchè possiamo eseguirli tramite un'unica chiamata di funzione uart_open passandogli:
	-la usart che adotteremo, in questo caso la USART2
	-baud rate
	-modalità di uso ricezione,trasmissione o entrambe */
		
	int uart_open(USART_TypeDef* USARTx, uint32_t baud, uint32_t flags){
    
		/*STEP 1: abilitazione clock delle periferiche interessate	
		nota bene: oltre la USART2 e i pin di I/O controllati dal GPIO, abbiamo anche AFIO (alternate function I/O) che contiene i pin che sono sempre I/O 
		ma comandati da periferiche esterne.
		Utilizziamo GPIO per il pin 3(RX) poiché è il microcontrollore che deve accedere al pin (tramite il quale "legge" quello che gli scrive la USART)
		Utilizziamo AFIO per il pin 2(TX) poiché è la USART (perifierica esterna al microcontrollore) che deve accedere al pin per poter scrivergli i bit e inviarli in uscita
		*/
		
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA, ENABLE); 
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); //se vedi slide STM32_02_introduction_to_STM32_F1 slide5 notiamo che la USART2 si trova nel bus APB1 
		
		/*STEP 2: configurazione pin 
					e
		STEP 3: configurazione periferica*/
		/*applichiamo le stesse istruzioni di prima , ma per i pin 2(trasmission TX) e 3(ricezione RX) [Vedi da slide]*/
		GPIO_InitTypeDef GPIO_InitStruct; 
		GPIO_StructInit(&GPIO_InitStruct);

		// Configuro GPIO TX (Pin 2)
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2; 
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; 
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;  //modalità alternate function sta ad indicare che in pin è comandato da periferica esterna (USART)
		GPIO_Init(GPIOA, &GPIO_InitStruct);
		
		// Configuro GPIO RX (Pin 3)
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3; 
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
		GPIO_Init(GPIOA, &GPIO_InitStruct);


		// see stm32f10x_usart.h
	
		/*il GPIO ha una sua struttura di parametri configurabili, stessa cosa vale per la USART che avrà una sua USART_InitStructure di tipo USART_InitTypeDef*/
		USART_InitTypeDef USART_InitStructure;
	
		// Initialize USART structure
		USART_StructInit(&USART_InitStructure);
		USART_InitStructure.USART_BaudRate = baud; 
		USART_InitStructure.USART_Mode = flags;
		USART_Init(USARTx,&USART_InitStructure);
		USART_Cmd(USARTx, ENABLE);

	}
   
   
	uart_open(USART2,baud,mode); //Chiamo la funzione che inizializza la uart e i pin che utilizza per RX / TX
	/*Stringhe che passiamo attraverso la UART*/
	char * on = "Il Led è Acceso\r\n";
	char * off = "Il Led è Spento\r\n";
	char * c;
    	
	
	
	//////////////////////////////////// INTERRUPT ////////////////////////////////////

    //abilitiamo il clock nella AFIO sul bus APB2; abbiamo bisogno di AFIO perché è comandato da perifirica esterna, ossia EXTI
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,  ENABLE);

    // Colleghiamo EXTI a PC13 (ossia gli comunichiamo che deve aspettarsi INTERRUPT da pc13)
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource13);

    /* 	
	EXTI_InitStructure è una struttura dati che contiene una serie di parametri che permettono di gestire gli interrupt esterni
   	scatenati ad esempio da linee esterne collegate ai pin (nel nostro caso la imposteremo per far scatenare un interrupt quando viene
   	premuto il tasto (PC13).
	
	L'NVIC (vedi dopo) vede delle linee esterne, ma non vede una linea esterna per ogni pin (li vede a gruppi) : 
	le linee collegate ai pin vengono raggruppate a numeri.
    Nel nostro caso con EXTI_Line13 diciamo di impostare l'interrupt quando accade qualcosa sulle linee 13 tra cui  è presente anche il tasto.
    Mettendo EXTI_Line5 (ad esempio) ci riferiamo a tutto ciò che è collegato ai pin numero 5 di ogni GPIO (A,B,C)
    */
    EXTI_InitTypeDef  EXTI_InitStructure;

    EXTI_InitStructure.EXTI_Line = EXTI_Line13; //scelgo la linea 13
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; //abilita la linea 13 in modalità interrupt
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; 
    EXTI_InitStructure.EXTI_LineCmd = ENABLE; 
    EXTI_Init(&EXTI_InitStructure);

    //disable AFIO clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,  DISABLE);

    /* 
	Affinchè gli interrupt funzionino bisogna programmare il componente NVIC (Nested Vector Interrupt Controller): si prende tutti i segnali hardware
    e se accade un interrupt comunica con il processore. Ogni interrupt ha delle priorità 
	(in STM32 esistono 16 livelli di priorità suddivise in due categorie:
	Preemption Priority e SubPrioriority.
    La Preemption Priority è una priorità valutata quando è in esecuzione un interrupt e ne arriva un altro con priorità diversa.
    La SubPriority: priorità valutata quando arrivano all'NVIC più priorità da gestire )
    */
    
	/*Inizializziamo la struttura per NVIC (NVIC_InitTypeDef) e configuriamo l'interrupt (da dove proviene la request e che priorità diamo)*/
	NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel =  EXTI15_10_IRQn; //Utilizziamo il gruppo 15-10 poichè stiamo utilizzando la linea 13 [SLIDE]
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); // no Preemption Priority
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3; // diamo priorità 3 ai interrupt da 10 a 15 (in cui è contenuto quello della nostra linea (13)) 
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // non vengono gestite le interruzioni delle interruzioni -> di conseguenza non è necessario impostare un valore di priorità 
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
    NVIC_Init(&NVIC_InitStructure);
    
	////////////////////////////////////////////////////////////

    
	if (SysTick_Config(SystemCoreClock / 1000)) while (1);

    /*********************************************************************/
	/*************************CORPO DEL PROGETTO*************************/
	/*******************************************************************/
	
	while (1) {
	// scrive il bit 1(BIT_SET) al pin 5 o 0(BIT_RESET) in funzione al valore di ledval
	// se ledval == 0  scriviamo BIT_SET  0 altrimenti.
	GPIO_WriteBit(GPIOA, GPIO_Pin_5, (ledval) ? Bit_SET : Bit_RESET); 
	
	// GPIO_ReadInputDataBit legge il valore al pin 13 che viene scritto quando viene premuto il pulsante
	// di default al pin 13 troviamo 5 V  (BIT a 1)

	if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13)){ //alla pressione del pulsante
		if (ledval == 0){ //se il led è spento
			c = off;
			while (*c){
				uart_putc(*(c++),USART2); //invia off attraverso la usart
	    	}
		} 
		else { //se il led è accesso
			c = on;
			while (*c){ 
				uart_putc(*(c++),USART2); //invia on attraverso la usart
			}
		}
		ledval = 1 - ledval; //alterniamo ad ogni fine while il lavore di ledval (1<->0)
	}
	Delay(pause); // facciamo un pausa alla fine di ogni while del valore di pause 
	}
   
}




void halve_pause(){// funzione chiamata dall' input handler (EXTI15_10_IRQHandler)
	pause = pause/2;
}

static __IO uint32_t TimingDelay;




/*Delay serve soltanto per sospendere un programma per un tot di ms,nell'ambiente di arduino è implementata, 
qui invece in STM32 non lo è quindi bisogna implementarla da sè. Perchè STM32 non offre funzioni
ad alto livello.
Da come si intravede nel codice esso è in loop infinito. 
Faremo si che TimingDelay, cioè la pausa, faccia la conta fino a zero--> tutto questo tramite l'aiuto dell'handle systick.*/

void Delay(uint32_t nTime){     
    TimingDelay = nTime; 
    while(TimingDelay != 0); //loop infinito fino a quando TimingDelay non diventa 0
}




/*Systick_handler è la funzione che va a "coppia" con la funzione Delay.
IMPORTANTE: un HANDLER è una funzione che si invoca quando avviene un interrupt(un evento esterno che fa sospendere momentaneamente 
l'esecuzione di un programmma). 
In questo caso l'handler systick si avvia ad ogni ciclo di clock(tecnicamente detto'ad ogni TICK'); 
precedentemente avevo spiegato la funzionalità di SysTick_Config e avevamo detto che un ciclo di clock è stato impostato ad 1ms, quindi
ad ogni tick(ogni millisecondo) si invoca questo handler.
COSA FA?
Valuta la variabile passata da Delay che è ntime che a sua volta non è altro che la pause inizializzata nel main; quindi se essa non è zero
allora la decrementiamo. Ciò vuol dire che ogni millisecondo, pausa è decrementata, quindi comporta che la funzione Delay che c'è sopra non rimane in 
un loop infinito.
*/

void SysTick_Handler(void){ 
	if (TimingDelay != 0x00) // se non è uguale a 0
    TimingDelay--; //decrementiamo fino a raggiungere 0 -> in questo modo esce dal while alla riga 291 (funzione: Delay)
}

/*questo frammento finale è un codice che si copia e incolla in ogni programma ha detto il prof. Palesi perchè serve per i debugging,quindi
aiuta il programmatore a rilevare errori nel codice nella compilazione del programma*/
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line) {
/* Infinite loop */
/* Use GDB to find out why we're here */ while (1);
} 
#endif
