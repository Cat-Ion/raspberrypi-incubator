/* The port for the web interface */
#define PORT 8877

/* Path for the settings file */
#define SAVEPATH "settings"

/* Path for the log file written on exit */
#define LOGPATH "log_tmp"

/* Username and password for the web interface */
#define USER "admin"
#define PASS "password"

/* I2C addresses for the sensor and DAC. The sensor's address is
   constant, the DAC's address can change in the lowest three bits. */
#define SENSOR_ADDR 0x5C
#define DAC_ADDR 0x62

/* How often should the sensor make a measurement, and how long should
   a sample be kept in memory? */
#define PERIOD_S 10
#define PERIOD_DAY_S 60
#define LOG_S 3600

/* If this is configured, lets users configure the PID values over HTTP */
#define HTTP_CONFIG_PID 1

/* Minimum and maximum environment parameters for requests.
   
   For testudo hermanni boettgeri, the temperature range [26:33]
   should be safe-ish, but I'd recommend not setting it outside of
   [30:33]. Up to 30°C all tortoises should be male, above 33°C all
   female:
   | Temp | Male / % |
   |------+----------|
   | 30   | 100      |
   | 31   | 80       |
   | 31.5 | 50       |
   | 32   | 25       |
   | 33   | 0        |
   
   I know less about the humidity, but about 70% RH should be safe.
 */
#define TEMP_MIN 26
#define TEMP_DEF 31.5
#define TEMP_MAX 33
#define HUM_MIN 50
#define HUM_DEF 70
#define HUM_MAX 80

/* Parameters for the PID control of the temperature and humidity. Still haven't tested these. */
#define T_P 1
#define T_I 0
#define T_IMAX 100
#define T_D 0

#define H_P 1
#define H_I 0
#define H_IMAX 100
#define H_D 0

/* Minimum DAC output (as a fraction of the maximum output) that results in a
 * non-zero voltage on the load resistor.
   This exists to take care of the voltage drop of transistors.
   set_voltage() will take an input in [0:1] and transform it to something in
   [MIN_VOLTAGE:1] before forwarding it to the DAC.
 */
#define MIN_VOLTAGE 0
