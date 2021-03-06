
#include "light_sensor.h"
#include <stdint.h>
#include <math.h>


int sensor_enable()
{
    int status;
    status = i2c_write_byte(LIGHT_SENSOR_ADDR, POWER_ON_BIT, (CONTROL_REG | COMMAND_REG));

    return status;
}

int sensor_disable()
{
    int status = i2c_write_byte(LIGHT_SENSOR_ADDR, POWER_OFF_BIT, (CONTROL_REG | COMMAND_REG));
    return status;
}

int write_int_tlow(uint16_t tlow)
{
    int status = i2c_write_word(LIGHT_SENSOR_ADDR, tlow, (THRESHLOWLOW_REG | COMMAND_REG | WORD_SET_BIT));

    return status;
}

int read_int_tlow(uint16_t *tlow)
{
    uint16_t data;

    int status = i2c_read_bytes(LIGHT_SENSOR_ADDR, (uint8_t *)&data, (THRESHLOWLOW_REG | COMMAND_REG | WORD_SET_BIT), sizeof(data));
    if (status == -1)
        return status;

    *tlow = data;

    return status;
}

int write_int_thigh(uint16_t thigh)
{
    int status = i2c_write_word(LIGHT_SENSOR_ADDR, thigh, (THRESHLOWHIGH_REG | COMMAND_REG | WORD_SET_BIT));

    return status;
}

int read_int_thigh(uint16_t *thigh)
{
    uint16_t data;

    int status = i2c_read_bytes(LIGHT_SENSOR_ADDR, (uint8_t *)&data, (THRESHLOWHIGH_REG | COMMAND_REG | WORD_SET_BIT), sizeof(data));
    if (status == -1)
        return status;

    *thigh = data;

    return status;
}

int read_sensorID(uint8_t *id)
{
    int status = i2c_read(LIGHT_SENSOR_ADDR, id, (ID_REG | COMMAND_REG));

    return status;
}

int read_timer_reg(uint8_t *data)
{
    int status = i2c_read(LIGHT_SENSOR_ADDR, data, (TIMING_REG | COMMAND_REG));

    return status;
}

int read_interrupt_reg(uint8_t *data)
{
    int status = i2c_read(LIGHT_SENSOR_ADDR, data, (INTERRUPT_REG | COMMAND_REG));

    return status;   
}

int read_command_reg(uint8_t *data)
{
    int status = i2c_read(LIGHT_SENSOR_ADDR, data, (COMMAND_REG));

    return status;
}

int enable_interrupt(uint8_t set)
{
    int status;
    uint8_t data;

    status = i2c_read(LIGHT_SENSOR_ADDR, &data, (INTERRUPT_REG | COMMAND_REG));

    if (set) {
        data |= (uint8_t)(1<<4);
    }
    else {
        data &= ~((uint8_t)(1<<4));
    }

    status = i2c_write_byte(LIGHT_SENSOR_ADDR, data, (INTERRUPT_REG | COMMAND_REG));

    return status;
}

int clear_pendingInterrupt()
{
    int status = i2c_write(LIGHT_SENSOR_ADDR, (CLEAR_PENDING_INT_BIT | COMMAND_REG));

    return status;
}

int set_integrationTime(uint8_t value)
{
    int status;
    uint8_t data;

    status = i2c_read(LIGHT_SENSOR_ADDR, &data, (TIMING_REG | COMMAND_REG));

    //printf("Read %d from timing regx\n", data);

    data &= ~((uint8_t)INTG_TIME_SET_BIT);
    data |= value;

    //printf("Writing %d to timing register\n", data);

    status = i2c_write_byte(LIGHT_SENSOR_ADDR, data, (TIMING_REG | COMMAND_REG));

    return status;
}

int set_manualControl(uint8_t on)
{
    int status;
    uint8_t data;

    status = i2c_read(LIGHT_SENSOR_ADDR, &data, (TIMING_REG | COMMAND_REG));
    //printf("Read %d from timing regx\n", data);

    data &= ~((uint8_t)MANUAL_CONTROL_ENABLE_BIT(1));
    data |= (uint8_t)MANUAL_CONTROL_ENABLE_BIT(on);

    //printf("Writing %d to timing register\n", data);

    status = i2c_write_byte(LIGHT_SENSOR_ADDR, data, (TIMING_REG | COMMAND_REG));

    return status;
}

int read_channel0(uint16_t *data)
{
    uint8_t ch0_MSB, ch0_LSB;

    int status = i2c_read(LIGHT_SENSOR_ADDR, &ch0_LSB, (DATA0LOW_REG | COMMAND_REG));
    if (status == -1) {
        printf("Failed to read DATA0LOW_REG\n");
        return status;
    }

    status = i2c_read(LIGHT_SENSOR_ADDR, &ch0_MSB, (DATA0HIGH_REG | COMMAND_REG));
    if (status == -1) {
        printf("Failed to read DATA1HIGH_REG\n");
        return status;
    }
    
    //printf("MSB and LSB = %d    %d\n", ch0_MSB, ch0_LSB);

    *data = (ch0_MSB << 8) | ch0_LSB;

    return status;
}

int read_channel1(uint16_t *data)
{
    uint8_t ch1_MSB, ch1_LSB;

    int status = i2c_read(LIGHT_SENSOR_ADDR, &ch1_LSB, (DATA1LOW_REG | COMMAND_REG));
    if (status == -1) {
        printf("Failed to read DATA1LOW_REG\n");
        return status;
    }

    status = i2c_read(LIGHT_SENSOR_ADDR, &ch1_MSB, (DATA1HIGH_REG | COMMAND_REG));
    if (status == -1) {
        printf("Failed to read DATA1HIGH_REG\n");
        return status;
    }
    
    //printf("MSB and LSB = %d    %d\n", ch1_MSB, ch1_LSB);

    *data = (ch1_MSB << 8) | ch1_LSB;

    return status;
}

float get_sensorlux()
{
    uint16_t CH0, CH1;
    float div;
    float Sensor_Lux = -1;

    int status = read_channel0(&CH0);
    if (status == -1)
        return Sensor_Lux;

    status = read_channel1(&CH1);
    if (status == -1)
        return Sensor_Lux;

    if (CH0 != 0)
        div = (float)CH1 / (float)CH0;
    else
        div = 0;

    //printf("Ch0 = %f    ch1 = %f    div = %f\n", CH0, CH1, div);

        if ((div > 0) && (div <= 0.50)) {
            Sensor_Lux = (0.0304 * CH0) - (0.062 * CH0 * powf(div, 1.4));
            //printf("In S1   Sensor Value = %f\n", Sensor_Lux);
        }
        else if ((div > 0.50) && (div <= 0.61)) {
            Sensor_Lux = (0.0224 * CH0) - (0.031 * CH1);
            //printf("In S2   Sensor Value = %f\n", Sensor_Lux);
        }
        else if ((div > 0.61) && (div <= 0.80)) {
            Sensor_Lux = (0.0128 * CH0) - (0.0153 * CH1);
            //printf("In S3   Sensor Value = %f\n", Sensor_Lux);
        }
        else if ((div > 0.80) && (div <= 1.30)) {
            Sensor_Lux = (0.00146 * CH0) - (0.00112 * CH1);
            //printf("In S4   Sensor Value = %f\n", Sensor_Lux);
        }
        else if (div > 1.30) {
            Sensor_Lux = 0;
            //printf("In S5   Sensor Value = %f\n", Sensor_Lux);

        }
        else {
            //printf("Undefined\n");
        }

    return Sensor_Lux;
}
