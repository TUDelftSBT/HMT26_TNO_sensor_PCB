#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include "main.h"

extern struct timeout_all_components_t timeout_components;

struct timeout_component_t {
    uint8_t disconnected;
    uint32_t max_timeout;
    uint32_t last_message_tick;
};

struct timeout_all_components_t {
    uint8_t total_disconnected;
    struct timeout_component_t sensor_port;
    struct timeout_component_t sensor_fc;
    struct timeout_component_t sensor_star;
    struct timeout_component_t sensor_4;
    struct timeout_component_t VCU;
};

void check_timeout_component(struct timeout_component_t *t);
void check_timeout_h2_sensors();
uint8_t update_vcu_timeout();
uint8_t get_component_disconnected(struct timeout_component_t *t);
void update_timeout_component(struct timeout_component_t *t);
void init_timeout_components();


#ifdef __cplusplus
}
#endif
