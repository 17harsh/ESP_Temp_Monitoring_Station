/*
 * dht.c
 *
 *  Created on: 18-Apr-2021
 *      Author: harsh
 */

#include "driver/dht.h"

#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"

/******************* GPIO_PIN PARAMETERS *******************/

#define NUMBER_VALID_GPIOS	12
const uint8_t gpio_num[NUMBER_VALID_GPIOS] = {0,1,2,3,4,5,9,10,12,13,14,15};
const uint32_t gpio_mux[NUMBER_VALID_GPIOS] = {PERIPHS_IO_MUX_GPIO0_U, PERIPHS_IO_MUX_U0TXD_U,PERIPHS_IO_MUX_GPIO2_U, PERIPHS_IO_MUX_U0RXD_U, PERIPHS_IO_MUX_GPIO4_U, PERIPHS_IO_MUX_GPIO5_U,
										PERIPHS_IO_MUX_SD_DATA2_U, PERIPHS_IO_MUX_SD_DATA3_U, PERIPHS_IO_MUX_MTDI_U, PERIPHS_IO_MUX_MTCK_U, PERIPHS_IO_MUX_MTMS_U, PERIPHS_IO_MUX_MTDO_U};
const uint8_t gpio_func[NUMBER_VALID_GPIOS] = {FUNC_GPIO0, FUNC_GPIO1, FUNC_GPIO2, FUNC_GPIO3, FUNC_GPIO4, FUNC_GPIO5, FUNC_GPIO9, FUNC_GPIO10, FUNC_GPIO12, FUNC_GPIO13, FUNC_GPIO14, FUNC_GPIO15};

/**********************************************************/

#define SENSING_TIME	2000000		//2 sec

//Set-Up Debugging Macros
#ifndef DHT_DEBUG
	#define LOG_DEBUG(message)					do {} while(0)
	#define LOG_DEBUG_ARGS(message, args...)	do {} while(0)
#else
	#define LOG_DEBUG(message)					do {os_printf("[DHT-DEBUG] %s", message); os_printf("\r\n");} while(0)
	#define LOG_DEBUG_ARGS(message, args...)	do {os_printf("[DHT-DEBUG] "); os_printf(message, args); os_printf("\r\n");} while(0)
#endif

/*********** STATIC VARIABLES *************/
static real32_t _maxCycles = 0;
static uint32_t _lastSystemTime = 0;
static int8_t _pin = -1;
/*****************************************/

DHT_STATUS _configureGPIO(const uint8_t iGPIO_Pin);
DHT_STATUS _waitBusLevelChange(const bool iLevel, uint8_t* oCounter);
real32_t _processHumidity(const uint8_t *iData);
real32_t _processTemperature(const uint8_t *iData, const TEMP_UNITS iTempUnit);


DHT_STATUS dht_init(const uint8_t iGPIO_Pin){

	LOG_DEBUG("DHT init.");

	// configure pin (make it input with pull up enabled)
	if(_configureGPIO(iGPIO_Pin) != DHT_OK){
		LOG_DEBUG("Unable to configure GPIO");
		return DHT_FAIL;
	}

	// read cpu clock period
	uint32_t rawCpuClockPeriod = 0;
	rawCpuClockPeriod = system_get_rtc_time(); // in us
	uint32_t integerPart = rawCpuClockPeriod >> 12;
	real32_t decimalPart = (real32_t)(rawCpuClockPeriod & 0xFFF);

	LOG_DEBUG_ARGS("RawCpuClockPeriod : %d, IntegerPart : %d , DecimalPart : %d", rawCpuClockPeriod, integerPart, decimalPart);

	while (decimalPart >= 1) decimalPart = decimalPart / 10;
	real32_t cpuClockPeriod = (real32_t)integerPart + decimalPart;

	//define maxCycles (dht read fail condition)
	_maxCycles = 1000/cpuClockPeriod;

	#ifdef DHT_DEBUG
		uint32_t cpuClockDebug = cpuClockPeriod*100;
		uint32_t maxCyclesDebug = _maxCycles;
		LOG_DEBUG_ARGS("maxcycles : %d , ApproxCpuClock : %d", maxCyclesDebug, cpuClockDebug);
	#endif

	//os_delay_us(2*1000*1000);

	//note the time
	_lastSystemTime = system_get_time();

	LOG_DEBUG("DHT init end." );
	return DHT_OK;
}

DHT_STATUS dht_read(float* ohumidty, float* otemperature, const TEMP_UNITS iTempUnit){
	LOG_DEBUG("DHT read.");

	if(ohumidty == NULL || otemperature == NULL || iTempUnit > 2){
		LOG_DEBUG("Invalid function parameters.");
		return DHT_FAIL;
	}

	uint32_t currentSystemTime = system_get_time();

	if (currentSystemTime - _lastSystemTime < SENSING_TIME){
		//error
		LOG_DEBUG_ARGS("2 Sec is required between poll times, It's been only %u ms", (currentSystemTime - _lastSystemTime)/1000 );
		return DHT_POLL_ERROR;
	}
	_lastSystemTime = currentSystemTime;

	//pulling pin high output (for statbility)
	GPIO_OUTPUT_SET(GPIO_ID_PIN(gpio_num[_pin]), 1);
	os_delay_us(250*1000);

	//send start signal (should be atleast 1ms)
	GPIO_OUTPUT_SET(GPIO_ID_PIN(gpio_num[_pin]), 0);
	os_delay_us(10*1000);

	//timing sensitive operation so disable all interrupts
	ETS_GPIO_INTR_DISABLE();

	/************ Critical data read start *****************/

	//send high for 40us and then read for DHT start signal
	GPIO_OUTPUT_SET(GPIO_ID_PIN(gpio_num[_pin]), 1);
	os_delay_us(40);

	//wait for DHT to start response signal
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(gpio_num[_pin]));
	PIN_PULLUP_EN(gpio_mux[_pin]);

	//wait low for high to low response signal (DHT start signal)
	if(_waitBusLevelChange(0, NULL) != DHT_OK ){
		LOG_DEBUG("Timeout waiting for DHT Start response low signal");
		return DHT_FAIL;
	}

	//wait for low to high response signal (DHT start signal)
	if(_waitBusLevelChange(1, NULL) != DHT_OK ){
		LOG_DEBUG("Timeout waiting for DHT end response high signal");
		return DHT_FAIL;
	}

	uint8_t counterArray[80] = {0};
	for(uint8_t i = 0; i < 80; i+=2){
		//will return be approx 50us equivalent of counter
		if(_waitBusLevelChange(0, &counterArray[i]) != DHT_OK ){
			LOG_DEBUG("Timeout waiting for DHT Data low signal");
			return DHT_FAIL;
		}
		//will return be either ~30us equivalent of counter represent 0 bit
		//or will return be either ~70us equivalent of counter represent 1 bit
		if(_waitBusLevelChange(1, &counterArray[i+1]) != DHT_OK ){
			LOG_DEBUG("Timeout waiting for DHT Data high signal");
			return DHT_FAIL;
		}
	}

	/************ Critical data read complete *****************/

	//timing critical operation complete so enable itnerrupts
	ETS_GPIO_INTR_ENABLE();

	uint8_t data[4] = {0};
	for (uint8_t i = 0; i < 80; i+=2){
		data[i/16] <<= 1;
		LOG_DEBUG_ARGS("%d: low : %d, high : %d",i/2, counterArray[i], counterArray[i+1]);
		if(counterArray[i] < counterArray[i+1]){
			data[i/16] |= 1;
		}
	}

	if(((data[0] + data[1] + data[2] + data[3]) & 255) != data[4]){
		LOG_DEBUG_ARGS("Checksum Error, Data : %d, %d, %d, %d, Checksum: %d ", data[0], data[1], data[2], data[3], data[4]);
		return DHT_FAIL;
	}
	else LOG_DEBUG_ARGS("Data : %d, %d, %d, %d, Checksum: %d ", data[0], data[1], data[2], data[3], data[4]);

	*ohumidty = _processHumidity(data);
	*otemperature = _processTemperature(data, iTempUnit);

	return DHT_OK;
}

DHT_STATUS _configureGPIO(const uint8_t iGPIO_Pin){
	LOG_DEBUG("configure GPIO.");

	_pin = -1;
	//get pin
	uint8_t index = 0;
	for(index = 0; index < NUMBER_VALID_GPIOS; ++index){
		if(gpio_num[index] == iGPIO_Pin){
			_pin = index;
			LOG_DEBUG_ARGS("%d will be configured as GPIO", iGPIO_Pin);
			break;
		}
	}
	if (_pin == -1){
		LOG_DEBUG_ARGS("%d cannot be configured as GPIO", iGPIO_Pin);
		return DHT_FAIL;
	}

	//gpio_init();

	//set GPIO Function Selection Register
	PIN_FUNC_SELECT(gpio_mux[_pin], gpio_func[index]);

	LOG_DEBUG("Pin function select register is set");

	//set pin as input low and enable pull up resistor
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(gpio_num[_pin]));
	PIN_PULLUP_EN(gpio_mux[_pin]);

	LOG_DEBUG("configure GPIO end.");
	return DHT_OK;
}

DHT_STATUS _waitBusLevelChange(const bool iLevel, uint8_t* oCounter){
	uint8_t counter = 0;
	while (GPIO_INPUT_GET(GPIO_ID_PIN(gpio_num[_pin])) == iLevel) {
		if(counter >= _maxCycles){
			LOG_DEBUG_ARGS("maxCycles reached %d ", counter);
			return DHT_FAIL;
		}
		counter++;
	}
	if(oCounter != NULL) *oCounter = counter;
	return DHT_OK;
}

real32_t _processHumidity(const uint8_t *iData){
	real32_t humidity = 0;
	uint16_t data = ((iData[0] << 8) | iData[1]);
	humidity = (real32_t)data/10;

	#ifdef DHT_DEBUG
		uint32_t humidity_i = humidity*10;
		LOG_DEBUG_ARGS("humidity : %d", humidity_i);
	#endif

	return humidity;
}

real32_t _processTemperature(const uint8_t *iData, const TEMP_UNITS iTempUnit){
	real32_t temperature = 0;
	uint16_t data = (((iData[2] << 8) | iData[3]) & 0x7FFF);
	temperature = (real32_t)data/10;
	if(iData[3] & 0x80){
		temperature *= -1;
	}

	#ifdef DHT_DEBUG
		int32_t temperature_i = temperature*10;
		LOG_DEBUG_ARGS("temperature : %d", temperature_i);
	#endif

	switch (iTempUnit) {
		case Celcius:
			break;
		case Fahrenheit:{
			temperature = (temperature * 1.8) + 32;
			break;
		}
		case Kelvin:{
			temperature = temperature + 273.15;
			break;
		}
		default:
			break;
	}

	return temperature;
}
