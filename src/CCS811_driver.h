#ifndef CCS811_DRIVER_H
#define CCS811_DRIVER_H

#include <Wire.h>
#include <stdint.h>

const uint8_t CCS811_HARDWARE_ID = 0x81;
const uint8_t CCS811_DEFAULT_I2C_ADDRESS = 0x5A;

///////////////////////////////////////////////////////////////////////////////
// STATUS

enum CCS811_FIRMWARE_MODE {
    CCS811_BOOT_MODE = 0,
    CCS811_APPLICATION_MODE = 1,
};

typedef union {
    uint8_t raw;
    struct {
        bool error_has_occurred : 1;  // Error on I2C or sensor. ERROR_ID contains the source of the error
        uint8_t _reserved : 2;
        bool data_ready : 1;                     // True if a new data sample is ready in ALG_RESULT_DATA.
        bool application_firmware_loaded : 1;    // True if application firmware was loaded
        bool application_firmware_verified : 1;  // Boot mode only. True if verify operation completed successfully
        bool application_firmware_erase_completed : 1;  // Boot mode only. True if erase completed successfully
        bool firmware_is_in_application_mode : 1;  // True if firmware is in application mode and ready for measurement
    };
} ccs811_status_t;

///////////////////////////////////////////////////////////////////////////////
// MEAS_MODE

enum CCS811_DRIVE_MODE {
    CCS811_IDLE_MODE = 0,            // Measurements disabled
    CCS811_CONSTANT_POWER_1SEC = 1,  // Measurements every second
    CCS811_PULSED_10SEC = 2,         // Measurements every 10 seconds
    CCS811_PULSED_60SEC = 3,         // Measurements every 60 seconds
    /*
    Measurements every 250 milliseconds.
    Data in the ALG_RESULT_DATA register is not updated.
    Raw readings are updated in the RAW_DATA register and must be processed
    by the host system.
    */
    CCS811_CONSTANT_POWER_250MS = 4
};

typedef union {
    uint8_t raw;
    struct {
        uint8_t _reserved0 : 2;
        /*
        True to enable interrupts when a measurement exceeds the set threshold
        and hysteresis set in the threshold configuration register.
        */
        bool interrupt_on_threshold_only_enabled : 1;
        bool interrupt_on_data_ready_enabled : 1;  // True to enable data ready interrupts. Active low.
        /*
        Measurement conversion mode. New samples are placed in the ALG_RESULT_DATA and RAW_DATA registers.
        New samples set the data_ready flag in the status register.
        */
        uint8_t drive_mode : 3;
        uint8_t _reserved1 : 1;
    };

} ccs811_measure_config_t;

///////////////////////////////////////////////////////////////////////////////
// ALG_RESULT_DATA

typedef union {
    uint8_t raw[2];
    int16_t total;  // Equivalent CO2 in parts per million
} ccs811_eCO2_data_t;

typedef union {
    uint8_t raw[2];
    int16_t total;  // Equivalent total volatile organic compounds in parts per billion
} ccs811_eTVOC_data_t;

typedef union {
    uint8_t raw[4];
    struct {
        ccs811_eTVOC_data_t eTVOC_reading;
        ccs811_eCO2_data_t eCO2_reading;
    };
} ccs811_air_quality_data_t;

///////////////////////////////////////////////////////////////////////////////
// RAW_DATA

typedef union {
    uint8_t raw[2];
    struct {
        uint16_t adc_reading : 10;  // Raw ADC reading of the sensor at the indicated current (1024 = 1.65V)
        uint8_t current_uA : 6;     // Current through the sensor in uA
    };
} ccs811_raw_data_t;

///////////////////////////////////////////////////////////////////////////////
// ENV_DATA

typedef union {
    uint8_t raw[2];
    uint16_t total;  // Humidity data for use in conversions (1/512%RH)
} ccs811_humidity_t;

typedef union {
    uint8_t raw[2];
    uint16_t total;  // Temperature data for use in conversions (1/512°C - 25)

} ccs811_temperature_t;

typedef union {
    uint8_t raw[4];
    struct {
        ccs811_humidity_t humidity;        // Humidity data for use in conversions (1/512%RH)
        ccs811_temperature_t temperature;  // Temperature data for use in conversions (1/512°C - 25)
    };
} ccs811_environmental_data_t;

///////////////////////////////////////////////////////////////////////////////
// THRESHOLDS

/**
 * An interrupt is asserted if the eCO2 value moves from the current zone (low, medium, or high) into another zone by
 * more than 50 ppm.
 */
typedef union {
    uint8_t raw[4];
    struct {
        uint16_t low_limit;   // Low to medium threshold of CO2 measurements
        uint16_t high_limit;  // Medium to high threshold of CO2 measurements
    };
} ccs811_co2_thresholds_t;

///////////////////////////////////////////////////////////////////////////////
// BASELINE

typedef union {
    uint8_t raw[2];
    uint16_t baseline;
} ccs811_baseline_t;  // Refer to AN000370:CCS811 for details.

///////////////////////////////////////////////////////////////////////////////
// HW_ID

typedef union {
    uint8_t raw;
} ccs811_hardware_id_t;

///////////////////////////////////////////////////////////////////////////////
// HW_VERSION

typedef union {
    uint8_t raw;
    struct {
        uint8_t major_version : 4;  // 1 for CCS811
        uint8_t build_variant : 4;  // Build variant of device
    };
} ccs811_hardware_version_t;

///////////////////////////////////////////////////////////////////////////////
// FW_Boot_Version

typedef union {
    uint8_t raw[2];
    struct {
        uint8_t minor : 4;  // Major firmware version
        uint8_t major : 4;  // Minor firmware version
        uint8_t trivial;    // Trivial firmware version
    };
} ccs811_firmware_boot_version_t;

typedef union {
    uint8_t raw[2];
    struct {
        uint8_t minor : 4;  // Major firmware version
        uint8_t major : 4;  // Minor firmware version
        uint8_t trivial;    // Trivial firmware version
    };
} ccs811_firmware_application_version_t;

///////////////////////////////////////////////////////////////////////////////
// ERROR_ID

typedef union {
    uint8_t raw;
    struct {
        bool write_register_invalid : 1;              // Device received a write command to an invalid register address
        bool read_register_invalid : 1;               // Device received a read request to an invalid register address
        bool measurement_mode_unsupported : 1;        // Device received a request to an unsupported measurement mode
        bool maximum_sensor_resistance_exceeded : 1;  // The sensor resistance measurement is out of range
        bool heater_current_not_in_range : 1;         // The device heater current is out of range
        bool heater_voltage_incorrectly_applied : 1;  // The heater voltage is not being applied correctly
        uint8_t _reserved : 2;
    };
} ccs811_error_t;

///////////////////////////////////////////////////////////////////////////////
// APP_ERASE

/**
 * The APP_ERASE can take a variable amount of time. The status
 * register can be polled to determine when this function is
 * complete. The 6th bit (0x40) is initialised to 0 and set to a 1 on
 * completion of the APP_ERASE function. After an erase this bit
 * is only cleared by doing a reset or starting the application.
 */
typedef uint8_t ccs811_application_erase_t[4];
const uint8_t CCS811_APPLICATION_ERASE_SEQUENCE[4] = {0xE7, 0xA7, 0xE6, 0x09};

///////////////////////////////////////////////////////////////////////////////
// APP_DATA

typedef union {
    uint8_t raw[9];
} ccs811_application_data_t;

///////////////////////////////////////////////////////////////////////////////
// APP_VERIFY

/**
 * Single byte write only register which starts the application verify process run by the bootloader to check for a
 * complete application code image. Command only needs to be called once after a firmware download as the result is
 * saved in a flash location that gets checked during device initialisation. The APP_VERIFY can take a variable amount
 * of time. The status register can be polled to determine when this function is complete. The 5th bit (0x20) is
 * initialised to 0 and set to a 1 on completion of the APP_VERIFY function. After an APP_VERIFY this bit is only
 * cleared by doing a reset or starting the application. For details on downloading new application firmware please
 * refer to application note ams AN000371.
 */
typedef union {
    uint8_t raw;
} ccs811_application_verify_t;

///////////////////////////////////////////////////////////////////////////////
// APP_START

/**
 * To change the mode of the CCS811 from Boot mode to running the application, a single byte write of 0xF4 is required.
 * The CCS811 interprets this as an address write to select the ‘APP_START’ register and starts running the loaded
 * application software if it is a valid version (Refer to the STATUS Register (0x00)).
 * */
typedef union {
    uint8_t raw;
} ccs811_application_start_t;

///////////////////////////////////////////////////////////////////////////////
// SW_RESET

typedef union {
    uint8_t raw[4];
} ccs811_reset_t;
const uint8_t CCS811_RESET_SEQUENCE[4] = {0x11, 0xE5, 0x72, 0x8A};

///////////////////////////////////////////////////////////////////////////////

typedef union {
    uint8_t raw[8];
    struct {
        ccs811_raw_data_t raw_data;             // Raw ADC reading of the sensor
        ccs811_error_t error;                   // Error source flags from last reading
        ccs811_status_t status;                 // Device status, including data ready flag
        ccs811_eTVOC_data_t eTVOC_ppm_reading;  // Equivalent total volatile organic compounds in parts per billion
        ccs811_eCO2_data_t eCO2_ppb_reading;    // Equivalent CO2 in parts per million
    };
} ccs811_all_data_t;

class CCS811 {
   public:
    bool begin(uint8_t device_address = CCS811_DEFAULT_I2C_ADDRESS);
    bool comms_check();

    bool read(ccs811_status_t&);
    bool read(ccs811_measure_config_t&);
    bool read(ccs811_co2_thresholds_t&);
    bool read(ccs811_eCO2_data_t&);
    bool read(ccs811_eTVOC_data_t&);
    bool read(ccs811_raw_data_t&);
    bool read(ccs811_air_quality_data_t&);
    bool read(ccs811_all_data_t&);
    bool read(ccs811_baseline_t&);
    bool read(ccs811_hardware_id_t&);
    bool read(ccs811_hardware_version_t&);
    bool read(ccs811_firmware_boot_version_t&);
    bool read(ccs811_firmware_application_version_t&);
    bool read(ccs811_error_t&);

    bool write(ccs811_measure_config_t);
    bool write(ccs811_environmental_data_t);
    bool write(ccs811_co2_thresholds_t);
    bool write(ccs811_baseline_t);
    bool write(ccs811_application_data_t);
    bool write(ccs811_application_verify_t);
    bool write(ccs811_application_start_t);

    bool write_environmental_data(float temperature = 25.0, float humidity = 50.0);
    bool write_co2_thresholds(uint16_t low_threshold, uint16_t high_threshold);
    uint16_t get_eCO2();
    uint16_t get_eTVOC();

    bool reset();
    bool start_application_erase();
    bool start_application_verify();
    bool start_application_mode();

   private:
    typedef enum {
        STATUS = 0x00,
        MEAS_MODE = 0x01,
        ALG_RESULT_DATA = 0x02,
        RAW_DATA = 0x03,
        ENV_DATA = 0x05,
        THRESHOLDS = 0x10,
        BASELINE = 0x11,
        HW_ID = 0x20,
        HW_VERSION = 0x21,
        FW_BOOT_VERSION = 0x23,
        FW_APP_VERSION = 0x24,
        INTERNAL_STATE = 0xA0,
        ERROR_ID = 0xE0,
        APP_ERASE = 0xF1,
        APP_DATA = 0xF2,
        APP_VERIFY = 0xF3,
        APP_START = 0xF4,
        SW_RESET = 0xFF
    } ccs811_reg_t;
    uint8_t _device_address;

    bool read(uint8_t* output, ccs811_reg_t address, uint8_t length = 1);
    bool write(uint8_t* input, ccs811_reg_t address, uint8_t length = 1);

    bool write(ccs811_application_erase_t);
    bool write(ccs811_reset_t);
};

void swap_endianess(uint8_t* buffer, size_t size);

#endif
