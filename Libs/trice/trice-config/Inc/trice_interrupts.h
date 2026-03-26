#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "trice.h"
#include "usart.h"

extern uint32_t ms32;
void trice_systick_handler(void);
void trice_uart_interrupt(void);

#ifdef __cplusplus
}
#endif
