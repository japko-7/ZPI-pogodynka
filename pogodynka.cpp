//
//  pogodynka.cpp
//  ZPI
//
//  Created by Filip Jabłoński on 24/10/2021.
//

#include "pogodynka.hpp"
#include <math.h>

#define ARDUINO_BAUD_RATE 9600
#define WIND_DIRECTION_PROVIDER_BAUD_RATE 4800

#define WIND_SPEED_CODE 'S'
#define TEMPERATURE_CODE 'T'
#define HUMIDITY_CODE 'H'
#define PRESSURE_CODE 'P'
#define ERROR_CODE 'E'
#define SERIAL_CONNECT_SUCCESS 1
#define SERIAL_CONNECT_ERROR 2
#define ARDUINO_SETUP_ERROR 3
#define READING_INTERRUPTED 0
#define SIZEOF_FLOATING_PART 8

#define ARDUINO_SETUP_TRIES


// Compute the MODBUS RTU CRC
uint16_t ModRTU_CRC(std::byte * buf, int len)
{
	uint16_t crc = 0xFFFF;
  
	for (int pos = 0; pos < len; pos++)
	{
		crc ^= (uint16_t)buf[pos];				// XOR byte into least sig. byte of crc

		for (int i = 8; i != 0; i--)			// Loop over each bit
		{
				if ((crc & 0x0001) != 0)		// If the LSB is set
				{
					crc >>= 1;					// Shift right and XOR 0xA001
					crc ^= 0xA001;
				}
				else                            // Else LSB is not set
					crc >>= 1;                  // Just shift right
		}
	}
	// Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
	return crc;
}

char pogodynka::run() /// TODO: beautify
{
	// connect to both serials
	char wind_direction_provider_serial_opening_status = wind_direction_provider_serial.openDevice(wind_direction_provider_port, WIND_DIRECTION_PROVIDER_BAUD_RATE);
	if (wind_direction_provider_serial_opening_status != SERIAL_CONNECT_SUCCESS) return wind_direction_provider_serial_opening_status;
	std::cout << "Successful connection to " << wind_direction_provider_port << "\n";
	
	char arduino_serial_opening_status = arduino_serial.openDevice(arduino_port, ARDUINO_BAUD_RATE);
	if (arduino_serial_opening_status != SERIAL_CONNECT_SUCCESS) return arduino_serial_opening_status;
	std::cout << "Successful connection to " << arduino_port << "\n";

	
	// start reading
	auto wind_direction_read_feedback = std::async(&pogodynka::continous_wind_direction_read, this);
	auto arduino_read_feedback = std::async(&pogodynka::continous_read_from_arduino, this);

	// possible need of reconnecting the arduino
	arduino_read_feedback.wait();

	for(int tries_left = ARDUINO_SETUP_TRIES - 1; arduino_read_feedback.get() == ARDUINO_SETUP_ERROR && tries_left > 0; tries_left--)
	{
		std::cout << "arduino setup error! retrying in 3 seconds...";
		sleep(3);

		arduino_serial_opening_status = arduino_serial.openDevice(arduino_port, ARDUINO_BAUD_RATE);
		if (arduino_serial_opening_status != SERIAL_CONNECT_SUCCESS) return arduino_serial_opening_status;
		std::cout << "Successful connection to " << arduino_port << "\n";
		arduino_read_feedback = std::async(&pogodynka::continous_read_from_arduino, this);
		arduino_read_feedback.wait();
	}
	
	return 0;
}


int pogodynka::continous_wind_direction_read()
{
	std::byte request[6];
	
	request[0] = (std::byte) 0x01; // client address
	
	request[1] = (std::byte) 0x03; // function code
	
	request[2] = (std::byte) 0x00; // first register to read
	request[3] = (std::byte) 0x00;
	
	request[4] = (std::byte) 0x00; // number of registers to read
	request[5] = (std::byte) 0x01;
	
	uint16_t CRC = ModRTU_CRC(request, 6);
	std::byte * read_data = new std::byte[1];
	std::byte response[2];
	unsigned short int_meas = 0;
	
	while (true) /// TODO: add a stop condition?
	{
		// send the request
		wind_direction_provider_serial.writeBytes(request, 6);
		std::byte* crc_ptr = (std::byte*)&CRC;
		wind_direction_provider_serial.writeBytes(crc_ptr, 1);
		crc_ptr++;
		wind_direction_provider_serial.writeBytes(crc_ptr, 1);
		
		
		wind_direction_provider_serial.readBytes(read_data, 1);
//		std::cout << "address: " << std::hex << std::to_integer<int>(*read_data) << "\n";
		wind_direction_provider_serial.readBytes(read_data, 1);
//		std::cout << "function: " << std::hex << std::to_integer<int>(*read_data) << "\n";
		wind_direction_provider_serial.readBytes(read_data, 1);
//		std::cout << "bytes to read: " << std::hex << std::to_integer<int>(*read_data) << "\n";
		
		wind_direction_provider_serial.readBytes(response, 2);
		std::byte temp = response[0];
		response[0] = response[1];
		response[1] = temp;
		std::memcpy(&int_meas, response, sizeof(short));
		
		data.wind_direction = float(int_meas) / 1800 * M_PI;
//		std::cout << std::dec << "direction: " << data.wind_direction << " rad\n";
		
		// response CRC - TODO: might want to check it
		wind_direction_provider_serial.readBytes(read_data, 1);
		wind_direction_provider_serial.readBytes(read_data, 1);
	}
	
	return READING_INTERRUPTED;
}

int pogodynka::continous_read_from_arduino()
{
	char * datatype_code = new char [1];
	void * data_buffer = new std::byte[SIZEOF_FLOATING_PART];
	double read_data = 0;
	
	while(true) /// TODO: add a stop condition?
	{
		arduino_serial.readChar(datatype_code);
		
		if( *datatype_code == ERROR_CODE )
		{
			std::cout << "weather conditions provider setup failed!";
			arduino_serial.closeDevice();
			return ARDUINO_SETUP_ERROR;
		}
		
		arduino_serial.readBytes(data_buffer, SIZEOF_FLOATING_PART);
		read_data = * (float*)data_buffer;
		
		if( *datatype_code == TEMPERATURE_CODE )
		{
//			std::cout << "temperature: ";
			data.temperature = read_data + CELCIUS_TO_KELVIN_DIFF;
		}
		if( *datatype_code == PRESSURE_CODE )
		{
//			std::cout << "pressure: ";
			data.pressure = read_data;
		}
		if( *datatype_code == HUMIDITY_CODE )
		{
//			std::cout << "humidity: ";
			data.humidty = read_data;
		}
		if( *datatype_code == WIND_SPEED_CODE )
		{
//			std::cout << "wind speed: ";
			data.wind_velocity = read_data;
		}
		
//		std::cout << read_data << std::endl;
	}
	
	return READING_INTERRUPTED;
}
