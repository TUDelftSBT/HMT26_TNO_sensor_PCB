//
// Created by julia on 28-5-2025.
//

#include "sensors.h"
#include "database.h"
#include "can_queue.h"
#include "can.h"
#include "adc.h"
#include <math.h>
#include "main.h"
#include <stdio.h>

//#define NUMBER_OF_TANKS_IN_USE 3

struct sensors_t sensors;

struct database_bc_hdgn_chub_sens_lh2_status_t lh2_status;
struct database_bc_cryo_chub_sens_h2_status_cool_t cryo_cool_status;

struct database_bc_hdgn_analogue_vi_t h2_analogue_vi_bc;

static void init_sensor(struct sensor_t* s, enum sensor_type type, uint32_t adc_channel);

double ADC_Read_Channel(uint32_t channel); // functie direct adc uitlezen 

double ADC_Read_Channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint16_t raw_value = 0;
    double pin_v = 0.0;

    /* 1. Configure the ADC to target ONLY the requested channel */
    sConfig.Channel = channel;
    sConfig.Rank = 1;                             // Always Rank 1 since we read one at a time
    sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES; // 56+ cycles prevents channels bleeding into each other
    
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        // Configuration failed - handle error or return 0
        return 0.0; 
    }

    /* 2. Run the sampling sequence */
    HAL_ADC_Start(&hadc1);
    
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
        raw_value = HAL_ADC_GetValue(&hadc1);
    }
    
    HAL_ADC_Stop(&hadc1); // Stop and release the ADC lock
    pin_v = 3.3 * raw_value / 4095.0;
    return pin_v;
}


/**
 * Function to add a datapoint to the moving average
 *
 * @param ma pointer to the moving average struct
 * @param value the datapoint that needs to be added
 * 
 */
static void __attribute__((unused)) add_to_moving_average(struct moving_average_t* ma, double value)
{
  ma->window[ma->pointer] = value;

  ma->pointer++;
  if (ma->pointer == MOVING_AVERAGE_WINDOW_SIZE)
  {
    ma->pointer = 0;
    ma->completed_once_flag = 1;
  }
}

/**
 * Function to calculate the moving average of a window
 *
 * @param ma pointer to the moving average struct of which the average will be calculated
 * @return the average. NaN will be returned if the array is empty
 */
static double __attribute__((unused)) get_moving_average(struct moving_average_t* ma)
{
  // If the array is completely filled with values
  if (ma->completed_once_flag)
  {
    double sum = 0;
    for (int i = 0; i < MOVING_AVERAGE_WINDOW_SIZE; i++)
    {
      sum += ma->window[i];
    }
    return sum / MOVING_AVERAGE_WINDOW_SIZE;
  }

  // If the array is only partly filled, calculating the average only using the filled values
  if (ma->pointer != 0)
  {
    double sum = 0;
    for (int i = 0; i < ma->pointer; i++)
    {
      sum += ma->window[i];
    }
    return sum / ma->pointer;
  }

  // This should only be reached if the array is empty
  return NAN;
}


  void init_sensors()
  {
    init_sensor(&sensors.cooling_temp_sensor, COOLING_TEMP, ADC_CHANNEL_0);                        // PA0 / TS1_Pin
    init_sensor(&sensors.cooling_flow_sensor, COOLING_FLOW, ADC_CHANNEL_4);                        // PA4 / FM1_Pin
    init_sensor(&sensors.liquid_tank_level_sensor, LIQUID_SENSOR_LEVEL, ADC_CHANNEL_10);          // PC0 / Level_sensor_Pin
    init_sensor(&sensors.liquid_tank_pressure_sensor_1, LIQUID_SENSOR_PRESSURE, ADC_CHANNEL_11);  // PC1 / PT2_Pin
    init_sensor(&sensors.liquid_tank_pressure_sensor_2, LIQUID_SENSOR_PRESSURE, ADC_CHANNEL_12);  // PC2 / PT3_Pin
    init_sensor(&sensors.liquid_tank_temperature_sensor, CYRO_TEMP, ADC_CHANNEL_13);               // PC3 / TC_1_Pin
    init_sensor(&sensors.leak_sensor, LEAK_DETECTOR, ADC_CHANNEL_6);                                // PA6 / Index 6
  }


static void update_sensor_value(struct sensor_t* s)
{
  // Read the voltage directly from the ADC channel
  double V_o = ADC_Read_Channel(s->adc_channel);

        if (V_o >= 0.6)
      {
        HAL_GPIO_TogglePin(LED_DEBUG_5_GPIO_Port, LED_DEBUG_5_Pin);
      }
      HAL_GPIO_TogglePin(LED_DEBUG_4_GPIO_Port, LED_DEBUG_4_Pin);

  //printf("sensor.c - ADC channel %lu: V_o = %fV\r\n", s->adc_channel, V_o);

  switch (s->type)
  {
  case LIQUID_SENSOR_PRESSURE:
    {
      // The pressure sensor is ratiometric what means that it is linear. 0-0.5V is non-valid and 0.5-4.5V is expected
      // behaviour.
      // Again, the voltage is mapped from 0-5V to 0-3V3 to be measurable by the stm. Therefore, the actual voltage can
      //  be calculated by: V_i = (R_1 + R_2) / R_2 * V_o.
      // With this voltage, the pressure can be calculated. 0.5V represents 0 bar and 4.5V represents 448 bar. Therefore,
      //  the pressure can be calculated by (V_i - 0.5) * Delta P / Delta V = (V_i - 0.5) * (448 - 0) / (4.5 - 0.5) =
      //  V_i * 112 - 66
      double R_1 = 5000;
      double R_2 = 18000;

      // Sensor output voltage
      double V_i = V_o * (R_1 + R_2) / R_2;

      double max_pressure = 14.0;
      double min_pressure = 0.0;
      double voltage_max = 5.0;
      double voltage_min = 0.0;
      double a = (max_pressure- min_pressure)/ (voltage_max-voltage_min);
      double b = min_pressure - a*voltage_min;

      
      /*
      // check als voltage word gemeten 
      if (V_i >= 0.2)
      {
        HAL_GPIO_TogglePin(LED_DEBUG_5_GPIO_Port, LED_DEBUG_5_Pin);

       return;
      }
      HAL_GPIO_TogglePin(LED_DEBUG_4_GPIO_Port, LED_DEBUG_4_Pin);
      */
      

      // Actual pressure
      s->value =   V_i * a + b + 1; // unknown offset needed hmt25 I appologize I'm not going to look futher into it.
      break;
    }

  case LIQUID_SENSOR_LEVEL:
    {
      // The temperture sensor is linear because of the transmitter that ouput a 4mA to 20mA signal.
      // This is mapped over a presion reistor into a voltage readout by the stm
      // Again, the voltage is mapped from 0-5V to 0-3V3 to be measurable by the stm.
      // No voltage division needed with this sensor.
      // With this voltage, the pressure can be calculated. 0.66V represents 0 degrees and 3.3 V represents 100 degrees. Therefore,
      //  the temperature can be calculated by y = ax+b. This thing is programmable with an app so you want to keep it modular
      // the story above is bullshit plz eggnore
      double R_1 = 149.5;
      double min_level = 0.0;
      double max_level = 100.0;
      double min_current = 0.004;
      double max_current = 0.020;
      double a = (max_level- min_level)/ (R_1*max_current-R_1*min_current); // still needs to be checked
      double b = min_level - a*min_current*R_1;

      // If the measured voltage equals an invalid value, set the pressure to 0 degreese
      if (V_o <= R_1*min_current) {
        s->value = 0;
        return;
      }
      /*
       if (V_o >= 0.2)
      {
        HAL_GPIO_TogglePin(LED_DEBUG_5_GPIO_Port, LED_DEBUG_5_Pin);

       return;
      }
      HAL_GPIO_TogglePin(LED_DEBUG_4_GPIO_Port, LED_DEBUG_4_Pin);
      */
      
      // Actual temperture
      s->value = V_o * a + b;
      break;

    }

  case CYRO_TEMP:
    {
      // The temperture sensor is linear because of the transmitter that ouput a 4mA to 20mA signal.
      // This is mapped over a presion reistor into a voltage readout by the stm
      // Again, the voltage is mapped from 0-5V to 0-3V3 to be measurable by the stm.
      // No voltage division needed with this sensor.
      // With this voltage, the pressure can be calculated. 0.66V represents 0 degrees and 3.3 V represents 100 degrees. Therefore,
      //  the temperature can be calculated by y = ax+b. This thing is programmable with an app so you want to keep it modular
      // the story above is bullshit plz eggnore
      double R_1 = 149.46;
      double min_temp = -40;
      double max_temp = 80;
      double min_current = 0.004;
      double max_current = 0.020;
      double a = (max_temp- min_temp)/ (R_1*max_current-R_1*min_current);
      double b = min_temp - a*min_current*R_1;
      //double a = (max_temp- min_temp)/ (voltage_dif-(R_1*min_current));
      //double b = min_temp - a*min_current*R_1;

      // If the measured voltage equals an invalid value, set the pressure to 0 degreese
      if (V_o <= R_1*min_current) {
        s->value = 0;
        return;
      }
      /*
      if (V_o >= 0.2)
      {
        HAL_GPIO_TogglePin(LED_DEBUG_5_GPIO_Port, LED_DEBUG_5_Pin);

       return;
      }
      HAL_GPIO_TogglePin(LED_DEBUG_4_GPIO_Port, LED_DEBUG_4_Pin);
      */
      // Actual temperture
      s->value = V_o * a + b;
      break;
    }


  case COOLING_TEMP:
    {

      double R_1 = 149.46;
      double min_temp = -40;
      double max_temp = 80;
      double min_current = 0.004;
      double max_current = 0.020;
      double a = (max_temp- min_temp)/ (R_1*max_current-R_1*min_current);
      double b = min_temp - a*min_current*R_1;
      //double a = (max_temp- min_temp)/ (voltage_dif-(R_1*min_current));
      //double b = min_temp - a*min_current*R_1;

      // If the measured voltage equals an invalid value, set the pressure to 0 degreese
      if (V_o <= R_1*min_current) {
        s->value = 0;
        return;
      }


      if (V_o >= 0.2)
      {
        HAL_GPIO_TogglePin(LED_DEBUG_5_GPIO_Port, LED_DEBUG_5_Pin);
      }
       
      HAL_GPIO_TogglePin(LED_DEBUG_4_GPIO_Port, LED_DEBUG_4_Pin);
      // Actual temperture
      s->value = V_o * a + b;
      break;

      
    }

  case COOLING_FLOW:
    {

      double R_1 = 149.46;
      double min_temp = 0;
      double max_temp = 25;
      double min_current = 0.004;
      double max_current = 0.020;
      double a = (max_temp- min_temp)/ (R_1*max_current-R_1*min_current);
      double b = min_temp - a*min_current*R_1;
      //double a = (max_temp- min_temp)/ (voltage_dif-(R_1*min_current));
      //double b = min_temp - a*min_current*R_1;

      // If the measured voltage equals an invalid value, set the pressure to 0 degreese
      if (V_o <= R_1*min_current) {
        s->value = 0;
        return;
      }
      /*
       if (V_o >= 0.2)
      {
        HAL_GPIO_TogglePin(LED_DEBUG_5_GPIO_Port, LED_DEBUG_5_Pin);

       return;
      }
       
      HAL_GPIO_TogglePin(LED_DEBUG_4_GPIO_Port, LED_DEBUG_4_Pin);
      */
      // Actual temperture
      s->value = V_o * a + b;
      break;
    }

  case LEAK_DETECTOR:
    {

      double R_1 = 5000;
      double R_2 = 10000;
      double V_i = V_o * (R_1 + R_2) / R_2;
      double max_concentration = 20000 ; //20000.0 ;
      double min_concentration = 0 ;
      double min_voltage = 0.5;
      double max_voltage = 4.5;
      double a = (max_concentration - min_concentration) / (max_voltage-min_voltage);
      double b  = min_concentration - a*min_voltage;



        if (V_i < 0.5 || V_i > 4.6) { //wAS 0.5
        s->value = 0;
        return;
    }

         s->value =   V_i * a + b ; // unknown offset needed hmt25 I appologize I'm not going to look futher into it.
      break;
    }

  default:
    break;

    
  }
}
  // static void update_h2_mass_estimation()
  // {
  //   // To estimate the total H2 mass in the tanks, the ideal gas law can be used. However, it becomes harder to compress
  //   //  gas as the pressure increases. To account for this, an extra factor is introduced: the compressibility factor Z.
  //   //  This results in the following equation:
  //   // pV = nRTZ,
  //   //  where p is the pressure in Pa, V is the volume in m^3, n is the amount of molecules, R the universal gas constant
  //   //  in J/(mol K), T is the temperature in K and Z the compressibility factor. We are interested in the mass m, which
  //   //  can obtained by m = n * M, where M is mass of hydrogen per mole. Combining all of this results in:
  //   // m = MpV/(RTZ).
  //   // To calculate the mass, we need Z. To conclude a paper (https://doi.org/10.6028/jres.113.028) the Lemmon equation
  //   //  does this. Stating complicated formulas in CLion is a pain, so check eq.3 in the paper if you are interested.
  //
  //   // Take the average of the three temperature sensor to calculate T. If a sensor reads -100 degrees Celsius, assume
  //   // that that tank is not connected. Use the tanks that are assumed to be connected this way to calculate the total
  //   // volume. Each tank has a volume of 350L or 0.350e-3 m^3. Check what thanks return a realistic pressure and use
  //   // these values to calculate the average pressure. The pressure in al thanks should be similar, so even with fewer
  //   // sensors than tanks, the mass can be calculated relatively accurate.
  //   uint8_t tanks_connected = 0;
  //
  //
  //   // Temperature (K)
  //   double T = 0;
  //   if (sensors.compressed_tank_temp_sensor.value > -50)
  //   {
  //     T += sensors.compressed_tank_temp_sensor.value;
  //     tanks_connected++;
  //   }
  //   /*
  //   if (sensors.temperature_sensor_centre.value > -50) {
  //     T += sensors.temperature_sensor_centre.value;
  //     tanks_connected++;
  //   }
  //   if (sensors.temperature_sensor_star.value > -50) {
  //     T += sensors.temperature_sensor_star.value;
  //     tanks_connected++;
  //   }
  //   */
  //
  //
  //   T /= tanks_connected;
  //   T += 273.15;
  //   // Pressure (Pa)
  //   double p = 0;
  //   uint8_t number_of_pressure_sensors = 0;
  //   if (sensors.compressed_pressure_sensor.value > 1)
  //   {
  //     p += sensors.compressed_pressure_sensor.value;
  //     number_of_pressure_sensors++;
  //   }
  //   /*
  //   if (sensors.pressure_sensor_centre.value > 1) {
  //     p += sensors.pressure_sensor_centre.value;
  //     number_of_pressure_sensors++;
  //   }
  //   if (sensors.pressure_sensor_star.value > 1) {
  //     p += sensors.pressure_sensor_star.value;
  //     number_of_pressure_sensors++;
  //   }
  //   */
  //   p = p * 1e5 * number_of_pressure_sensors;
  //
  //   // Volume (m^3)
  //   double V = NUMBER_OF_TANKS_IN_USE * 350e-3;
  //   // Universal gas constant
  //   double R = 8.314472;
  //   // Molar mass of hydrogen (kg/mole)
  //   double M = 2.01588e-3;
  //
  //   // Compressibility factor
  //   double Z = 1;
  //   double a[] = {
  //     0.0588846, -0.06136111, -0.002650473, 0.002731125, 0.001802374, -0.001150707, 9.588528e-05, -1.10904e-07,
  //     1.264403e-10
  //   };
  //   double b[] = {1.325, 1.87, 2.5, 2.8, 2.938, 3.14, 3.37, 3.75, 4.0};
  //   double c[] = {1.0, 1.0, 2.0, 2.0, 2.42, 2.63, 3.0, 4.0, 5.0};
  //   // Lemmon equation to calculate Z
  //   for (int i = 0; i < 9; i++)
  //   {
  //     Z += a[i] * pow(100 / T, b[i]) * pow(p / 1e6, c[i]);
  //   }
  //
  //   pressure_status.h2_mass_estimation = database_bc_hdgn_chub_sens_1_status_pres_h2_mass_estimation_encode(
  //     M * p * V / (R * T * Z));
  // }

  void update_sensor_values()
  {
    update_sensor_value(&sensors.cooling_temp_sensor);
    update_sensor_value(&sensors.cooling_flow_sensor);
    update_sensor_value(&sensors.liquid_tank_level_sensor);
    update_sensor_value(&sensors.liquid_tank_pressure_sensor_1);
    update_sensor_value(&sensors.liquid_tank_pressure_sensor_2);
    update_sensor_value(&sensors.liquid_tank_temperature_sensor);
    update_sensor_value(&sensors.leak_sensor);


  }

  void send_sensor_values()
  {
    uint8_t data[8];

    // lh2_status.liquid_tank_temp = database_bc_hdgn_chub_sens_lh2_status_liquid_tank_temp_encode(
    //   sensors.liquid_tank_temp_sensor.value);

    HAL_GPIO_TogglePin(LED_DEBUG_1_GPIO_Port, LED_DEBUG_1_Pin);

    lh2_status.liquid_tank_pres_1 = database_bc_hdgn_chub_sens_lh2_status_liquid_tank_pres_1_encode(
      sensors.liquid_tank_pressure_sensor_1.value);
    lh2_status.liquid_tank_pres_2 = database_bc_hdgn_chub_sens_lh2_status_liquid_tank_pres_2_encode(
      sensors.liquid_tank_pressure_sensor_2.value);
    lh2_status.level_sensor = database_bc_hdgn_chub_sens_lh2_status_level_sensor_encode(
     sensors.liquid_tank_level_sensor.value);
    lh2_status.liquid_tank_temp = database_bc_hdgn_chub_sens_lh2_status_liquid_tank_temp_encode(
      sensors.liquid_tank_temperature_sensor.value);

  database_bc_hdgn_chub_sens_lh2_status_pack(data, &lh2_status, DATABASE_BC_HDGN_CHUB_SENS_LH2_STATUS_LENGTH);
  
  queue_CAN_message(&hcan1, DATABASE_BC_HDGN_CHUB_SENS_LH2_STATUS_FRAME_ID, DATABASE_BC_HDGN_CHUB_SENS_LH2_STATUS_LENGTH, data);



  
  cryo_cool_status.flow_cryo_coolant_loop = database_bc_cryo_chub_sens_h2_status_cool_flow_cryo_coolant_loop_encode(
    sensors.cooling_flow_sensor.value);
  cryo_cool_status.temp_cryo_coolant_loop = database_bc_cryo_chub_sens_h2_status_cool_temp_cryo_coolant_loop_encode(
    sensors.cooling_temp_sensor.value);

  database_bc_cryo_chub_sens_h2_status_cool_pack(data, &cryo_cool_status, DATABASE_BC_CRYO_CHUB_SENS_H2_STATUS_COOL_LENGTH);

  queue_CAN_message(&hcan1, DATABASE_BC_CRYO_CHUB_SENS_H2_STATUS_COOL_FRAME_ID, DATABASE_BC_CRYO_CHUB_SENS_H2_STATUS_COOL_LENGTH, data);

  
  h2_analogue_vi_bc.hdgn_analogue_1_vi = database_bc_hdgn_analogue_vi_hdgn_analogue_1_vi_encode(sensors.leak_sensor.value);
  database_bc_hdgn_analogue_vi_pack(data, &h2_analogue_vi_bc, DATABASE_BC_HDGN_ANALOGUE_VI_LENGTH);

  queue_CAN_message(&hcan1, DATABASE_BC_HDGN_ANALOGUE_VI_FRAME_ID, DATABASE_BC_HDGN_ANALOGUE_VI_LENGTH, data);
  }

  static void init_sensor(struct sensor_t* s, enum sensor_type type, uint32_t adc_channel)
  {
    s->type = type;
    s->adc_channel = adc_channel;
    s->value = 0;
  }
