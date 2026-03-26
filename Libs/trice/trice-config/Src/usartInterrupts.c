#include "trice_interrupts.h"
#include "trice.h"
#include "main.h"

#if !TRICE_OFF
// ms32 is a 32-bit millisecond counter, counting circular in steps of 1 every ms.
uint32_t ms32 = 0;
#endif

void trice_systick_handler(void) {
    #if !TRICE_OFF
    ms32++;
    #endif
}


void trice_uart_interrupt(void) {
 /* USER CODE BEGIN USART2_IRQn 0 */
#if !TRICE_OFF && defined( TRICE_UARTA ) // only needed. if backchannel is used
    if (__HAL_UART_GET_FLAG(TRICE_UARTA, UART_FLAG_RXNE)) { // Read Data Register Not Empty Flag 

        static char rxBuf[TRICE_COMMAND_SIZE_MAX+1]; // with terminating 0
        static int index = 0;
        uint8_t v;

         if (__HAL_UART_GET_FLAG(TRICE_UARTA, UART_FLAG_ORE)) {
            TRice("WARNING:USARTq OverRun Error Flag is set!\n");
            __HAL_UART_CLEAR_OREFLAG(TRICE_UARTA);
        }

        HAL_UART_Receive(TRICE_UARTA, &v, 1, HAL_MAX_DELAY);
        rxBuf[index] = (char)v;
        index += index < TRICE_COMMAND_SIZE_MAX ? 1 : 0; 
        if( v == 0 ){ // command end
            TRICE_S(Id(0), "rx:received command:%s\n", rxBuf );
            strcpy(triceCommandBuffer, rxBuf );
            triceCommandFlag = 1;
            index = 0;
        }
        return;
    }
#endif // #if !TRICE_OFF && defined( TRICE_UARTA ) // only needed. if backchannel is used
    // If both flags active and only one was served, the IRQHandler gets activated again.

#if !TRICE_OFF && defined( TRICE_UARTA ) && ((TRICE_BUFFER == TRICE_DOUBLE_BUFFER) || (TRICE_BUFFER == TRICE_RING_BUFFER) ) // buffered out to UARTA
    if( __HAL_UART_GET_FLAG(TRICE_UARTA, UART_FLAG_TXE) ){ // Transmit Data Register Empty Flag
        triceServeTransmitUartA();
        return;
    }
#endif
}