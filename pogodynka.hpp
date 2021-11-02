//
//  pogodynka.hpp
//  ZPI
//
//  Created by Filip Jabłoński on 24/10/2021.
//

#ifndef pogodynka_hpp
#define pogodynka_hpp

#include <stdio.h>
#include <future>

#include "serialib.h"
#include "WeatherProvider.h"


#if defined (_WIN32) || defined(_WIN64)
	//for serial ports above "COM9", we must use this extended syntax of "\\.\COMx".
	//also works for COM0 to COM9.
	//https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea?redirectedfrom=MSDN#communications-resources
	#define WEATHER_ARDUINO_PORT "\\\\.\\COM1"
	#define WIND_DIRECTION_PROVIDER_PORT "\\\\.\\COM2"
#elif defined (__linux__) || defined(__APPLE__)
//	#define SERIAL_PORT "/dev/ttyS0"
	#define WEATHER_ARDUINO_PORT "/dev/cu.usbmodem14101"
	#define WIND_DIRECTION_PROVIDER_PORT "/dev/cu.usbserial-AR0KLG46"
#endif

#define CELCIUS_TO_KELVIN_DIFF 273.15
#define DEFAULT_TEMPEATURE (20 + CELCIUS_TO_KELVIN_DIFF )
#define DEFAULT_PRESSURE 101325
#define DEFAULT_HUMIDITY 50
#define DEFAULT_WIND_VELOCITY 0
#define DEFAULT_WIND_DIRERCTION 0

struct weather_state
{
	float
		temperature = DEFAULT_TEMPEATURE,
		pressure = DEFAULT_PRESSURE,
		humidty = DEFAULT_HUMIDITY,
		wind_velocity = DEFAULT_WIND_VELOCITY,
		wind_direction = DEFAULT_WIND_DIRERCTION;
};

class pogodynka : public WeatherProvider
{
	weather_state data;
	const char * wind_direction_provider_port, * arduino_port;
	serialib wind_direction_provider_serial, arduino_serial;
	
	int continous_read_from_arduino();
	int continous_wind_direction_read();
	
public:
	pogodynka(const char * wind_direction_provider_port = WIND_DIRECTION_PROVIDER_PORT, const char * arduino_port = WEATHER_ARDUINO_PORT):
	wind_direction_provider_port(wind_direction_provider_port),
	arduino_port(arduino_port)
	{}
	
	char run();
	
	/* implementing the weather povider */
	double getTempK() { return data.temperature; }
	double getPressurePa() { return data.pressure; }
	double getHumidityPerc() { return data.humidty; }
	double getVerticlaWind() { return 0; }
	double getHorizontalWindVelocity() { return data.wind_velocity; }
	double getHorizontalWindAngle() { return data.wind_direction; }
	
};

#endif /* pogodynka_hpp */
