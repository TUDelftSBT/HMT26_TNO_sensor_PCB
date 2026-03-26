//
// Created by Teund on 02/02/2026.
//

#include "app.h"
#include "h2_sensors.h"
#include "sensors.h"
#include "can.h"

#define H2_SENSOR_MESSAGE_INTERVAL 100

static uint32_t h2_tick = 0;

void app_init(void) {
    // Initialization of the application
    HAL_CAN_Start(&hcan2);
    init_sensors();
    init_h2_sensors();
    h2_tick = HAL_GetTick();
}

void app_loop(void) {
    if (HAL_GetTick() - h2_tick >= H2_SENSOR_MESSAGE_INTERVAL) {
        read_h2_sensors();
        convert_h2_data_to_boat_CAN();
        send_h2_sensor_status();
        
        update_sensor_values();
        send_sensor_values();
        
        h2_tick = HAL_GetTick();
    }
}
