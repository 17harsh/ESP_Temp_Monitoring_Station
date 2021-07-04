/*
 * dht.h
 *
 *  Created on: 18-Apr-2021
 *      Author: harsh
 */

#ifndef INCLUDE_DRIVER_DHT_H_
#define INCLUDE_DRIVER_DHT_H_

#include "c_types.h"

//un-comment this for debugging messages
//#define DHT_DEBUG

typedef enum tempUnits{
	Celcius,
	Fahrenheit,
	Kelvin
}TEMP_UNITS;

typedef enum dhtStatus{
	DHT_OK,
	DHT_FAIL,
	DHT_POLL_ERROR
}DHT_STATUS;

/**
  * function : 	initialize timer data and sets up GPIO used for sensor data bus.
  * 			Call this again	if CPU clock/frequency is changed
  * @param iGPIO_Pin		:	gpio pin number which is used as data bus
  * @return DHT_STATUS		: OK if configured successfully, FAIL if unsupported GPIO pin is entered
  */
DHT_STATUS dht_init(const uint8_t iGPIO_Pin);

/**
  * function : reads and outputs the humidity and temperature values from DHT Sensor
  * @param ohumidty		:	humidity output value (in percent)
  * @param otemperature	:	temperature output value (based on iTempUnit)
  * @param tempUnit		:	Unit of temperature
  * @return DHT_STATUS	: 	OK if successful, FAILl if something fails, POLL_ERROR if
  * 						read is called within 2sec after last read or after init.
  */
DHT_STATUS dht_read(float* ohumidty, float* otemperature, const TEMP_UNITS iTempUnit);

#endif /* INCLUDE_DRIVER_DHT_H_ */
