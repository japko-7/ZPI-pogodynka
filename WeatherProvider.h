#pragma once
class WeatherProvider
{
public:
//	virtual WeatherProvider();
//	virtual ~WeatherProvider();

	virtual double getTempK() { return 273.15; }
	virtual double getPressurePa() { return 101325; }
	virtual double getHumidityPerc() { return 50; }
	virtual double getVerticlaWind() { return 0; }
	virtual double getHorizontalWindVelocity() { return 0; }
	virtual double getHorizontalWindAngle() { return 0; }
};

