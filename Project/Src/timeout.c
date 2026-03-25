#include "timeout.h"
#include "can.h"
#include "h2_sensors.h"
#include "database.h"

#define MAX_TIMEOUT_SENSORS 500
#define MAX_TIMEOUT_VCU 500


// Structure that keeps track of the last received message of all the pcbs and CAN components
struct timeout_all_components_t timeout_components;

/**
 * Function to check whether too much time has elapsed since the last message
 * @param t
 */
void check_timeout_component(struct timeout_component_t *t) {
    // Look for rising and falling edges, ignore otherwise. This way we can easily keep track of the amount of failing
    // sensors
    if (HAL_GetTick() - t->last_message_tick >= t->max_timeout && t->disconnected == 0) {
        t->disconnected = 1;
        timeout_components.total_disconnected++;
    }
    else if (HAL_GetTick() - t->last_message_tick < t->max_timeout && t->disconnected == 1) {
        t->disconnected = 0;
        timeout_components.total_disconnected--;
    }
}

/**
 * Function to check whether all the pcb and components are still connected
 */
void check_timeout_h2_sensors() {
    // Check each individual struct in the main timeouts struct
    check_timeout_component(&timeout_components.sensor_port);
    check_timeout_component(&timeout_components.sensor_fc);
    check_timeout_component(&timeout_components.sensor_star);
    check_timeout_component(&timeout_components.sensor_4);
}

uint8_t update_vcu_timeout() {
    update_timeout_component(&timeout_components.VCU);

    return get_component_disconnected(&timeout_components.VCU);
}

uint8_t get_component_disconnected(struct timeout_component_t *t) {
    return t->disconnected;
}

/**
 * Function to update the timestamp of the last message for one of the pcbs/components
 *
 * @param t pointer to the structure of the pcb/component that needs to be updated
 */
void update_timeout_component(struct timeout_component_t *t) {
    t->last_message_tick = HAL_GetTick();
}

/**
 * Function to initialise the timeout of pcbs/components
 *
 * @param t pointer to the structure of the pcb/component that needs to be initialised
 * @param max_interval maximum duration between two consecutive CAN messages before and error will be raised (LED light)
 */
static void init_timeout(struct timeout_component_t *t, uint32_t max_interval) {
    update_timeout_component(t);
    t->disconnected = 1;
    t->max_timeout = max_interval;

    timeout_components.total_disconnected++;
}

/**
 * Function to initialise the timeouts for the pcbs and components
 */
void init_timeout_components() {
    timeout_components.total_disconnected = 0;
    init_timeout(&timeout_components.VCU, MAX_TIMEOUT_VCU);
    init_timeout(&timeout_components.sensor_port, MAX_TIMEOUT_SENSORS);
    init_timeout(&timeout_components.sensor_fc, MAX_TIMEOUT_SENSORS);
    init_timeout(&timeout_components.sensor_star, MAX_TIMEOUT_SENSORS);
    // Not used
    init_timeout(&timeout_components.sensor_4, MAX_TIMEOUT_SENSORS);
}