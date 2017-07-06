#include <stm32f10x.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_exti.h>
#include <stm32f10x_usart.h>
#include <misc.h>

void Delay(uint32_t nTime);
void halve_pause();

static int pause = 4000;
static int ledval = 0;

int uart_close(USART_TypeDef* USARTx);

int uart_putc(int c, USART_TypeDef* USARTx)
{
        while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET) ;
        USARTx->DR = (c & 0xff);
        return 0;
}

int uart_getc(USART_TypeDef* USARTx)
{
        while (USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == RESET) ;
        return USARTx->DR & 0xff;
}

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

int uart_open(USART_TypeDef* USARTx, uint32_t baud, uint32_t flags)
{
        // Enable bus A and C, since PA5 and PC13 will be used
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA, ENABLE); // (1)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); // (1)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);


        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_StructInit(&GPIO_InitStructure);

	      // Initialize GPIO TX
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

	      // Initialize GPIO RX
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        // PA5 (LED)
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        // PA6 (LED)
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        // PA7 (LED)
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        // PA8 (LED)---7
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        // PC13 (Button)
        GPIO_StructInit(&GPIO_InitStructure);
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOC, &GPIO_InitStructure);

        // see stm32f10x_usart.h
        USART_InitTypeDef USART_InitStructure;
        // Initialize USART structure
        USART_StructInit(&USART_InitStructure);
        USART_InitStructure.USART_BaudRate = baud;
        USART_InitStructure.USART_Mode = flags;
        USART_Init(USARTx,&USART_InitStructure);
        USART_Cmd(USARTx, ENABLE);

}

void EXTI15_10_IRQHandler(void)
{
        if (EXTI_GetITStatus(EXTI_Line13) != RESET)
        {
                halve_pause();
                EXTI_ClearITPendingBit(EXTI_Line13);
        }
}

void halve_pause()
{
        pause = pause/2;
}


/*Prima di andare a implementare la funzione del progetto, dobbiamo prima abilitare tutte le periferiche attraverso TRE STEPS fondamentali e cioè:

1)abilitazione dei clock delle periferiche che si intendono utilizzare e i moduli che essi necessitano (ricordate: ogni periferica dell'STM32 ha
un suo clock locale  e vanno abilitati singolarmente per  ridurre la  dissipazione potenza dinamica cioè dovuta agli switch dei transistor)
esempio una UART(è un dispositivo hardware di uso generale o dedicato. Converte flussi di bit di dati da un formato parallelo a un formato
seriale asincrono o viceversa. Praticamente ogni famiglia di microprocessori ha la sua UART/USART dedicata.)
necessita del modulo UART ma richiede l'accensione del modulo GPIO che controlla l'Input/Output dei pin, del modulo Alternate Function AF che
gestisce le risorse condivise.

2)configurazione dei pin che useremo. (nel nostro caso stiamo usando pin PA5 e pin PC13)

3)Inizializzazione periferica (le periferiche sono tutti i componenti che vedete nella board: USART,NVIC,GPIO,DMA,SPI,TIMER etc.)*/
int main(void)
{
        uint32_t mode  = USART_Mode_Rx | USART_Mode_Tx;
        uint32_t baud = 19200;

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

        GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource13);

        /*questa struttura GPIO_InitStructture che è di tipo GPIO_InitTypeDef deve essere dichiarata,
      	perchè essa contiene tutti i parametri che dobbiamo andare a configurare successivamente per
      	fare funzionare le periferiche di I/O interessate*/
        EXTI_InitTypeDef EXTI_InitStructure;

        EXTI_InitStructure.EXTI_Line = EXTI_Line13;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        EXTI_Init(&EXTI_InitStructure);

        NVIC_InitTypeDef NVIC_InitStructure;

        NVIC_InitStructure.NVIC_IRQChannel =  EXTI15_10_IRQn;
        NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        if (SysTick_Config(SystemCoreClock / 1000)) while (1) ;

        uart_open(USART2,baud,mode);

        char * s1 = "1 led soltanto\r\n";
        char * s2 = "2 led\r\n";
        char * s3 = "3 led\r\n";
        char * s4 = "4 led\r\n";
        char * c;

        volatile int variation;


        static int ledval = 0;

        while (1)
        {

                for(variation=0; variation<=25; variation++) {
                        if(variation <5) {
                                GPIO_WriteBit(GPIOA, GPIO_Pin_5, (ledval) ? Bit_SET : Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_6, (ledval) ? Bit_SET : Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_7, (ledval) ? Bit_SET : Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_8, (ledval) ? Bit_SET : Bit_RESET);
                                Delay(pause);

                                c = s4;
                                Delay(500);
                                while (*c)
                                {
                                        uart_putc(*(c++),USART2);
                                }

                                ledval = 1 - ledval;
                        }
                        else if(variation>5 && variation<10) {
                                GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_6, (ledval) ? Bit_SET : Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_7, (ledval) ? Bit_SET : Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_8, (ledval) ? Bit_SET : Bit_RESET);
                                Delay(pause);
                                c = s3;
                                Delay(500);
                                while (*c)
                                {
                                        uart_putc(*(c++),USART2);
                                }

                                ledval = 1 - ledval;
                        }
                        else if(variation>10 && variation<15) {
                                GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_6, Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_7, (ledval) ? Bit_SET : Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_8, (ledval) ? Bit_SET : Bit_RESET);
                                Delay(pause);

                                c = s2;
                                Delay(500);
                                while (*c)
                                {
                                        uart_putc(*(c++),USART2);
                                }
                                ledval = 1 - ledval;
                        }
                        else if(variation>15 && variation<20) {
                                GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_6, Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_7, Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_8, (ledval) ? Bit_SET : Bit_RESET);
                                Delay(pause);

                                c = s1;
                                Delay(500);
                                while (*c)
                                {
                                        uart_putc(*(c++),USART2);
                                }
                                ledval = 1 - ledval;
                        }
                        else{
                                GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_6, Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_7, Bit_RESET);
                                GPIO_WriteBit(GPIOA, GPIO_Pin_8, Bit_RESET);


                        }
                }
        }
}

static __IO uint32_t TimingDelay;

void Delay(uint32_t nTime){
        TimingDelay = nTime;
	while(TimingDelay != 0);
}

void SysTick_Handler(void){
        if (TimingDelay != 0x00)
                TimingDelay--;
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line) {
/* Infinite loop */
/* Use GDB to find out why we're here */ while (1) ;
}
#endif
