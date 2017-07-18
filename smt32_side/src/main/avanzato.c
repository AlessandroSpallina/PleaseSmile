//TRUCE
#include <stm32f10x.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_exti.h>
#include <stm32f10x_usart.h>
#include <misc.h>

#define QUEUE_SIZE 2000

//Definizione della coda del buffer
typedef struct Queue {
  uint16_t pRD, pWR;
  uint8_t  q[QUEUE_SIZE];
} TQueue;

TQueue UART1_TXq, UART1_RXq; // Varabili di tipo "coda" in ricezione e in invio

static int Enqueue(TQueue *q, uint8_t data);
static int Dequeue(TQueue *q, uint8_t *data);

int PutChar(char c);
int GetChar(void);
void Delay(uint32_t nTime);
uint32_t mode  = USART_Mode_Rx | USART_Mode_Tx;
uint32_t baud = 19200;void halve_pause();
static int pause = 8000;
static int p = 8;
static int ledval = 0;

// int uart_close(USART_TypeDef* USARTx);

int uart_putc(int c, USART_TypeDef* USARTx)
{
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
    USARTx->DR = (c & 0xff);
    return 0;
}

int uart_getc(USART_TypeDef* USARTx)
{
    while (USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == RESET);
    return USARTx->DR & 0xff;
}

int uart_open(USART_TypeDef* USARTx, uint32_t baud, uint32_t flags)
{
  // Enable bus A and C, since PA5 and PC13 will be used
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE); // (1)
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

// ____ INIZIALIZZAZIONE TX RX PIN 2_3 monitor seriale ____

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);

    // Initialize GPIO TX -- non si usa
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    // Initialize GPIO RX -- non si usa
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);


        // PA5 (LED)
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        // PA6 (LED)---7
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(GPIOA, &GPIO_InitStructure);

        // PA7 (LED)---7
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
    PutChar(p);
    EXTI_ClearITPendingBit(EXTI_Line13);
    }
}

void halve_pause()
{
  if (pause>500){
      pause = pause-1000;
      p=pause/1000;
  }
    else
    {
      p=8;
    pause = 8000;
  }
}


int main(void)
{


    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

//____INIZIALIZZO TX_RX PIN 9_10____

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);

   // Initialize GPIO USART1 TX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    // Initialize GPIO USART1 RX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

//__________________


    // ____ INIZIALIZZAZIONE INTERRUPTS ____

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

  GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource13);
  EXTI_InitTypeDef EXTI_InitStructure;

  EXTI_InitStructure.EXTI_Line = EXTI_Line13;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, DISABLE);

  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_InitStructure.NVIC_IRQChannel =  EXTI15_10_IRQn;
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

   NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn; //seriale con l'interrupt
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

// Initialize USART structure
  USART_InitTypeDef USART_InitStructure;
  USART_StructInit(&USART_InitStructure);

  USART_InitStructure.USART_BaudRate = 19200;

  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; //USART1 sia in scrittura che in lettura
  USART_Init(USART1, &USART_InitStructure);
  USART_Cmd(USART1, ENABLE);

  USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);


  // Init circular buffers (Inizializzo i buffer circolari)
  UART1_TXq.pRD = 0;
  UART1_TXq.pWR = 0;
  UART1_RXq.pRD = 0;
  UART1_RXq.pWR = 0;


  if (SysTick_Config(SystemCoreClock / 1000)) while (1);

    uart_open(USART2,baud,mode);

       char * s1 = "1 led\r\n";
       char * s2 = "2 led\r\n";
       char * s3 = "3 led\r\n";
       char * s4 = "4 led\r\n";
       char * c;

  volatile int variation;


    static int ledval = 0;

    while (1)
   {

     for(variation=0; variation<=25; variation++){
      if(variation <5){
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
        else if(variation>5 && variation<10){
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
      else if(variation>10 && variation<15){
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
      else if(variation>15 && variation<20){
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
    TimingDelay = nTime; while(TimingDelay != 0);
}

void SysTick_Handler(void){
    if (TimingDelay != 0x00)
    TimingDelay--;
}


static int TxPrimed = 0;
int RxOverflow = 0;

void USART1_IRQHandler(void)
{

  if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
  {
    uint8_t data;

    // write one byte to the transmit data register

    if (Dequeue(&UART1_TXq, &data))
      USART_SendData(USART1, data);
    else
    {
      // if we have nothing to send, disable the interrupt
      // and wait for a kick
      USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
      TxPrimed = 0;
    }
  }
}


///////////////////////
int PutChar(char c)
{
  while (!Enqueue(&UART1_TXq, c)) ; //Chiama la funzione Enqueue per inserire la variabile C nella coda. (La funzione è bloccante, se non scrive non esce dal while)

  if (!TxPrimed)
  {
    TxPrimed = 1;
    USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
  }

  return 0;
}

static int QueueFull(TQueue *q) //Funzione che verifica se la coda è piena
{
  return (((q->pWR + 1) % QUEUE_SIZE) == q->pRD);
}

static int QueueEmpty(TQueue *q) //Funzione che verifica se la coda è vuota
{
  return (q->pWR == q->pRD); //Controlla se i puntatori di inzio e fine coda sono uguali
}

//Enqueue prende come parametri una coda e un byte di dati, la sua funzione è aggiungere i dati alla coda.
static int Enqueue(TQueue *q, uint8_t data)
{
  if (QueueFull(q)) //Check se la coda è piena
    return 0;  //Se si esco
  else{ //Se no scrivo nell buffer i dati
     q->q[q->pWR] = data;
     q->pWR = ((q->pWR + 1) == QUEUE_SIZE) ? 0 : q->pWR + 1;
  }
  return 1;
}

//Dequeue è simile, ma l'operazione elimina un byte di dati dalla coda
static int Dequeue(TQueue *q, uint8_t *data)
{
  if (QueueEmpty(q)) //Check se la coda è vuota
    return 0; //Se si esco
  else{ // Se no estraggo i dati dal buffer e li assegno a data
    *data = q->q[q->pRD];
    q->pRD = ((q->pRD + 1) == QUEUE_SIZE) ? 0 : q->pRD + 1;
  }
  return 1;
}


#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line) {
/* Infinite loop */
/* Use GDB to find out why we're here */ while (1);
}
#endif
