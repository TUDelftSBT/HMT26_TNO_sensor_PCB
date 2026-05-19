#include "main.h"
#include "can.h"
#include "database.h"
#include "can_queue.h"

// TODO:
// Could also be part of the CAN QUEUE library?

#ifdef __cplusplus
extern "C" {
#endif

 /**
  * Callback function that is called when a CAN message is successfully send from mailbox 0. This function will check
  * whether there is a message in the CAN queue that can be put in this mailbox
  *
//   * @param hcan handle of the CAN port
  */
 void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
    
     add_CAN_message_to_mailbox(hcan);
 }
 
 /**
  * Callback function that is called when a CAN message is successfully send from mailbox 1. This function will check
  * whether there is a message in the CAN queue that can be put in this mailbox
  *
  * @param hcan handle of the CAN port
  */
 void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
    HAL_GPIO_TogglePin(LED_DEBUG_1_GPIO_Port, LED_DEBUG_1_Pin);
    
     add_CAN_message_to_mailbox(hcan);
 }
 
 /**
  * Callback function that is called when a CAN message is successfully send from mailbox 2. This function will check
  * whether there is a message in the CAN queue that can be put in this mailbox
  *
  * @param hcan handle of the CAN port
  */
 void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) {
   
     add_CAN_message_to_mailbox(hcan);
 }

#ifdef __cplusplus
}
#endif
