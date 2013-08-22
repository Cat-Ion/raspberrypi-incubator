/* The port for the web interface */
#define PORT 8877

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
#define LOG_S 3600
#define LOG_SIZE (LOG_S/PERIOD_S)

/* Parameters for the PID control of the temperature and humidity. Still haven't tested these. */
#define T_P 1
#define T_I 0
#define T_IMAX 100
#define T_D 0

#define H_P 1
#define H_I 0
#define H_IMAX 100
#define H_D 0
