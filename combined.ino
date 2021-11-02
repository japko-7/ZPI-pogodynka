#include <Wire.h>
#include <Adafruit_BME280.h>

#define PINWHEEL_DIAMETER 0.18
#define PINWHEEL_CIRC PINWHEEL_DIAMETER*PI
#define PINWHEEL_PIN 2
#define IMPULSES_PER_HALF_TURN 3

#define MEASUREMENT_HALF_TURNS 3
#define TPH_MEASUREMENT_INTERVAL 3000
#define SETUP_RETRY_DELAY 5000

#define SIZEOF_FLOATING_PART 8

// porównać wskazanie z rzeczzywistą prędkością - w którym punkcie ramienia mierzyć prędkość liniową
// czy prędkość liniowa to to samo, co prędkość wiatru, czy może wiatr jednocześnie napędza i hamuje z różnych stron

typedef union
{
    double as_double;
    byte as_binary[SIZEOF_FLOATING_PART];
} binary_double;

binary_double speed;
volatile unsigned long saved_times[MEASUREMENT_HALF_TURNS + 1];
volatile int count = 0;
volatile bool first_run = true;

Adafruit_BME280 bme;

void setup()
{
    Serial.begin(9600);
    while(!Serial);

    while (!setup_BME280())
    {
        Serial.println("ERROR");
        delay(SETUP_RETRY_DELAY);
    }

    pinMode(PINWHEEL_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PINWHEEL_PIN), read_wind_speed, RISING);

    saved_times[count] = micros();
}

void loop()
{
    print_stats();
    delay(TPH_MEASUREMENT_INTERVAL);
}

void read_wind_speed()
{
    count++;

    if(count % IMPULSES_PER_HALF_TURN == 0)
    {
        count = count % ((MEASUREMENT_HALF_TURNS + 1) * IMPULSES_PER_HALF_TURN);
        if (!count) first_run = false;

        int save_index = count / IMPULSES_PER_HALF_TURN;
        int read_index = (save_index + 1) % (MEASUREMENT_HALF_TURNS + 1);
        saved_times[save_index] = micros();

        if(first_run) return;

        unsigned long time_interval = saved_times[save_index] - saved_times[read_index];
        speed.as_double = MEASUREMENT_HALF_TURNS * 1000000 * PINWHEEL_CIRC / 2 / time_interval;

        Serial.write('S');
        Serial.write(speed.as_binary, SIZEOF_FLOATING_PART);
    }
}

int setup_BME280()
{
    unsigned status;
    status = bme.begin(BME280_ADDRESS_ALTERNATE, &Wire);

    return status;
}

void print_stats()
{
    binary_double converter;

    converter.as_double = bme.readTemperature();
    Serial.write('T');
    Serial.write(converter.as_binary, SIZEOF_FLOATING_PART);

    converter.as_double = bme.readPressure();
    Serial.write('P');
    Serial.write(converter.as_binary, SIZEOF_FLOATING_PART);

    converter.as_double = bme.readHumidity() * 100.0;
    Serial.write('H');
    Serial.write(converter.as_binary, SIZEOF_FLOATING_PART);
}
