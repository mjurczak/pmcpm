/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_bas.h"
#include "ble_cscs.h"
#include "ble_dis.h"
#include "ble_conn_params.h"
#include "sensorsim.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "device_manager.h"
#include "pstorage.h"
#include "app_trace.h"
#include "bsp.h"
#include "bsp_btn_ble.h"

#include "mpu6050.h"

#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                          /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/

#define DEVICE_NAME                     "TEST kadencji"                            /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME               "Maciej and Maciej"                        /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                40                                         /**< The advertising interval (in units of 0.625 ms. This value corresponds to 25 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS      180                                        /**< The advertising timeout in units of seconds. */

#define APP_TIMER_PRESCALER             0                                          /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE         4                                          /**< Size of timer operation queues. */

#define BATTERY_LEVEL_MEAS_INTERVAL     APP_TIMER_TICKS(2000, APP_TIMER_PRESCALER) /**< Battery level measurement interval (ticks). */
#define MIN_BATTERY_LEVEL               81                                         /**< Minimum battery level as returned by the simulated measurement function. */
#define MAX_BATTERY_LEVEL               100                                        /**< Maximum battery level as returned by the simulated measurement function. */
#define BATTERY_LEVEL_INCREMENT         1                                          /**< Value by which the battery level is incremented/decremented for each call to the simulated measurement function. */

#define SPEED_AND_CADENCE_MEAS_INTERVAL 1000                                       /**< Speed and cadence measurement interval (milliseconds). */



#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(500, UNIT_1_25_MS)           /**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(1000, UNIT_1_25_MS)          /**< Maximum acceptable connection interval (1 second). */
#define SLAVE_LATENCY                   0                                          /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)            /**< Connection supervisory timeout (4 seconds). */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)/**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                          /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                  1                                          /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                          /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                       /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                          /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                          /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                         /**< Maximum encryption key size. */

#define DEAD_BEEF                       0xDEADBEEF                                 /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define APP_FEATURE_NOT_SUPPORTED       BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2       /**< Reply when unsupported features are requested. */

static uint16_t                         m_conn_handle = BLE_CONN_HANDLE_INVALID;   /**< Handle of the current connection. */
static ble_bas_t                        m_bas;                                     /**< Structure used to identify the battery service. */
static ble_cscs_t                       m_cscs;                                    /**< Structure used to identify the cycling speed and cadence service. */

static sensorsim_cfg_t                  m_battery_sim_cfg;                         /**< Battery Level sensor simulator configuration. */
static sensorsim_state_t                m_battery_sim_state;                       /**< Battery Level sensor simulator state. */


APP_TIMER_DEF(m_battery_timer_id);                                                 /**< Battery timer. */
APP_TIMER_DEF(m_gyro_timer_id);                                                    /**< Gyroscope measurement timer. */

static dm_application_instance_t        m_app_handle;                              /**< Application identifier allocated by device manager. */
static bool                             m_auto_calibration_in_progress;            /**< Set when an autocalibration is in progress. */

static uint16_t 													gyro_crank_rev = 0;
static uint32_t 													gyro_crank_deg_dec = 0;
static uint16_t                           gyro_crank_avgRate = 0;

static ble_sensor_location_t supported_locations[] = {BLE_SENSOR_LOCATION_FRONT_WHEEL ,
                                                      BLE_SENSOR_LOCATION_LEFT_CRANK  ,
                                                      BLE_SENSOR_LOCATION_RIGHT_CRANK ,
                                                      BLE_SENSOR_LOCATION_LEFT_PEDAL  ,
                                                      BLE_SENSOR_LOCATION_RIGHT_PEDAL ,
                                                      BLE_SENSOR_LOCATION_FRONT_HUB   ,
                                                      BLE_SENSOR_LOCATION_REAR_DROPOUT,
                                                      BLE_SENSOR_LOCATION_CHAINSTAY   ,
                                                      BLE_SENSOR_LOCATION_REAR_WHEEL  ,
                                                      BLE_SENSOR_LOCATION_REAR_HUB};          /**< supported location for the sensor location. */

static ble_uuid_t m_adv_uuids[] = {{BLE_UUID_CYCLING_SPEED_AND_CADENCE,  BLE_UUID_TYPE_BLE},
                                   {BLE_UUID_BATTERY_SERVICE,            BLE_UUID_TYPE_BLE},
                                   {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}}; /**< Universally unique service identifiers. */

																	 
static void csc_meas_timeout_handler(void * p_context);
																	 
/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for performing battery measurement and updating the Battery Level characteristic
 *        in Battery Service.
 */
static void battery_level_update(void)
{
    uint32_t err_code;
    uint8_t  battery_level;

    battery_level = (uint8_t)sensorsim_measure(&m_battery_sim_state, &m_battery_sim_cfg);

    err_code = ble_bas_battery_level_update(&m_bas, battery_level);
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
        )
    {
        APP_ERROR_HANDLER(err_code);
    }
}


/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void battery_level_meas_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    battery_level_update();
}

static void gyro_meas_timeout_handler(void * p_context)
{
	
	static uint16_t previousCrankRev = 0;
	int16_t measuredRate;
	uint16_t absoluteRate;
	static uint8_t avgRateSample = 0;
	static uint32_t sampleSum = 0;
	uint16_t sampleAvg;
	
  nrf_gpio_pin_set(22);

	measuredRate = mpu6050_read_gyro_z();
	
  if(measuredRate > 5)
	{
		nrf_gpio_pin_set(21);
		absoluteRate = measuredRate;
	}
	else
	{
		nrf_gpio_pin_clear(21);
		absoluteRate = 0;
	}
	
	sampleSum += absoluteRate;
	avgRateSample++;
	
	gyro_crank_deg_dec += absoluteRate;
	// Convert cumulative 0.1 degrees to revolutions
	gyro_crank_rev = gyro_crank_deg_dec / 3600;
	
	
	if ( avgRateSample == 10)
	{
		sampleAvg = sampleSum / avgRateSample;
		gyro_crank_avgRate = sampleAvg;
		csc_meas_timeout_handler(NULL);
		avgRateSample = 0;
		sampleSum = 0;
	}

	previousCrankRev = gyro_crank_rev;
	
	nrf_gpio_pin_clear(22);
}


/**@brief Function for populating simulated cycling speed and cadence measurements.
 */
static void csc_sim_measurement(ble_cscs_meas_t * p_measurement)
{
	  static uint16_t fakeRevs = 0;
		uint32_t current_ticks;
	  uint16_t event_time;
	  static uint16_t lastEventTime = 0;
	
	  // Get current ticks
		//app_timer_cnt_get(&current_ticks);
	
    //APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER);
	  
	  // Per specification event time is in 1/1024th's of a second.
	  //event_time = (current_ticks * 1024) / APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER);

    // Indicate no wheel data present.
    p_measurement->is_wheel_rev_data_present = false;

    // Calculate cadence value from average rate
    p_measurement->is_crank_rev_data_present = true;

	  if ( gyro_crank_avgRate > 0)
		{
	    event_time = lastEventTime + ((1024*360) / gyro_crank_avgRate);
			fakeRevs++;
	  }
		else
		{
			event_time = lastEventTime + 1024;
		}
			
    p_measurement->cumulative_crank_revs = fakeRevs;
    p_measurement->last_crank_event_time = event_time;
		
		lastEventTime = event_time;
}


/**@brief Function for handling the Cycling Speed and Cadence measurement timer timeouts.
 *
 * @details This function will be called each time the cycling speed and cadence
 *          measurement timer expires.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */
static void csc_meas_timeout_handler(void * p_context)
{
    uint32_t        err_code;
    ble_cscs_meas_t cscs_measurement;
		uint16_t        avgRate;
	
    UNUSED_PARAMETER(p_context);
	
		nrf_gpio_pin_set(20);
	
    csc_sim_measurement(&cscs_measurement);

    err_code = ble_cscs_measurement_send(&m_cscs, &cscs_measurement);
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
        )
    {
        APP_ERROR_HANDLER(err_code);
    }
    if (m_auto_calibration_in_progress)
    {
        err_code = ble_sc_ctrlpt_rsp_send(&(m_cscs.ctrl_pt), BLE_SCPT_SUCCESS);
        if ((err_code != NRF_SUCCESS) &&
            (err_code != NRF_ERROR_INVALID_STATE) &&
            (err_code != BLE_ERROR_NO_TX_BUFFERS)
            )
        {
            APP_ERROR_HANDLER(err_code);
        }
        if (err_code != BLE_ERROR_NO_TX_BUFFERS)
        {
            m_auto_calibration_in_progress = false;
        }
    }
		
		nrf_gpio_pin_clear(20);
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    uint32_t err_code;

    // Initialize timer module.
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, false);

    // Create timers.
    err_code = app_timer_create(&m_battery_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                battery_level_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);

	  // Create gyro measurement timer.
    err_code = app_timer_create(&m_gyro_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                gyro_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);
	
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_CYCLING_SPEED_CADENCE_SENSOR);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Speed and Cadence Control point events
 *
 * @details Function for handling Speed and Cadence Control point events.
 *          This function parses the event and in case the "set cumulative value" event is received,
 *          sets the wheel cumulative value to the received value.
 */
ble_scpt_response_t sc_ctrlpt_event_handler(ble_sc_ctrlpt_t     * p_sc_ctrlpt,
                                            ble_sc_ctrlpt_evt_t * p_evt)
{
    switch (p_evt->evt_type)
    {
        case BLE_SC_CTRLPT_EVT_START_CALIBRATION:
            m_auto_calibration_in_progress = true;
            break;

        default:
            // No implementation needed.
            break;
    }
    return (BLE_SCPT_SUCCESS);
}


/**@brief Function for initializing services that will be used by the application.
 *
 * @details Initialize the Cycling Speed and Cadence, Battery and Device Information services.
 */
static void services_init(void)
{
    uint32_t              err_code;
    ble_cscs_init_t       cscs_init;
    ble_bas_init_t        bas_init;
    ble_dis_init_t        dis_init;
    ble_sensor_location_t sensor_location;

    // Initialize Cycling Speed and Cadence Service.
    memset(&cscs_init, 0, sizeof(cscs_init));

    cscs_init.evt_handler = NULL;
    cscs_init.feature     = BLE_CSCS_FEATURE_CRANK_REV_BIT |
                            BLE_CSCS_FEATURE_MULTIPLE_SENSORS_BIT;

    // Here the sec level for the Cycling Speed and Cadence Service can be changed/increased.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cscs_init.csc_meas_attr_md.cccd_write_perm);   // for the measurement characteristic, only the CCCD write permission can be set by the application, others are mandated by service specification
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cscs_init.csc_feature_attr_md.read_perm);      // for the feature characteristic, only the read permission can be set by the application, others are mandated by service specification
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cscs_init.csc_ctrlpt_attr_md.write_perm);      // for the SC control point characteristic, only the write permission and CCCD write can be set by the application, others are mandated by service specification
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cscs_init.csc_ctrlpt_attr_md.cccd_write_perm); // for the SC control point characteristic, only the write permission and CCCD write can be set by the application, others are mandated by service specification
    
    cscs_init.ctrplt_supported_functions    = BLE_SRV_SC_CTRLPT_CUM_VAL_OP_SUPPORTED
                                              |BLE_SRV_SC_CTRLPT_SENSOR_LOCATIONS_OP_SUPPORTED
                                              |BLE_SRV_SC_CTRLPT_START_CALIB_OP_SUPPORTED;
    cscs_init.ctrlpt_evt_handler            = sc_ctrlpt_event_handler;
    cscs_init.list_supported_locations      = supported_locations;
    cscs_init.size_list_supported_locations = sizeof(supported_locations) / sizeof(ble_sensor_location_t);            
    
    sensor_location           = BLE_SENSOR_LOCATION_FRONT_WHEEL;                    // initializes the sensor location to add the sensor location characteristic.
    cscs_init.sensor_location = &sensor_location;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cscs_init.csc_sensor_loc_attr_md.read_perm);    // for the sensor location characteristic, only the read permission can be set by the application, others are mendated by service specification

    err_code = ble_cscs_init(&m_cscs, &cscs_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Battery Service.
    memset(&bas_init, 0, sizeof(bas_init));

    // Here the sec level for the Battery Service can be changed/increased.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&bas_init.battery_level_char_attr_md.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_report_read_perm);

    bas_init.evt_handler          = NULL;
    bas_init.support_notification = true;
    bas_init.p_report_ref         = NULL;
    bas_init.initial_batt_level   = 100;

    err_code = ble_bas_init(&m_bas, &bas_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Device Information Service.
    memset(&dis_init, 0, sizeof(dis_init));

    ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, MANUFACTURER_NAME);

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&dis_init.dis_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init.dis_attr_md.write_perm);

    err_code = ble_dis_init(&dis_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting application timers.
 */
static void application_timers_start(void)
{
    uint32_t err_code;
    uint32_t gyro_meas_timer_ticks;

    // Start application timers.
    err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);

	  gyro_meas_timer_ticks = APP_TIMER_TICKS(100, APP_TIMER_PRESCALER);

    err_code = app_timer_start(m_gyro_timer_id, gyro_meas_timer_ticks, NULL);
    APP_ERROR_CHECK(err_code);	
}



/**@brief Function for handling the Connection Parameter events.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail configuration parameter, but instead we use the
 *                event handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t connection_params_init;

    memset(&connection_params_init, 0, sizeof(connection_params_init));

    connection_params_init.p_conn_params                  = NULL;
    connection_params_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    connection_params_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    connection_params_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    connection_params_init.start_on_notify_cccd_handle    = m_cscs.meas_handles.cccd_handle;
    connection_params_init.disconnect_on_fail             = false;
    connection_params_init.evt_handler                    = on_conn_params_evt;
    connection_params_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&connection_params_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;
        default:
            break;
    }
}


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t                              err_code = NRF_SUCCESS;
    ble_gatts_rw_authorize_reply_params_t auth_reply;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        case BLE_EVT_USER_MEM_REQUEST:
            err_code = sd_ble_user_mem_reply(m_conn_handle, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
            if(p_ble_evt->evt.gatts_evt.params.authorize_request.type
               != BLE_GATTS_AUTHORIZE_TYPE_INVALID)
            {
                if ((p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.op
                     == BLE_GATTS_OP_PREP_WRITE_REQ)
                    || (p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.op
                     == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW)
                    || (p_ble_evt->evt.gatts_evt.params.authorize_request.request.write.op
                     == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL))
                {
                    if (p_ble_evt->evt.gatts_evt.params.authorize_request.type
                        == BLE_GATTS_AUTHORIZE_TYPE_WRITE)
                    {
                    auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE;
                    }
                    else
                    {
                        auth_reply.type = BLE_GATTS_AUTHORIZE_TYPE_READ;
                    }
                    auth_reply.params.write.gatt_status = APP_FEATURE_NOT_SUPPORTED;
                    err_code = sd_ble_gatts_rw_authorize_reply(m_conn_handle,&auth_reply);
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        case BLE_GATTS_OP_EXEC_WRITE_REQ_NOW:
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the BLE Stack event interrupt handler after a BLE stack
 *          event has been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    dm_ble_evt_handler(p_ble_evt);
    ble_cscs_on_ble_evt(&m_cscs, p_ble_evt);
    ble_bas_on_ble_evt(&m_bas, p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
    bsp_btn_ble_on_ble_evt(p_ble_evt);
    on_ble_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in] sys_evt  System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    pstorage_sys_event_handler(sys_evt);
    ble_advertising_on_sys_evt(sys_evt);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);

    // Enable BLE stack.
    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
#if (defined(S130) || defined(S132))
    ble_enable_params.gatts_enable_params.attr_tab_size   = BLE_GATTS_ATTR_TAB_SIZE_DEFAULT;
#endif
    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}



/**@brief Function for handling the Device Manager events.
 *
 * @param[in] p_evt  Data associated to the device manager event.
 */
static uint32_t device_manager_evt_handler(dm_handle_t const * p_handle,
                                           dm_event_t const  * p_event,
                                           ret_code_t        event_result)
{
    APP_ERROR_CHECK(event_result);
    return NRF_SUCCESS;
}


/**@brief Function for the Device Manager initialization.
 *
 * @param[in] erase_bonds  Indicates whether bonding information should be cleared from
 *                         persistent storage during initialization of the Device Manager.
 */
static void device_manager_init(bool erase_bonds)
{
    uint32_t               err_code;
    dm_init_param_t        init_param = {.clear_persistent_data = erase_bonds};
    dm_application_param_t register_param;

    // Initialize persistent storage module.
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);

    err_code = dm_init(&init_param);
    APP_ERROR_CHECK(err_code);

    memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));
    
    register_param.sec_param.bond         = SEC_PARAM_BOND;
    register_param.sec_param.mitm         = SEC_PARAM_MITM;
    register_param.sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    register_param.sec_param.oob          = SEC_PARAM_OOB;
    register_param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    register_param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    register_param.evt_handler            = device_manager_evt_handler;
    register_param.service_type           = DM_PROTOCOL_CNTXT_GATT_SRVR_ID;

    err_code = dm_register(&m_app_handle, &register_param);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;

    // Build advertising data struct to pass into @ref ble_advertising_init.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = m_adv_uuids;

    ble_adv_modes_config_t options = {0};
    options.ble_adv_fast_enabled  = BLE_ADV_FAST_ENABLED;
    options.ble_adv_fast_interval = APP_ADV_INTERVAL;
    options.ble_adv_fast_timeout  = APP_ADV_TIMEOUT_IN_SECONDS;

    err_code = ble_advertising_init(&advdata, NULL, &options, on_adv_evt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

 
static void leds_init(void)
{
	nrf_gpio_cfg_output(22);
	nrf_gpio_cfg_output(21);
	nrf_gpio_cfg_output(20);
	nrf_gpio_cfg_output(19);
}

/**@brief Function for application main entry.
 */
int main(void)
{
    uint32_t err_code;
    bool erase_bonds;

    // Initialize.
    app_trace_init();
    timers_init();
	  leds_init();
	  nrf_gpio_pin_set(19);
    //buttons_leds_init(&erase_bonds);
    ble_stack_init();
    device_manager_init(erase_bonds);
    gap_params_init();
    advertising_init();
    services_init();
	 
	  mpu6050_init();

    conn_params_init();

    // Start execution.
    application_timers_start();
    err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
	  nrf_gpio_pin_clear(19);

    // Enter main loop.
    for (;; )
    {
      power_manage();
    }
}


/**
 * @}
 */
