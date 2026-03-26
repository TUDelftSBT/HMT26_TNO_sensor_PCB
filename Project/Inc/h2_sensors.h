#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "database.h"

extern struct h2_sensors_t h2_sensors;
extern struct database_bc_hdgn_sensors_can_t h2_sensors_can_bc;

enum h2_sensor_type {
    CAN_H2_SENSOR,
    ANALOGUE_H2_SENSOR
};

struct h2_sensor_t {
    enum h2_sensor_type type;
    double concentration;
    uint8_t disconnected;
    uint8_t consecutive_level;
    uint8_t lel_error;
    // CAN sensor only
    uint8_t status;
    // Analogue sensor only
    uint8_t dma_item;
    uint16_t voltage_value; //DDING THIS -VVIVIAN
};


/* deffining the sensors */
struct h2_sensors_t {
    struct h2_sensor_t can_h2_sensor_port;
    struct h2_sensor_t can_h2_sensor_fc;
    struct h2_sensor_t can_h2_sensor_star;
    struct h2_sensor_t can_h2_sensor_4;
    struct h2_sensor_t analogue_h2_sensor_centre;
};



double convert_ppm_to_lel(double concentration);
uint8_t check_h2_safe();

void store_CAN_h2_sensor_data(struct h2_sensor_t *s, double concentration, uint8_t status);
void read_h2_sensors();
void convert_h2_data_to_boat_CAN();
void send_h2_sensor_status();

void init_h2_sensors();

#ifdef __cplusplus
}
#endif
