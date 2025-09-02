#include "RadioHal.hpp"

template<>
int16_t RadioHal<SX1278>::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain, float tcxoVoltage)
{ 
    
    if (power>=17) radio->setCurrentLimit(150);
    return radio->begin(freq, bw, sf, cr, syncWord, power, preambleLength, gain);
}
template<>
int16_t RadioHal<SX1278>::begin()
{
    return radio->begin();
}

template<>
int16_t RadioHal<SX1276>::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain, float tcxoVoltage)
{   
    if (power>=17) radio->setCurrentLimit(150);
    return radio->begin(freq, bw, sf, cr, syncWord, power, preambleLength, gain);
}
template<>
int16_t RadioHal<SX1276>::begin()
{
    return radio->begin();
}

template<>
int16_t RadioHal<SX1268>::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain, float tcxoVoltage)
{
    if (power>=17) radio->setCurrentLimit(150);
    return radio->begin(freq, bw, sf, cr, syncWord, power, preambleLength, tcxoVoltage);
}
template<>
int16_t RadioHal<SX1268>::begin()
{
    return radio->begin();
}

template<>
int16_t RadioHal<SX1262>::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain, float tcxoVoltage)
{
    if (power>=17) radio->setCurrentLimit(150);
    return radio->begin(freq, bw, sf, cr, syncWord, power, preambleLength, tcxoVoltage);
}
template<>
int16_t RadioHal<SX1262>::begin()
{
    return radio->begin();
}


template<>
int16_t RadioHal<SX1280>::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain, float tcxoVoltage)
{
    return radio->begin(freq, bw, sf, cr, syncWord, power, preambleLength);
}
template<>
int16_t RadioHal<SX1280>::begin()
{
    return radio->begin();
}


template<>
int16_t RadioHal<SX1278>::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, bool enableOOK, float tcxoVoltage, bool useRegulatorLDO)
{
    if (power>=17) radio->setCurrentLimit(150);
    return radio->beginFSK(freq, br, freqDev, rxBw, power, preambleLength, enableOOK);
}

template<>
int16_t RadioHal<SX1276>::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, bool enableOOK, float tcxoVoltage, bool useRegulatorLDO)
{
    if (power>=17) radio->setCurrentLimit(150);
    return radio->beginFSK(freq, br, freqDev, rxBw, power, preambleLength, enableOOK);
}

template<>
int16_t RadioHal<SX1268>::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, bool enableOOK, float tcxoVoltage, bool useRegulatorLDO)
{
    if (power>=17) radio->setCurrentLimit(150);
    return radio->beginFSK(freq, br, freqDev, rxBw, power, preambleLength, tcxoVoltage, useRegulatorLDO);
}

template<>
int16_t RadioHal<SX1262>::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, bool enableOOK, float tcxoVoltage, bool useRegulatorLDO)
{
    if (power>=17) radio->setCurrentLimit(150);
    return radio->beginFSK(freq, br, freqDev, rxBw, power, preambleLength, tcxoVoltage, useRegulatorLDO);
}

template<>
int16_t RadioHal<SX1280>::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, bool enableOOK, float tcxoVoltage, bool useRegulatorLDO)
{
    return radio->beginGFSK(freq, br, freqDev, power, preambleLength);
}

template<>
float RadioHal<SX1280>::getRSSI(bool packet,bool skipReceive)
{
    return radio->getRSSI();
}

template<>
float RadioHal<SX1268>::getRSSI(bool packet,bool skipReceive)
{
    return radio->getRSSI(packet);
}

template<>
float RadioHal<SX1262>::getRSSI(bool packet,bool skipReceive)
{
    return radio->getRSSI(packet);
}

template<>
float RadioHal<SX1278>::getRSSI(bool packet,bool skipReceive)
{
    return radio->getRSSI(packet,skipReceive);
}

template<>
float RadioHal<SX1276>::getRSSI(bool packet,bool skipReceive)
{
    return radio->getRSSI(packet,skipReceive);
}




template<>
float RadioHal<SX1268>::getFrequencyError(bool autoCorrect)
{
    return radio->getFrequencyError();
}

template<>
float RadioHal<SX1262>::getFrequencyError(bool autoCorrect)
{
    return radio->getFrequencyError();
}

template<>
float RadioHal<SX1280>::getFrequencyError(bool autoCorrect)
{
    return radio->getFrequencyError();
}

template<>
float RadioHal<SX1278>::getFrequencyError(bool autoCorrect)
{
    return radio->getFrequencyError(autoCorrect);
}

template<>
float RadioHal<SX1276>::getFrequencyError(bool autoCorrect)
{
    return radio->getFrequencyError(autoCorrect);
}

template<>
void RadioHal<SX1268>::setPacketReceivedAction(void (*func)(void))
{
    radio->setPacketReceivedAction(func);
}

template<>
void RadioHal<SX1262>::setPacketReceivedAction(void (*func)(void))
{
    radio->setPacketReceivedAction(func);
}

template<>
void RadioHal<SX1278>::setPacketReceivedAction(void (*func)(void))
{
    radio->setPacketReceivedAction(func);

}

template<>
void RadioHal<SX1276>::setPacketReceivedAction(void (*func)(void))
{
    radio->setPacketReceivedAction(func);
}

template<>
void RadioHal<SX1280>::setPacketReceivedAction(void (*func)(void))
{
    radio->setPacketReceivedAction(func);
}

template<>
int16_t RadioHal<SX1278>::startReceive(uint8_t len, uint8_t mode)
{
    return radio->startReceive(len, mode);
}

template<>
int16_t RadioHal<SX1276>::startReceive(uint8_t len, uint8_t mode)
{
    return radio->startReceive(len, mode);
}

template<>
int16_t RadioHal<SX1268>::startReceive(uint8_t len, uint8_t mode)
{
    return radio->startReceive(len);
}

template<>
int16_t RadioHal<SX1262>::startReceive(uint8_t len, uint8_t mode)
{
    return radio->startReceive(len);
}

template<>
int16_t RadioHal<SX1280>::startReceive(uint8_t len, uint8_t mode)
{
    return radio->startReceive(len);
}

template<>
int16_t RadioHal<SX1280>::autoLDRO()
{
    return 0;
}

template<>
int16_t RadioHal<SX1278>::autoLDRO()
{
    return radio->autoLDRO();
}

template<>
int16_t RadioHal<SX1276>::autoLDRO()
{
    return radio->autoLDRO();
}

template<>
int16_t RadioHal<SX1268>::autoLDRO()
{
    return radio->autoLDRO();
}

template<>
int16_t RadioHal<SX1262>::autoLDRO()
{
    return radio->autoLDRO();
}

template<>
int16_t RadioHal<SX1278>::forceLDRO(bool enable)
{
    return radio->forceLDRO(enable);
}

template<>
int16_t RadioHal<SX1276>::forceLDRO(bool enable)
{
    return radio->forceLDRO(enable);
}

template<>
int16_t RadioHal<SX1268>::forceLDRO(bool enable)
{
    return radio->forceLDRO(enable);
}

template<>
int16_t RadioHal<SX1262>::forceLDRO(bool enable)
{
    return radio->forceLDRO(enable);
}

template<>
int16_t RadioHal<SX1280>::forceLDRO(bool enable)
{
    return 0;
}

template<>
int16_t RadioHal<SX1278>::fixedPacketLengthMode(uint8_t len)
{   if (len>64) len=64;
    return radio->fixedPacketLengthMode(len);
}

template<>
int16_t RadioHal<SX1276>::fixedPacketLengthMode(uint8_t len)
{   if (len>64) len=64;
    return radio->fixedPacketLengthMode(len);
}

template<>
int16_t RadioHal<SX1268>::fixedPacketLengthMode(uint8_t len)
{
    return radio->fixedPacketLengthMode(len);
}

template<>
int16_t RadioHal<SX1262>::fixedPacketLengthMode(uint8_t len)
{
    return radio->fixedPacketLengthMode(len);
}

template<>
int16_t RadioHal<SX1280>::fixedPacketLengthMode(uint8_t len)
{
    return 0;
}


template<>
int16_t RadioHal<SX1278>::setCRC(uint8_t len,	uint16_t initial , uint16_t polynomial , bool inverted )
{
    return radio->setCRC(len==1);
}

template<>
int16_t RadioHal<SX1276>::setCRC(uint8_t len,	uint16_t initial , uint16_t polynomial , bool inverted )
{
     return radio->setCRC(len==1);
}

template<>
int16_t RadioHal<SX1268>::setCRC(uint8_t len,	uint16_t initial , uint16_t polynomial , bool inverted )
{
    return radio->setCRC(len,initial,polynomial,inverted);
}

template<>
int16_t RadioHal<SX1262>::setCRC(uint8_t len,	uint16_t initial , uint16_t polynomial , bool inverted )
{
    return radio->setCRC(len,initial,polynomial,inverted);
}

template<>
int16_t RadioHal<SX1280>::setCRC(uint8_t len,	uint16_t initial , uint16_t polynomial , bool inverted )
{
    return 0;
}

template<>
int16_t RadioHal<SX1276>::setEncoding(uint8_t encoding) 
{
    if (encoding == 10)
        encoding = 0;

    return radio->setEncoding(encoding);
}

template<>
int16_t RadioHal<SX1278>::setEncoding(uint8_t encoding) 
{
    if (encoding == 10)
        encoding = 0;

    return radio->setEncoding(encoding);
}

template<>
int16_t RadioHal<SX1262>::setEncoding(uint8_t encoding) 
{
    if (encoding == 10)
        encoding = 1;

    return radio->setEncoding(encoding);
}

template<>
int16_t RadioHal<SX1268>::setEncoding(uint8_t encoding) 
{
    if (encoding == 10)
        encoding = 1;

    return radio->setEncoding(encoding);
}

template<>
int16_t RadioHal<SX1280>::setEncoding(uint8_t encoding) 
{
    if (encoding == 10)
        encoding = 1;

    return radio->setEncoding(encoding);
}

template<>
int16_t RadioHal<SX1268>::setWhitening(bool enabled, uint16_t initial)
{
    return radio->setWhitening(enabled,initial);
}


template<>
int16_t RadioHal<SX1262>::setWhitening(bool enabled, uint16_t initial)
{
    return 0;
}

template<>
int16_t RadioHal<SX1276>::setWhitening(bool enabled, uint16_t initial)
{
    return 0;
}

template<>
int16_t RadioHal<SX1278>::setWhitening(bool enabled, uint16_t initial)
{
    return 0;
}

template<>
int16_t RadioHal<SX1280>::setWhitening(bool enabled, uint16_t initial)
{
    return 0;
}


template<>
int16_t RadioHal<SX1278>::invertIQ(bool enable)
{
    return radio->invertIQ(enable);
}

template<>
int16_t RadioHal<SX1276>::invertIQ(bool enable)
{
    return radio->invertIQ(enable);
}

template<>
int16_t RadioHal<SX1268>::invertIQ(bool enable)
{
    return radio->invertIQ(enable);
}

template<>
int16_t RadioHal<SX1262>::invertIQ(bool enable)
{
    return radio->invertIQ(enable);
}

template<>
int16_t RadioHal<SX1280>::invertIQ(bool enable)
{
    return radio->invertIQ(enable);
}