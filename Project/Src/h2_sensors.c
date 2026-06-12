#include "h2_sensors.h"

#include <math.h>

#include "main.h"
#include "database.h"
#include "can.h"
#include "can_queue.h"
#include "timeout.h"

// LEL boundary at which the solenoids will be closed to prevent a fire or explosion in case of an H2 leak. The value
// can lie anywhere between 0 and 100%
#define LEL_WARNING_BOUNDARY 10
#define LEL_ERROR_BOUNDARY 30
// The number of consecutive CAN messages from one of the sensors before all the solenoids will be closed. This is to
// prevent closing all the solenoids in case of one wrongly measured value or outlier
#define CONSECUTIVE_HIGHS_BOUNDARY 5
#define CONSECUTIVE_LOWS_BOUNDARY 5

// Structure that contains all the H2 sensors
struct h2_sensors_t h2_sensors;
// Structure of the H2 data that will be sent on the boat CAN. This is a summary of the most important data that was sent
// by the H2 sensors
struct database_bc_hdgn_sensor_status_t sensor_status_bc;
struct database_bc_hdgn_sensors_can_t h2_sensors_can_bc;
struct database_bc_hdgn_sensors_analogue_t h2_sensors_analogue_bc;
//struct database_bc_hdgn_analogue_vi_t h2_analogue_vi_bc;  moved to sensors.c because it is only used there


double convert_ppm_to_lel(double concentration) {
  return concentration / 400;
}

/**
 * Function to check if there are any sensors that measure a value that is higher than the set boundary
 *
 * @return The amount of sensors where the there are at least CONSECUTIVE_HIGHS_BOUNDARY consecutive messages that were
 * above the set LEL boundaryCAN-Queue git@github.com:TUDelftSBT/HMT24_CAN_Queue.git master
 */
uint8_t check_h2_safe() {
  uint8_t counter = 0;

  // Put all the LEL values in a list to easily find the max LEL
  double lels[] = {convert_ppm_to_lel(h2_sensors.can_h2_sensor_port.concentration),
                   convert_ppm_to_lel(h2_sensors.can_h2_sensor_fc.concentration),
                   convert_ppm_to_lel(h2_sensors.can_h2_sensor_star.concentration),
                   convert_ppm_to_lel(h2_sensors.can_h2_sensor_4.concentration),
                   convert_ppm_to_lel(h2_sensors.analogue_h2_sensor_centre.concentration)
  };
  double max_lel = 0;
  for (uint8_t i = 0; i < 5; i++) {
    if (lels[i] > max_lel) {
      max_lel = lels[i];
    }
  }

  if (h2_sensors.can_h2_sensor_port.lel_error) {
    counter++;
  }
  if (h2_sensors.can_h2_sensor_fc.lel_error) {
    counter++;
  }
  if (h2_sensors.can_h2_sensor_star.lel_error) {
    counter++;
  }
  if (h2_sensors.can_h2_sensor_4.lel_error) {
    counter++;
  }
  if (h2_sensors.analogue_h2_sensor_centre.lel_error) {
    counter++;
  }


  if (counter >= 1 && !sensor_status_bc.lel_error) {
    uint8_t data[8];

    sensor_status_bc.lel_error = 1;
    database_bc_hdgn_sensors_can_pack(data, &h2_sensors_can_bc, DATABASE_BC_HDGN_SENSORS_CAN_LENGTH);
    queue_CAN_message(&hcan2, DATABASE_BC_HDGN_SENSORS_CAN_FRAME_ID, DATABASE_BC_HDGN_SENSORS_CAN_LENGTH, data);
  }
  else if (counter == 0 && sensor_status_bc.lel_error){
    uint8_t data[8];

    sensor_status_bc.lel_error = 0;
    database_bc_hdgn_sensors_can_pack(data, &h2_sensors_can_bc, DATABASE_BC_HDGN_SENSORS_CAN_LENGTH);
    queue_CAN_message(&hcan2, DATABASE_BC_HDGN_SENSORS_CAN_FRAME_ID, DATABASE_BC_HDGN_SENSORS_CAN_LENGTH, data);
  }

  if (max_lel > LEL_WARNING_BOUNDARY && !sensor_status_bc.lel_warning) {
    uint8_t data[8];

    sensor_status_bc.lel_warning = 1;
    database_bc_hdgn_sensors_can_pack(data, &h2_sensors_can_bc, DATABASE_BC_HDGN_SENSORS_CAN_LENGTH);
    queue_CAN_message(&hcan2, DATABASE_BC_HDGN_SENSORS_CAN_FRAME_ID, DATABASE_BC_HDGN_SENSORS_CAN_LENGTH, data);
  }
  else if (max_lel < LEL_WARNING_BOUNDARY && sensor_status_bc.lel_warning) {
    uint8_t data[8];

    sensor_status_bc.lel_warning = 0;
    database_bc_hdgn_sensors_can_pack(data, &h2_sensors_can_bc, DATABASE_BC_HDGN_SENSORS_CAN_LENGTH);
    queue_CAN_message(&hcan2, DATABASE_BC_HDGN_SENSORS_CAN_FRAME_ID, DATABASE_BC_HDGN_SENSORS_CAN_LENGTH, data);
  }

    // //todo: TESTING IF I CAN SEND ANALOGUE MESSAEG
    // uint8_t data[8];
    // database_bc_hdgn_sensors_analogue_pack(data, &h2_sensors_analogue_bc, DATABASE_BC_HDGN_SENSORS_ANALOGUE_LENGTH);
    // queue_CAN_message(&hcan2, DATABASE_BC_HDGN_SENSORS_ANALOGUE_FRAME_ID, DATABASE_BC_HDGN_SENSORS_ANALOGUE_LENGTH, data);

  return counter;
}

void store_CAN_h2_sensor_data(struct h2_sensor_t *s, double concentration, uint8_t status) {
    if (s->type != CAN_H2_SENSOR) {
        return;
    }

    s->concentration = concentration;
    s->status = status;
}

static void check_CAN_sensor_disconnected(struct h2_sensor_t *s) {

    if (s->type != CAN_H2_SENSOR) {
        return;
    }

    if (s == &h2_sensors.can_h2_sensor_port) {
        s->disconnected = get_component_disconnected(&timeout_components.sensor_port);
    }
    else if (s == &h2_sensors.can_h2_sensor_fc) {
      s->disconnected = get_component_disconnected(&timeout_components.sensor_fc);
    }
    else if (s == &h2_sensors.can_h2_sensor_star) {
      s->disconnected = get_component_disconnected(&timeout_components.sensor_star);
    }
    else if (s == &h2_sensors.can_h2_sensor_4) {
      s->disconnected = get_component_disconnected(&timeout_components.sensor_4);
    }
}

/**
 * Function to read analogue value of the analogue h2 sensors
 *
 * @param s pointer to the sensor
 */
static void read_analogue_h2_sensor(struct h2_sensor_t *s) {
    if (s->type != ANALOGUE_H2_SENSOR) {
        return;
    }

    // Use the polling ADC read function to bypass DMA
    extern double ADC_Read_Channel(uint32_t channel);
    double V_o = ADC_Read_Channel(s->dma_item); // Using dma_item as the ADC channel
    
    uint16_t dma_value = (uint16_t)((V_o / 3.3) * 4095.0); // Recover raw value for CAN message

    // The h2 sensor is ratiometric what means that it is linear. 0-0.5V is non-valid and 0.5-4.5V is expected
    // behaviour. 4.5V to 4.6V is overscale?
    // The voltage is mapped from 0-5V to 0-3V3 to be measurable by the stm. Therefore, the actual voltage can
    //  be calculated by: V_i = (R_1 + R_2) / R_2 * V_o.
    // With this voltage, the concentration can be calculated. 0.5V represents 0 ppm and 4.5V represents 40.000 ppm.
    //  Therefore, the pressure can be calculated by (V_i - 0.5) * Delta P / Delta V = (V_i - 0.5) * (40.000 - 0) /
    //  (4.5 - 0.5) = V_i * 10e3 - 5e3
    double R_1 = 120;
    double R_2 = 320; //was originally 10k resistor
    double V_i = V_o * (R_1 + R_2) / R_2;
    double max_concentration = 20000 ; //20000.0 ;
    double min_concentration = 0 ;
    double min_voltage = 0.5;
    double max_voltage = 4.5;
    double a = (max_concentration - min_concentration) / (max_voltage-min_voltage);
    double b  = min_concentration - a*min_voltage;

    // If the measured voltage equals an invalid value, set the pressure to 0 bar
    if (V_i < 0.5 || V_i > 4.6) { //wAS 0.5
        s->disconnected = 1;;//2;//s->dma_item;//1;
        s->concentration = 0;//V_i; // TODO: remove this bc i added this myself vivian
        s->voltage_value=dma_value;
        return;
    }

		// TODO: Change back to value when connected
        s->concentration = ((V_i * a + b ));
        s->disconnected = 0;

		//s->concentration = V_i;// V_i;//0;//_i; // 0; 0 is the oriinal
		//s->disconnected = 0;////0;//5;//s->dma_item;//0; // TODO: make this 0 or 1
        s->voltage_value=dma_value;
}

/**
 * Function to check whether the measured hydrogen concentration is higher than allowed. If this is the case, the
 * s->lel_error flag will be set high (and low otherwise of course)
 *
 * @param s pointer to the sensor that needs to be checked.
 */
static void check_sensor_lel(struct h2_sensor_t *s) {
  // Calculate the LEL of the sensor
  double lel = convert_ppm_to_lel(s->concentration);

  // Determine whether the value has been above or below the boundary for at least five consecutive measurements (500ms).
  // If this is the case, the error is changed.
  if (s->lel_error) {
    if (lel >= LEL_ERROR_BOUNDARY) {
      s->consecutive_level = 0;
    }
    else if (s->consecutive_level < CONSECUTIVE_LOWS_BOUNDARY) {
      (s->consecutive_level)++;
    }

    if (s->consecutive_level >= CONSECUTIVE_LOWS_BOUNDARY) {
      s->lel_error = 0;
      s->consecutive_level = 0;
    }
  }
  else {
    if (lel < LEL_ERROR_BOUNDARY) {
      s->consecutive_level = 0;
    }
    else if (s->consecutive_level < CONSECUTIVE_HIGHS_BOUNDARY) {
      (s->consecutive_level)++;
    }

    if (s->consecutive_level >= CONSECUTIVE_HIGHS_BOUNDARY) {
      s->lel_error = 1;
      s->consecutive_level = 0;
    }
  }
}

/**
 * Read one h2 sensor
 *
 * @param s pointer to the sensor struct
 */
static void read_h2_sensor(struct h2_sensor_t *s) {
    if (s->type == CAN_H2_SENSOR) {
        // Values are already up-to-date, but check whether they are disconnected or not
        check_CAN_sensor_disconnected(s);
    }
    else if (s->type == ANALOGUE_H2_SENSOR) {
        read_analogue_h2_sensor(s);
    }
    else {
        return;
    }

    // If a sensor is disconnected, make the value -4000 ppm to inform that the sensor is not working
    if (s->disconnected) {
        s->concentration = -4000;//
    }

    check_sensor_lel(s);
}

/**
 * Function to read value of the h2 sensors and check whether it is connected or not
 */
void read_h2_sensors() {
  read_h2_sensor(&h2_sensors.can_h2_sensor_port);
  read_h2_sensor(&h2_sensors.can_h2_sensor_fc);
  read_h2_sensor(&h2_sensors.can_h2_sensor_star);
  read_h2_sensor(&h2_sensors.can_h2_sensor_4);
  read_h2_sensor(&h2_sensors.analogue_h2_sensor_centre);

}

/**
 * Convert and put the data of a CAN h2 sensor to the boat CAN message
 *
 * @param s pointer to the sensor struct
 */
static void convert_CAN_h2_data_to_boat_CAN(struct h2_sensor_t *s) {
    if (s->type != CAN_H2_SENSOR) {
        return;
    }

    if (s == &h2_sensors.can_h2_sensor_port) {
        // CAN sensor message
        h2_sensors_can_bc.can_sensor_port_h2_concentration = database_bc_hdgn_sensors_can_can_sensor_port_h2_concentration_encode(s->concentration);
        h2_sensors_can_bc.can_sensor_port_lel = database_bc_hdgn_sensors_can_can_sensor_port_lel_encode(convert_ppm_to_lel(s->concentration));
        // HDGN status message
        sensor_status_bc.can_sensor_port_disconnected = s->disconnected;
        sensor_status_bc.can_sensor_port_status = s->status;
    }
    else if (s == &h2_sensors.can_h2_sensor_fc) {
        // CAN sensor message
        h2_sensors_can_bc.can_sensor_fc_h2_concentration = database_bc_hdgn_sensors_can_can_sensor_fc_h2_concentration_encode(s->concentration);
        h2_sensors_can_bc.can_sensor_fc_lel = database_bc_hdgn_sensors_can_can_sensor_fc_lel_encode(convert_ppm_to_lel(s->concentration));
        // HDGN status message
        sensor_status_bc.can_sensor_fc_disconnected = s->disconnected;
        sensor_status_bc.can_sensor_fc_status = s->status;
    }
    else if (s == &h2_sensors.can_h2_sensor_star) {
      // CAN sensor message
      h2_sensors_can_bc.can_sensor_star_h2_concentration = database_bc_hdgn_sensors_can_can_sensor_star_h2_concentration_encode(s->concentration);
      h2_sensors_can_bc.can_sensor_star_lel = database_bc_hdgn_sensors_can_can_sensor_star_lel_encode(convert_ppm_to_lel(s->concentration));
      // HDGN status message
      sensor_status_bc.can_sensor_star_disconnected = s->disconnected;
      sensor_status_bc.can_sensor_star_status = s->status;
    }
    else if (s == &h2_sensors.can_h2_sensor_4) {
      // CAN sensor message
      // h2_sensors_can_bc.can_sensor_4_h2_concentration = database_bc_hdgn_sensors_can_can_sensor_4_h2_concentration_encode(s->concentration);
      // h2_sensors_can_bc.can_sensor_4_lel = database_bc_hdgn_sensors_can_can_sensor_4_lel_encode(convert_ppm_to_lel(s->concentration));
      // h2_sensors_can_bc.can_sensor_4_h2_concentration = database_bc_hdgn_sensors_can_can_sensor_4_h2_concentration_encode(s->concentration);
      // h2_sensors_can_bc.can_sensor_4_lel = database_bc_hdgn_sensors_can_can_sensor_4_lel_encode(convert_ppm_to_lel(s->concentration));
      // HDGN status message
      // sensor_status_bc.can_sensor_4_disconnected = s->disconnected;
      // sensor_status_bc.can_sensor_4_status = s->status;
      // sensor_status_bc.can_sensor_4_disconnected = s->disconnected;
      // sensor_status_bc.can_sensor_4_status = s->status;
    }
}

static void convert_analogue_h2_data_to_boat_CAN(struct h2_sensor_t *s) {
    if (s->type != ANALOGUE_H2_SENSOR) {
        return;
    }

    if (s == &h2_sensors.analogue_h2_sensor_centre) {
        // Analogue sensor message
        h2_sensors_analogue_bc.analogue_sensor_centre_h2_concentration =
                database_bc_hdgn_sensors_analogue_analogue_sensor_centre_h2_concentration_encode(s->concentration);
        h2_sensors_analogue_bc.analogue_sensor_centre_lel =
                database_bc_hdgn_sensors_analogue_analogue_sensor_centre_lel_encode(convert_ppm_to_lel(s->concentration));
        // HDGN status message
        // sensor_status_bc.analog_sensor_centre_disconnected = s->disconnected;
        // sensor_status_bc.analog_sensor_centre_disconnected = s->disconnected;
    }
}

/**
 * Function to convert the received data from the sensors to the format that is required for the boat CAN
 *
 * @param sensor_ID CAN id of the sensor itself. This way the data is put in the right bits in the right message
 */
void convert_h2_data_to_boat_CAN() {
    convert_CAN_h2_data_to_boat_CAN(&h2_sensors.can_h2_sensor_port);
    convert_CAN_h2_data_to_boat_CAN(&h2_sensors.can_h2_sensor_fc);
    convert_CAN_h2_data_to_boat_CAN(&h2_sensors.can_h2_sensor_star);
    convert_CAN_h2_data_to_boat_CAN(&h2_sensors.can_h2_sensor_4);
    convert_analogue_h2_data_to_boat_CAN(&h2_sensors.analogue_h2_sensor_centre);


    check_h2_safe();
}

void send_h2_sensor_status() {
    uint8_t data[8];
    database_bc_hdgn_sensors_analogue_pack(data, &h2_sensors_analogue_bc, DATABASE_BC_HDGN_SENSORS_ANALOGUE_LENGTH);
    queue_CAN_message(&hcan2, DATABASE_BC_HDGN_SENSORS_ANALOGUE_FRAME_ID, DATABASE_BC_HDGN_SENSORS_ANALOGUE_LENGTH, data);
    database_bc_hdgn_sensor_status_pack(data, &sensor_status_bc, DATABASE_BC_HDGN_SENSOR_STATUS_LENGTH);
    queue_CAN_message(&hcan2, DATABASE_BC_HDGN_SENSOR_STATUS_FRAME_ID, DATABASE_BC_HDGN_SENSOR_STATUS_LENGTH, data);
    database_bc_hdgn_sensors_can_pack(data, &h2_sensors_can_bc, DATABASE_BC_HDGN_SENSORS_CAN_LENGTH);
    queue_CAN_message(&hcan2, DATABASE_BC_HDGN_SENSORS_CAN_FRAME_ID, DATABASE_BC_HDGN_SENSORS_CAN_LENGTH, data);

   }

static void init_h2_sensor(struct h2_sensor_t *s, enum h2_sensor_type type, uint8_t dma_item) {
    s->type = type;
    s->concentration = -4000;
    s->disconnected = 1;
    s->consecutive_level = 0;
    s->lel_error = 0;
    if (type == CAN_H2_SENSOR) {
        s->status = 0;
    }
    else {
        s->status = -1;
    }
    s->dma_item = dma_item;
}

void init_h2_sensors() {
    init_h2_sensor(&h2_sensors.can_h2_sensor_port, CAN_H2_SENSOR, -1);
    init_h2_sensor(&h2_sensors.can_h2_sensor_fc, CAN_H2_SENSOR, -1);
    init_h2_sensor(&h2_sensors.can_h2_sensor_star, CAN_H2_SENSOR, -1);
    init_h2_sensor(&h2_sensors.can_h2_sensor_4, CAN_H2_SENSOR, -1);
    
    // Safe to re-enable without DMA! Use the proper ADC channel macro.
    // Update ADC_CHANNEL_6 if the analogue sensor uses a different pin!
    init_h2_sensor(&h2_sensors.analogue_h2_sensor_centre, ANALOGUE_H2_SENSOR, ADC_CHANNEL_6);

}