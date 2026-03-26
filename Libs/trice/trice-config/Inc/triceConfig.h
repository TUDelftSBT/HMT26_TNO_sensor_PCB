#ifndef TRICE_CONFIG_H_
#define TRICE_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

// ===========================================================
//         CONFIGURATION SPACE FOR THE DUMB-DUMBS
// ===========================================================

#define TRICE_OFF 0

// Define which uart handle to use.
#define UART_USED_FOR_TRICE huart1

//! TRICE_CLEAN, if found inside triceConfig.h, is modified by the Trice tool to silent editor warnings in the cleaned state.
#define TRICE_CLEAN 0 // Do not define this at an other place! But you can delete this here.

// ===========================================================




// ===========================================================
//       ONLY TOUCH WHEN YOU KNOW WHAT YOU'RE DOING
// ===========================================================

#ifndef TRICE_INLINE
//! TRICE_INLINE is used for inlining trice code to be usable with any compiler. Define this value according to your compiler syntax.
#define TRICE_INLINE static inline
#endif

extern uint32_t ms32; //! ms32 is a 32-bit millisecond counter, counting circular in steps of 1 every ms.
#define TriceStamp16 (SysTick->VAL) // Counts from 31999 -> 0 in each ms.
#define TriceStamp32 ms32           // 1ms, wraps after 2^32 ms ~= 49,7 days

#define TRICE_BUFFER TRICE_DOUBLE_BUFFER
#define TRICE_DEFERRED_BUFFER_SIZE 2048

#include "usart.h"
// Windows: trice log -p com4         -ts16 "time:tick #%6d" -i ../../demoTIL.json -li ../../demoLI.json
// Unix:    trice log -p /dev/ttyACM0 -ts16 "time:tick #%6d" -i ../../demoTIL.json -li ../../demoLI.json
#define TRICE_DEFERRED_OUTPUT 1
#define TRICE_DEFERRED_UARTA 1

extern UART_HandleTypeDef UART_USED_FOR_TRICE;
#define TRICE_UARTA (& UART_USED_FOR_TRICE)

#include "cmsis_gcc.h"
#define TRICE_ENTER_CRITICAL_SECTION             \
	{                                            \
		uint32_t primaskstate = __get_PRIMASK(); \
		__disable_irq();                         \
		{

#define TRICE_LEAVE_CRITICAL_SECTION \
	}                                \
    __set_PRIMASK(primaskstate);     \
	}

#ifdef __cplusplus
}
#endif

#endif /* TRICE_CONFIG_H_ */