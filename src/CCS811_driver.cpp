#include "CCS811_driver.h"
#include <ArduinoLog.h>

////////////////////////////////////////////////////////////////////////////////

/**
 * Start the air quality sensor.
 * @return True if the device was started and is communicating successfully.
 */
bool CCS811::begin(uint8_t device_address) {
    _device_address = device_address;
    return comms_check();
}

/**
 * Check if the device is communicating properly.
 * The hardware ID is checked from the device's registers.
 * @return True if the hardware ID matches the expected value.
 */
bool CCS811::comms_check() {
    ccs811_hardware_id_t id;

    bool success = read(id);
    uint8_t retries = 0;
    while (not success and retries < 10) {
        success = read(id);
        retries++;
        delay(10);
        Log.trace(F("AQ - id retrieval failed; retrying (%d)\n"), retries);
    }

    return id.raw == CCS811_HARDWARE_ID;
}

/**
 * Write a value to a register using I2C
 *
 * @param input: Byte to write to the register
 * @param address: Address of register to write to
 * @param length: Number of bytes to be written
 * @return: Success/error result of the write.
 */
bool CCS811::write(uint8_t* input, ccs811_reg_t address, uint8_t length) {
    bool result = true;
    Wire.beginTransmission(_device_address);
    Wire.write(address);
    for (size_t i = 0; i < length; i++) {
        Wire.write(input[i]);
        Log.trace(F("AQ Sent [%X] >> %X\n"), input[i]);
    }

    if (Wire.endTransmission() != 0) {
        result = false;
    }
    return result;
}

/**
 * Read a specified number of bytes using the I2C bus.
 * @param output: The buffer in which to store the read values.
 * @param address: Register address to read (or starting address in burst reads)
 * @param length: Number of bytes to read.
 */
bool CCS811::read(uint8_t* output, ccs811_reg_t address, uint8_t length) {
    bool result = true;
    Wire.beginTransmission(_device_address);
    Wire.write(address);
    if (Wire.endTransmission() != 0)
        result = false;

    else  // OK, all worked, keep going
    {
        Wire.requestFrom(_device_address, length);
        for (size_t i = 0; (i < length) and Wire.available(); i++) {
            uint8_t c = Wire.read();
            output[i] = c;
            Log.trace(F("AQ Received [%X] >> %X\n"), output[i]);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * Read the status from the sensor.
 * @param status: Container to read status into.
 */
bool CCS811::read(ccs811_status_t& status) { return read(&status.raw, STATUS); }

/**
 * Read the measurement mode configuration from the sensor.
 * @param config: Container to read the configuration into.
 */
bool CCS811::read(ccs811_measure_config_t& config) { return read(&config.raw, MEAS_MODE); }

/**
 * Read thresholds from the device.
 * @param thresholds: Container to read thresholds into.
 */
bool CCS811::read(ccs811_co2_thresholds_t& thresholds) {
    bool success = read(thresholds.raw, THRESHOLDS, sizeof(thresholds));
    swap_endianess((uint8_t*)&thresholds.low_limit, sizeof(thresholds.low_limit));
    swap_endianess((uint8_t*)&thresholds.high_limit, sizeof(thresholds.high_limit));
    return success;
}

/**
 * Read the measured eCO2 level from the sensor.
 * @param data: Container to read data into.
 */
bool CCS811::read(ccs811_eCO2_data_t& data) { return read((uint8_t*)&data, ALG_RESULT_DATA, sizeof(data)); }

/**
 * Read the latest eTVOC measurement from the sensor.
 * @param data: Container to read data into.
 */
bool CCS811::read(ccs811_eTVOC_data_t& data) {
    ccs811_air_quality_data_t output;
    bool success = read(output.raw, ALG_RESULT_DATA, sizeof(output.raw));
    data = output.eTVOC_reading;
    return success;
}

/**
 * Read the raw data from the sensor.
 * @param data: Container to read the data into.
 */
bool CCS811::read(ccs811_raw_data_t& data) { return read((uint8_t*)&data, RAW_DATA, sizeof(data)); }

/**
 * Read the latest air quality measurements from the sensor.
 * @param data: Container to read data into.
 */
bool CCS811::read(ccs811_air_quality_data_t& data) { return read(data.raw, ALG_RESULT_DATA, sizeof(data)); }

/**
 * Read the latest data from the sensor.
 * All measurements, including raw data, status, and error registers are included.
 * @param data: Container to read data into.
 */
bool CCS811::read(ccs811_all_data_t& data) { return read(data.raw, ALG_RESULT_DATA, sizeof(data)); }

/**
 * Read the sensor baseline calibration? data from the sensor.
 * @param baseline: Container to read the data into.
 */
bool CCS811::read(ccs811_baseline_t& baseline) { return read(baseline.raw, BASELINE, sizeof(baseline)); }

/**
 * Read the hardware ID from the sensor.
 * @param id: Container to read id into.
 */
bool CCS811::read(ccs811_hardware_id_t& id) { return read(&id.raw, HW_ID); }

/**
 * Read the hardware version from the sensor.
 * @param version: Container to read version information into.
 */
bool CCS811::read(ccs811_hardware_version_t& version) { return read(&version.raw, HW_VERSION, sizeof(version)); }

/**
 * Read the firmware version from the sensor.
 * @param version: Container to read the firmware version into.
 */
bool CCS811::read(ccs811_firmware_boot_version_t& version) {
    return read(version.raw, FW_BOOT_VERSION, sizeof(version));
}

/**
 * Read the firmware application version from the sensor.
 * @param version: Container to read the version information into.
 */
bool CCS811::read(ccs811_firmware_application_version_t& version) {
    return read(version.raw, FW_APP_VERSION, sizeof(version));
}

/**
 * Read the error register from the sensor.
 * @param error: Container to read error flags into.
 */
bool CCS811::read(ccs811_error_t& error) { return read(&error.raw, ERROR_ID); }

////////////////////////////////////////////////////////////////////////////////

/**
 * Write a new measurement configuration to the sensor.
 * @param config: Configuration to write to the sensor.
 */
bool CCS811::write(ccs811_measure_config_t config) { return write(&config.raw, MEAS_MODE); }

/**
 * Write environmental data to the sensor for calculation purposes.
 * @param data: Humidity and temperature information to write to the sensor.
 */
bool CCS811::write(ccs811_environmental_data_t data) { return write(data.raw, ENV_DATA, sizeof(data)); }

/**
 * Write CO2 measurement thresholds to the sensor.
 * If enabled, the sensor will interrupt when the measured CO2 transitions between the low, medium, and high zones set
 * by the thresholds.
 * @param thresholds: New thresholds to write to the sensor.
 */
bool CCS811::write(ccs811_co2_thresholds_t thresholds) {
    swap_endianess((uint8_t*)&thresholds.low_limit, sizeof(thresholds.low_limit));
    swap_endianess((uint8_t*)&thresholds.high_limit, sizeof(thresholds.high_limit));
    return write(thresholds.raw, THRESHOLDS, sizeof(thresholds));
}

/**
 * Write a new baseline value to the sensor for conversions.
 * See ANxxx for details on the baseline values.
 * @param baseline: New baseline to write to the sensor.
 */
bool CCS811::write(ccs811_baseline_t baseline) { return write(baseline.raw, BASELINE, sizeof(baseline)); }

/**
 * Write application data to the sensor.
 * The sensor must be put into boot mode to accept application firmware.
 * @param data: Application data to write to the sensor.
 * @return True if written successfully
 */
bool CCS811::write(ccs811_application_data_t data) { return write(data.raw, APP_DATA, sizeof(data)); }

/**
 * Start a verify operation of the application firmware.
 * @param code: Data to write to the verify register. Not sure if the input makes any difference.
 * @return True if successfully written
 */
bool CCS811::write(ccs811_application_verify_t code) { return write(&code.raw, APP_VERIFY); }

/**
 * Start the application mode of the sensor.
 * The sensor must be put into application mode for normal sensor operation.
 * @param code: Data to wrute to the application start register. Not sure if the input makes any difference.
 * @return True if data was written successfully
 */
bool CCS811::write(ccs811_application_start_t code) { return write(&code.raw, APP_START); }

/**
 * Start an application firmware erase operation.
 * @param sequence: Erase sequence to be written to the register.
 * @return True if the sequence was written successfully.
 */
bool CCS811::write(ccs811_application_erase_t sequence) { return write(sequence, APP_ERASE, sizeof(sequence)); }

/**
 * Reset the device.
 * The correct sequence must be written to the register for a successful reset.
 * @param sequence: Reset sequence to write: 0x11, 0xE5, 0x72, 0x8A
 * @return True if the sequence was written successfully
 */
bool CCS811::write(ccs811_reset_t sequence) { return write(sequence.raw, SW_RESET, sizeof(sequence)); }

////////////////////////////////////////////////////////////////////////////////

/**
 * Write environmental data to the sensor for more accurate calculations.
 * @param temperature: Ambient temperature in degrees celsius.
 * @param humidity: Relative humidity in %
 * @return True if parameters were written successfully
 */
bool CCS811::write_environmental_data(float temperature, float humidity) {
    ccs811_environmental_data_t data;
    data.temperature.total = (uint16_t)((temperature - 25) * 512);
    data.humidity.total = (uint16_t)(humidity * 512);
    swap_endianess((uint8_t*)&data.temperature, sizeof(data.temperature));
    swap_endianess((uint8_t*)&data.humidity, sizeof(data.humidity));

    return write(data);
}

/**
 * Write CO2 thresholds to establish interrupt zones.
 * The sensor device will interrupt whenever the CO2 measurements change zones (if enabled).
 * @param low_threshold: Threshold for low-medium zone in parts per million.
 * @param high_threshold: Threshold for medium-high zone in parts per million.
 * @return True if parameters were written successfully
 */
bool CCS811::write_co2_thresholds(uint16_t low_threshold, uint16_t high_threshold) {
    ccs811_co2_thresholds_t thresholds;
    thresholds.low_limit = low_threshold;
    thresholds.high_limit = high_threshold;
    return write(thresholds);
}

/**
 * Get the latest measured equivalent CO2 concentration.
 * @return CO2 level in parts per million.
 */
uint16_t CCS811::get_eCO2() {
    ccs811_eCO2_data_t data;
    read(data);
    return data.total;
}

/**
 * Get the equivalent total volatile organic compound concentration.
 * @return eTVOC concentration in parts per billion.
 */
uint16_t CCS811::get_eTVOC() {
    ccs811_eTVOC_data_t data;
    read(data);
    return data.total;
}

/**
 * Software reset the sensor device.
 * @return True if the reset sequence was written successfully (does not indicate if the operation was successful).
 */
bool CCS811::reset() {
    ccs811_reset_t sequence;
    memcpy_P(sequence.raw, CCS811_RESET_SEQUENCE, sizeof(CCS811_RESET_SEQUENCE));
    return write(sequence);
}

/**
 * Start an application erase operation.
 * The result of the operation is placed in the status register.
 * @return True if the erase sequence was written successfully (does not indicate a successful operation).
 */
bool CCS811::start_application_erase() {
    ccs811_application_erase_t sequence;
    memcpy_P(sequence, CCS811_APPLICATION_ERASE_SEQUENCE, sizeof(CCS811_APPLICATION_ERASE_SEQUENCE));
    return write(sequence);
}

/**
 * Start a verify operation of the sensor's application firmware.
 * The result of the verify operation will be placed in the status register.
 * @return True if the verification command was written succesfully (does not indicate success of the operation)
 */
bool CCS811::start_application_verify() {
    ccs811_application_verify_t code;
    return write(code);
}

/**
 * Start the application mode of the sensor.
 * The sensor should automatically start in application mode after loading the application firmware.
 * The application start operation should only be needed if the application data is erased and the device is placed in
 * boot mode.
 * The status register will indicate if the application firmware was loaded successfully.
 * @return True if the operation command was successfully written (does not indicate operation success)
 */
bool CCS811::start_application_mode() {
    ccs811_application_start_t code;
    return write(code);
}

/**
 * Swap the endianness of a buffer.
 * Swap will occur in place.
 */
void swap_endianess(uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size / 2; i++) {
        uint8_t temp = buffer[i];
        buffer[i] = buffer[size - 1 - i];
        buffer[size - 1 - i] = temp;
    }
}