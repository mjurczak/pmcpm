/* This file was generated by plugin 'Nordic Semiconductor nRF5x v.1.2.4' (BDS version 1.1.3139.0) */

#include "service_if.h"
#include <stdint.h>
#include "app_trace.h" 
#include "ble_cps.h" 

static ble_cps_t    m_cps; 

uint8_t m_cps_cycling_power_measurement_initial_value_extreme_angles___maximum_angle_arr[1]; 
uint8_t m_cps_cycling_power_measurement_initial_value_extreme_angles___minimum_angle_arr[1]; 
uint8_t m_cps_cycling_power_control_point_initial_value_parameter_value_arr[1]; 
uint8_t m_cps_cycling_power_control_point_initial_value_response_parameter_arr[1]; 


/**@brief Function for handling the Cycling Power events.
 *
 * @details This function will be called for all Cycling Power events which are passed to
 *          the application.
 *
 * @param[in]   p_cycling_power   Cycling Power structure.
 * @param[in]   p_evt   Event received from the Cycling Power.
 */
static void on_cps_evt(ble_cps_t * p_cps, ble_cps_evt_t * p_evt)
{
    switch (p_evt->evt_type)
    { 
        case BLE_CPS_CYCLING_POWER_MEASUREMENT_EVT_NOTIFICATION_ENABLED:
            app_trace_log("[Bluetooth_IF]: CPS_CYCLING_POWER_MEASUREMENT evt NOTIFICATION_ENABLED. \r\n");
            break;
        case BLE_CPS_CYCLING_POWER_MEASUREMENT_EVT_NOTIFICATION_DISABLED:
            app_trace_log("[Bluetooth_IF]: CPS_CYCLING_POWER_MEASUREMENT evt NOTIFICATION_DISABLED. \r\n");
            break;
        case BLE_CPS_CYCLING_POWER_MEASUREMENT_EVT_CCCD_WRITE:
            app_trace_log("[Bluetooth_IF]: CPS_CYCLING_POWER_MEASUREMENT evt CCCD_WRITE. \r\n");
            break; 
        case BLE_CPS_CYCLING_POWER_VECTOR_EVT_NOTIFICATION_ENABLED:
            app_trace_log("[Bluetooth_IF]: CPS_CYCLING_POWER_VECTOR evt NOTIFICATION_ENABLED. \r\n");
            break;
        case BLE_CPS_CYCLING_POWER_VECTOR_EVT_NOTIFICATION_DISABLED:
            app_trace_log("[Bluetooth_IF]: CPS_CYCLING_POWER_VECTOR evt NOTIFICATION_DISABLED. \r\n");
            break;
        case BLE_CPS_CYCLING_POWER_VECTOR_EVT_CCCD_WRITE:
            app_trace_log("[Bluetooth_IF]: CPS_CYCLING_POWER_VECTOR evt CCCD_WRITE. \r\n");
            break; 
        case BLE_CPS_CYCLING_POWER_CONTROL_POINT_EVT_NOTIFICATION_ENABLED:
            app_trace_log("[Bluetooth_IF]: CPS_CYCLING_POWER_CONTROL_POINT evt NOTIFICATION_ENABLED. \r\n");
            break;
        case BLE_CPS_CYCLING_POWER_CONTROL_POINT_EVT_NOTIFICATION_DISABLED:
            app_trace_log("[Bluetooth_IF]: CPS_CYCLING_POWER_CONTROL_POINT evt NOTIFICATION_DISABLED. \r\n");
            break;
        case BLE_CPS_CYCLING_POWER_CONTROL_POINT_EVT_CCCD_WRITE:
            app_trace_log("[Bluetooth_IF]: CPS_CYCLING_POWER_CONTROL_POINT evt CCCD_WRITE. \r\n");
            break; 
        case BLE_CPS_CYCLING_POWER_CONTROL_POINT_EVT_WRITE:
            app_trace_log("[Bluetooth_IF]: CPS_CYCLING_POWER_CONTROL_POINT evt WRITE. \r\n");
            break; 
        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for initializing the Services generated by Bluetooth Developer Studio.
 *
 *
 * @return      NRF_SUCCESS on successful initialization of services, otherwise an error code.
 */
uint32_t bluetooth_init(void)
{
    uint32_t    err_code; 
    ble_cps_init_t    cps_init; 
    

    // Initialize Cycling Power.
    memset(&cps_init, 0, sizeof(cps_init));

    cps_init.evt_handler = on_cps_evt; 
    memset(&cps_init.ble_cps_cycling_power_measurement_initial_value.instantaneous_power,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_measurement_initial_value.instantaneous_power));
    memset(&cps_init.ble_cps_cycling_power_measurement_initial_value.pedal_power_balance,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_measurement_initial_value.pedal_power_balance));
    memset(&cps_init.ble_cps_cycling_power_measurement_initial_value.accumulated_torque,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_measurement_initial_value.accumulated_torque));
    memset(&cps_init.ble_cps_cycling_power_measurement_initial_value.wheel_revolution_data___cumulative_wheel_revolutions,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_measurement_initial_value.wheel_revolution_data___cumulative_wheel_revolutions));
    memset(&cps_init.ble_cps_cycling_power_measurement_initial_value.wheel_revolution_data___last_wheel_event_time,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_measurement_initial_value.wheel_revolution_data___last_wheel_event_time));
    memset(&cps_init.ble_cps_cycling_power_measurement_initial_value.crank_revolution_data__cumulative_crank_revolutions,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_measurement_initial_value.crank_revolution_data__cumulative_crank_revolutions));
    memset(&cps_init.ble_cps_cycling_power_measurement_initial_value.crank_revolution_data__last_crank_event_time,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_measurement_initial_value.crank_revolution_data__last_crank_event_time));
    memset(&cps_init.ble_cps_cycling_power_measurement_initial_value.extreme_force_magnitudes___maximum_force_magnitude,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_measurement_initial_value.extreme_force_magnitudes___maximum_force_magnitude));
    memset(&cps_init.ble_cps_cycling_power_measurement_initial_value.extreme_force_magnitudes___minimum_force_magnitude,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_measurement_initial_value.extreme_force_magnitudes___minimum_force_magnitude));
    memset(&cps_init.ble_cps_cycling_power_measurement_initial_value.extreme_torque_magnitudes__maximum_torque_magnitude,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_measurement_initial_value.extreme_torque_magnitudes__maximum_torque_magnitude));
    memset(&cps_init.ble_cps_cycling_power_measurement_initial_value.extreme_torque_magnitudes__minimum_torque_magnitude,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_measurement_initial_value.extreme_torque_magnitudes__minimum_torque_magnitude));
    cps_init.ble_cps_cycling_power_measurement_initial_value.extreme_angles___maximum_angle.size = 1;
    cps_init.ble_cps_cycling_power_measurement_initial_value.extreme_angles___maximum_angle.p_data = m_cps_cycling_power_measurement_initial_value_extreme_angles___maximum_angle_arr; 
    cps_init.ble_cps_cycling_power_measurement_initial_value.extreme_angles___minimum_angle.size = 1;
    cps_init.ble_cps_cycling_power_measurement_initial_value.extreme_angles___minimum_angle.p_data = m_cps_cycling_power_measurement_initial_value_extreme_angles___minimum_angle_arr; 
    memset(&cps_init.ble_cps_cycling_power_measurement_initial_value.top_dead_spot_angle,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_measurement_initial_value.top_dead_spot_angle));
    memset(&cps_init.ble_cps_cycling_power_measurement_initial_value.bottom_dead_spot_angle,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_measurement_initial_value.bottom_dead_spot_angle));
    memset(&cps_init.ble_cps_cycling_power_measurement_initial_value.accumulated_energy,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_measurement_initial_value.accumulated_energy));
    cps_init.ble_cps_sensor_location_initial_value.sensor_location.sensor_location = SENSOR_LOCATION_OTHER; 
    memset(&cps_init.ble_cps_cycling_power_vector_initial_value.crank_revolution_data___cumulative_crank_revolutions,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_vector_initial_value.crank_revolution_data___cumulative_crank_revolutions));
    memset(&cps_init.ble_cps_cycling_power_vector_initial_value.crank_revolution_data___last_crank_event_time,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_vector_initial_value.crank_revolution_data___last_crank_event_time));
    memset(&cps_init.ble_cps_cycling_power_vector_initial_value.first_crank_measurement_angle_,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_vector_initial_value.first_crank_measurement_angle_));
    memset(&cps_init.ble_cps_cycling_power_vector_initial_value.instantaneous_force_magnitude_array,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_vector_initial_value.instantaneous_force_magnitude_array));
    memset(&cps_init.ble_cps_cycling_power_vector_initial_value.instantaneous_torque_magnitude_array,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_vector_initial_value.instantaneous_torque_magnitude_array));
    cps_init.ble_cps_cycling_power_control_point_initial_value.op_codes.op_codes = OP_CODES_SET_CUMULATIVE_VALUE; 
    cps_init.ble_cps_cycling_power_control_point_initial_value.parameter_value.size = 1;
    cps_init.ble_cps_cycling_power_control_point_initial_value.parameter_value.p_data = m_cps_cycling_power_control_point_initial_value_parameter_value_arr; 
    memset(&cps_init.ble_cps_cycling_power_control_point_initial_value.request_op_code,
           0x00,
           sizeof(cps_init.ble_cps_cycling_power_control_point_initial_value.request_op_code));
    cps_init.ble_cps_cycling_power_control_point_initial_value.response_value.response_value = RESPONSE_VALUE_SUCCESS; 
    cps_init.ble_cps_cycling_power_control_point_initial_value.response_parameter.size = 1;
    cps_init.ble_cps_cycling_power_control_point_initial_value.response_parameter.p_data = m_cps_cycling_power_control_point_initial_value_response_parameter_arr; 

    err_code = ble_cps_init(&m_cps, &cps_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    } 

    return NRF_SUCCESS;
}

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to all Bluetooth Developer Studio generated Services.
 *
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 */
void bluetooth_on_ble_evt(ble_evt_t * p_ble_evt)
{ 
    ble_cps_on_ble_evt(&m_cps, p_ble_evt); 
}