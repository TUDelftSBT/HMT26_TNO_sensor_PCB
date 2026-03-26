/*! \file triceUart.h
\author Thomas.Hoehenleitner [at] seerose.net
*******************************************************************************/

#ifndef TRICE_UART_H_
#define TRICE_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "trice.h"

#if TRICE_DEFERRED_UARTA == 1
#include "main.h"

TRICE_INLINE uint32_t triceTxDataRegisterEmptyUartA(void) {
    return __HAL_UART_GET_FLAG(TRICE_UARTA, UART_FLAG_TXE);
}

//! Write value v into trice transmit register.
//! \param v byte to transmit
//! User must provide this function.
TRICE_INLINE void triceTransmitData8UartA(uint8_t v) {
    // Directly write to the UART data buffer
    // Because otherwise the function is blocking.
    // And somehow HAL_UART_TRANSMIT_IT does not work here
    // So yeah.... okay!
    TRICE_UARTA->Instance->DR = v;
}

//! Allow interrupt for empty trice data transmit register.
//! User must provide this function.
TRICE_INLINE void triceEnableTxEmptyInterruptUartA(void) {
    __HAL_UART_ENABLE_IT(TRICE_UARTA, UART_IT_TXE);
}

//! Disallow interrupt for empty trice data transmit register.
//! User must provide this function.
TRICE_INLINE void triceDisableTxEmptyInterruptUartA(void) {
    __HAL_UART_DISABLE_IT(TRICE_UARTA, UART_IT_TXE);
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* TRICE_UART_H_ */