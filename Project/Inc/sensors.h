#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "database.h"
#include <stdint.h>



#define MOVING_AVERAGE_WINDOW_SIZE 50
extern struct database_bc_hdgn_chub_sens_lh2_status_t lh2_status;
extern struct database_bc_cryo_chub_sens_h2_status_cool_t cryo_cool_status;

enum sensor_type {
    LIQUID_SENSOR_LEVEL,
    LIQUID_SENSOR_PRESSURE,
    CYRO_TEMP,
    COOLING_TEMP,
    COOLING_FLOW,
    LEAK_DETECTOR
};


/*
enum solenoid_version_t {
    // Solenoid version 1 (port and star)
    SOLENOID_V1,
    // Solenoid version 2 (centre)
    SOLENOID_V2
};
*/
struct moving_average_t {
    // The window of the moving average, all the data will be saved in this array.
    double window[MOVING_AVERAGE_WINDOW_SIZE];
    // This variable will be used to fill the window array. If an item is added to the array, the value will be placed
    // on the 'pointerth' index of the array. This is the easiest way to fill and replace the items in the list in the
    // right order
    uint8_t pointer;
    // To know whether the array is filled for the first time or not, this flag is used. This is necessary for knowing
    // how to calculate the average (a selected number of variables or all the variables)
    uint8_t completed_once_flag;
};

struct sensor_t {
    enum sensor_type type;
    uint8_t dma_item;
    double value;
    struct moving_average_t moving_average;
};

struct sensors_t {
    struct sensor_t liquid_tank_level_sensor;           // level sensor pin *PC0
    struct sensor_t liquid_tank_pressure_sensor_1;      // PT2 sensor pin *PC1
    struct sensor_t liquid_tank_pressure_sensor_2;      // PT3 sensor *PC2
    struct sensor_t liquid_tank_temperature_sensor;     //TC-1 PC3
    struct sensor_t cooling_temp_sensor;                // TS-1 PA0
    struct sensor_t cooling_flow_sensor;                // FM-1 PA4
    struct sensor_t leak_sensor;                        // Leak detector PA6
};

void update_sensor_values();
void send_sensor_values();
void init_sensors();


#ifdef __cplusplus
}
#endif
