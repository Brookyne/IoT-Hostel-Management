#include "UNIT_ACMEASURE.h"

void UNIT_ACMEASURE::writeBytes(uint8_t addr, uint8_t reg, uint8_t *buffer,
                                uint8_t length) {
    _wire->beginTransmission(addr);
    _wire->write(reg);
    for (int i = 0; i < length; i++) {
        _wire->write(*(buffer + i));
    }
    _wire->endTransmission();
}

void UNIT_ACMEASURE::readBytes(uint8_t addr, uint8_t reg, uint8_t *buffer,
                               uint8_t length) {
    uint8_t index = 0;
    _wire->beginTransmission(addr);
    _wire->write(reg);
    _wire->endTransmission(false);
    _wire->requestFrom(addr, length);
    for (int i = 0; i < length; i++) {
        buffer[index++] = _wire->read();
    }
}

bool UNIT_ACMEASURE::begin(TwoWire *wire, uint8_t addr, uint8_t sda,
                           uint8_t scl, uint32_t speed) {
    _wire  = wire;
    _addr  = addr;
    _sda   = sda;
    _scl   = scl;
    _speed = speed;
    _wire->begin(_sda, _scl, _speed);
    delay(10);
    _wire->beginTransmission(_addr);
    uint8_t error = _wire->endTransmission();
    if (error == 0) {
        return true;
    } else {
        return false;
    }
}

uint16_t UNIT_ACMEASURE::getVoltage(void) {
    uint8_t data[4];
    readBytes(_addr, UNIT_ACMEASURE_VOLTAGE_REG, data, 2);
    uint16_t value = data[0] | (data[1] << 8);
    return value;
}

uint16_t UNIT_ACMEASURE::getCurrent(void) {
    uint8_t data[4];
    readBytes(_addr, UNIT_ACMEASURE_CURRENT_REG, data, 2);
    uint16_t value = data[0] | (data[1] << 8);
    return value;
}

uint32_t UNIT_ACMEASURE::getPower(void) {
    uint8_t data[4];
    readBytes(_addr, UNIT_ACMEASURE_POWER_REG, data, 4);
    uint32_t value =
        data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    return value;
}

uint32_t UNIT_ACMEASURE::getApparentPower(void) {
    uint8_t data[4];
    readBytes(_addr, UNIT_ACMEASURE_APPARENT_POWER_REG, data, 4);
    uint32_t value =
        data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    return value;
}

uint8_t UNIT_ACMEASURE::getPowerFactor(void) {
    uint8_t data[4];
    readBytes(_addr, UNIT_ACMEASURE_POWER_FACTOR_REG, data, 1);
    return data[0];
}

uint32_t UNIT_ACMEASURE::getKWH(void) {
    uint8_t data[4];
    readBytes(_addr, UNIT_ACMEASURE_KWH_REG, data, 4);
    uint32_t value =
        data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    return value;
}

void UNIT_ACMEASURE::setKWH(uint32_t value) {
    writeBytes(_addr, UNIT_ACMEASURE_KWH_REG, (uint8_t *)&value, 4);
}

void UNIT_ACMEASURE::getVoltageString(char *str) {
    char read_buf[7] = {0};

    readBytes(_addr, UNIT_ACMEASURE_VOLTAGE_STRING_REG, (uint8_t *)read_buf, 7);
    memcpy(str, read_buf, sizeof(read_buf));
}

void UNIT_ACMEASURE::getCurrentString(char *str) {
    char read_buf[7] = {0};

    readBytes(_addr, UNIT_ACMEASURE_CURRENT_STRING_REG, (uint8_t *)read_buf, 7);
    memcpy(str, read_buf, sizeof(read_buf));
}

void UNIT_ACMEASURE::getPowerString(char *str) {
    char read_buf[7] = {0};

    readBytes(_addr, UNIT_ACMEASURE_POWER_STRING_REG, (uint8_t *)read_buf, 7);
    memcpy(str, read_buf, sizeof(read_buf));
}

void UNIT_ACMEASURE::getApparentPowerString(char *str) {
    char read_buf[7] = {0};

    readBytes(_addr, UNIT_ACMEASURE_APPARENT_POWER_STRING_REG,
              (uint8_t *)read_buf, 7);
    memcpy(str, read_buf, sizeof(read_buf));
}

void UNIT_ACMEASURE::getPowerFactorString(char *str) {
    char read_buf[7] = {0};

    readBytes(_addr, UNIT_ACMEASURE_POWER_FACTOR_STRING_REG,
              (uint8_t *)read_buf, 4);
    memcpy(str, read_buf, sizeof(read_buf));
}

void UNIT_ACMEASURE::getKWH(char *str) {
    char read_buf[11] = {0};

    readBytes(_addr, UNIT_ACMEASURE_KWH_STRING_REG, (uint8_t *)read_buf, 11);
    memcpy(str, read_buf, sizeof(read_buf));
}

uint8_t UNIT_ACMEASURE::getVoltageFactor(void) {
    uint8_t data[4] = {0};

    readBytes(_addr, UNIT_ACMEASURE_VOLTAGE_FACTOR_REG, data, 1);
    return data[0];
}

uint8_t UNIT_ACMEASURE::getCurrentFactor(void) {
    uint8_t data[4] = {0};

    readBytes(_addr, UNIT_ACMEASURE_CURRENT_FACTOR_REG, data, 1);
    return data[0];
}

uint8_t UNIT_ACMEASURE::getReady(void) {
    char read_buf[7] = {0};

    readBytes(_addr, UNIT_ACMEASURE_GET_READY_REG, (uint8_t *)read_buf, 1);
    return read_buf[0];
}

void UNIT_ACMEASURE::setVoltageFactor(uint8_t value) {
    writeBytes(_addr, UNIT_ACMEASURE_VOLTAGE_FACTOR_REG, (uint8_t *)&value, 1);
}

void UNIT_ACMEASURE::setCurrentFactor(uint8_t value) {
    writeBytes(_addr, UNIT_ACMEASURE_CURRENT_FACTOR_REG, (uint8_t *)&value, 1);
}

void UNIT_ACMEASURE::saveVoltageCurrentFactor(void) {
    uint8_t value = 1;

    writeBytes(_addr, UNIT_ACMEASURE_SAVE_FACTOR_REG, (uint8_t *)&value, 1);
}

void UNIT_ACMEASURE::jumpBootloader(void) {
    uint8_t value = 1;

    writeBytes(_addr, JUMP_TO_BOOTLOADER_REG, (uint8_t *)&value, 1);
}

uint8_t UNIT_ACMEASURE::setI2CAddress(uint8_t addr) {
    _wire->beginTransmission(_addr);
    _wire->write(I2C_ADDRESS_REG);
    _wire->write(addr);
    _wire->endTransmission();
    _addr = addr;
    return _addr;
}

uint8_t UNIT_ACMEASURE::getI2CAddress(void) {
    _wire->beginTransmission(_addr);
    _wire->write(I2C_ADDRESS_REG);
    _wire->endTransmission();

    uint8_t RegValue;

    _wire->requestFrom(_addr, 1);
    RegValue = Wire.read();
    return RegValue;
}

uint8_t UNIT_ACMEASURE::getFirmwareVersion(void) {
    _wire->beginTransmission(_addr);
    _wire->write(FIRMWARE_VERSION_REG);
    _wire->endTransmission();

    uint8_t RegValue;

    _wire->requestFrom(_addr, 1);
    RegValue = Wire.read();
    return RegValue;
}
