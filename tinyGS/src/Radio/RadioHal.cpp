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
int16_t RadioHal<LR1121>::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain, float tcxoVoltage)
{
    
    if (power > 22) power = 22;  // LR1121 HP PA max is +22 dBm
    // Snap BW to nearest LR1121-supported value
        if      (bw <= 62.5f)  bw = 62.5f;
        else if (bw <= 125.0f) bw = 125.0f;
        else if (bw <= 250.0f) bw = 250.0f;
        else                   bw = 500.0f;

    return radio->begin(freq, bw, sf, cr, syncWord, power, preambleLength, tcxoVoltage);
}

template<>
int16_t RadioHal<LR1121>::begin()
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
int16_t RadioHal<SX1278>::startReceive()
{
    return radio->startReceive();
}

template<>
int16_t RadioHal<SX1276>::startReceive()
{
    return radio->startReceive();
}

template<>
int16_t RadioHal<SX1268>::startReceive()
{
    return radio->startReceive();
}

template<>
int16_t RadioHal<SX1262>::startReceive()
{
    return radio->startReceive();
}

template<>
int16_t RadioHal<SX1280>::startReceive()
{
    return radio->startReceive();
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
    return radio->setCRC(len > 0);
}

template<>
int16_t RadioHal<SX1276>::setCRC(uint8_t len,	uint16_t initial , uint16_t polynomial , bool inverted )
{
     return radio->setCRC(len > 0);
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

template<>
int16_t RadioHal<SX1278>::explicitHeader()
{
    return radio->explicitHeader();
}

template<>
int16_t RadioHal<SX1276>::explicitHeader()
{
    return radio->explicitHeader();
}

template<>
int16_t RadioHal<SX1268>::explicitHeader()
{
    return radio->explicitHeader();
}

template<>
int16_t RadioHal<SX1262>::explicitHeader()
{
    return radio->explicitHeader();
}

template<>
int16_t RadioHal<SX1280>::explicitHeader()
{
    return radio->explicitHeader();
}

template<>
int16_t RadioHal<SX1278>::implicitHeader(size_t len)
{
    return radio->implicitHeader(len);
}

template<>
int16_t RadioHal<SX1276>::implicitHeader(size_t len)
{
    return radio->implicitHeader(len);
}

template<>
int16_t RadioHal<SX1268>::implicitHeader(size_t len)
{
    return radio->implicitHeader(len);
}

template<>
int16_t RadioHal<SX1262>::implicitHeader(size_t len)
{
    return radio->implicitHeader(len);
}

template<>
int16_t RadioHal<SX1280>::implicitHeader(size_t len)
{
    return radio->implicitHeader(len);
}

// ─── LR1121 ────────────────────────────────────────────────────────────────




template<>
int16_t RadioHal<LR1121>::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, bool enableOOK, float tcxoVoltage, bool useRegulatorLDO)
{
    if (power > 22) power = 22;
    return radio->beginGFSK(freq, br, freqDev, rxBw, power, preambleLength, tcxoVoltage);
}

template<>
int16_t RadioHal<LR1121>::autoLDRO()
{
    return 0; // LR1121 manages this internally
}

template<>
int16_t RadioHal<LR1121>::forceLDRO(bool enable)
{
    return 0;
}

template<>
int16_t RadioHal<LR1121>::setCRC(uint8_t len, uint16_t initial, uint16_t polynomial, bool inverted)
{
    return radio->setCRC(len, initial, polynomial, inverted);
}

template<>
void RadioHal<LR1121>::setPacketReceivedAction(void (*func)(void))
{
    radio->setPacketReceivedAction(func);
}

template<>
int16_t RadioHal<LR1121>::startReceive()
{
    return radio->startReceive();
}

template<>
float RadioHal<LR1121>::getRSSI(bool packet, bool skipReceive)
{
    return radio->getRSSI();
}

template<>
float RadioHal<LR1121>::getFrequencyError(bool autoCorrect)
{
    return radio->getFrequencyError();

}

template<>
int16_t RadioHal<LR1121>::fixedPacketLengthMode(uint8_t len)
{
    return radio->fixedPacketLengthMode(len);
}

template<>
int16_t RadioHal<LR1121>::setEncoding(uint8_t encoding)
{
    if (encoding == 10)
        encoding = 1;
    return radio->setEncoding(encoding);
}

template<>
int16_t RadioHal<LR1121>::setWhitening(bool enabled, uint16_t initial)
{
    return 0;
}

template<>
int16_t RadioHal<LR1121>::invertIQ(bool enable)
{
    return radio->invertIQ(enable);
}

template<>
int16_t RadioHal<LR1121>::explicitHeader()
{
    return radio->explicitHeader();
}

template<>
int16_t RadioHal<LR1121>::implicitHeader(size_t len)
{
    return radio->implicitHeader(len);
}

// ─── setRxBoostedGainMode ───────────────────────────────────────────────────
// SX126x: supported with persist flag
template<> int16_t RadioHal<SX1268>::setRxBoostedGainMode(bool enable) { return radio->setRxBoostedGainMode(enable, true); }
template<> int16_t RadioHal<SX1262>::setRxBoostedGainMode(bool enable) { return radio->setRxBoostedGainMode(enable, true); }
// SX127x and SX1280: not supported
template<> int16_t RadioHal<SX1278>::setRxBoostedGainMode(bool enable) { return RADIOLIB_ERR_NONE; }
template<> int16_t RadioHal<SX1276>::setRxBoostedGainMode(bool enable) { return RADIOLIB_ERR_NONE; }
template<> int16_t RadioHal<SX1280>::setRxBoostedGainMode(bool enable) { return RADIOLIB_ERR_NONE; }
// LR1121: supported
template<> int16_t RadioHal<LR1121>::setRxBoostedGainMode(bool enable) { return radio->setRxBoostedGainMode(enable); }
//template<> int16_t RadioHal<LR1121>::setRxBoostedGainMode(bool enable) { return RADIOLIB_ERR_NONE; }


// ─── LR2021 ────────────────────────────────────────────────────────────────

template<>
int16_t RadioHal<LR2021>::begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain, float tcxoVoltage)
{
    radio->irqDioNum =8;
    if (power > 22) power = 22;  // LR2021 HP PA max is +22 dBm

    if      (bw <= 31.25f)    bw = 31.25f;
    else if (bw <= 41.67f)    bw = 41.67f;
    else if (bw <= 62.5f)     bw = 62.5f;
    else if (bw <= 83.34f)    bw = 83.34f;
    else if (bw <= 101.5625f) bw = 101.5625f;
    else if (bw <= 125.0f)    bw = 125.0f;
    else if (bw <= 203.125f)  bw = 203.125f;
    else if (bw <= 250.0f)    bw = 250.0f;
    else if (bw <= 406.25f)   bw = 406.25f;
    else if (bw <= 500.0f)    bw = 500.0f;
    else if (bw <= 812.5f)    bw = 812.5f;
    else                      bw = 1000.0f;

    return radio->begin(freq, bw, sf, cr, syncWord, power, preambleLength, tcxoVoltage);
}

template<>
int16_t RadioHal<LR2021>::begin()
{
    radio->irqDioNum =8;
    return radio->begin();
}

template<>
int16_t RadioHal<LR2021>::beginFSK(float freq, float br, float freqDev, float rxBw, int8_t power, uint16_t preambleLength, bool enableOOK, float tcxoVoltage, bool useRegulatorLDO)
{
      radio->irqDioNum =8;
    if (power > 22) power = 22;
    return radio->beginGFSK(freq, br, freqDev, rxBw, power, preambleLength, tcxoVoltage);
}

template<>
int16_t RadioHal<LR2021>::autoLDRO()
{
    return 0; // LR2021 manages this internally
}

template<>
int16_t RadioHal<LR2021>::forceLDRO(bool enable)
{
    return 0;
}

template<>
int16_t RadioHal<LR2021>::setCRC(uint8_t len, uint16_t initial, uint16_t polynomial, bool inverted)
{
    return radio->setCRC(len, initial, polynomial, inverted);
}

template<>
void RadioHal<LR2021>::setPacketReceivedAction(void (*func)(void))
{
    radio->setPacketReceivedAction(func);
}

template<>
int16_t RadioHal<LR2021>::startReceive()
{
    return radio->startReceive();
}

template<>
float RadioHal<LR2021>::getRSSI(bool packet, bool skipReceive)
{
    return radio->getRSSI(packet, skipReceive);
}

template<>
float RadioHal<LR2021>::getFrequencyError(bool autoCorrect)
{
    return 0.0f; // not available in LR2021
}

template<>
int16_t RadioHal<LR2021>::fixedPacketLengthMode(uint8_t len)
{
    return radio->fixedPacketLengthMode(len);
}

template<>
int16_t RadioHal<LR2021>::setEncoding(uint8_t encoding)
{
    if (encoding == 10)
        encoding = 1;
    return radio->setEncoding(encoding);
}

template<>
int16_t RadioHal<LR2021>::setWhitening(bool enabled, uint16_t initial)
{
    return radio->setWhitening(enabled, initial);
}

template<>
int16_t RadioHal<LR2021>::invertIQ(bool enable)
{
    return radio->invertIQ(enable);
}

template<>
int16_t RadioHal<LR2021>::explicitHeader()
{
    return radio->explicitHeader();
}

template<>
int16_t RadioHal<LR2021>::implicitHeader(size_t len)
{
    return radio->implicitHeader(len);
}

template<>
int16_t RadioHal<LR2021>::setRxBoostedGainMode(bool enable)
{
    return radio->setRxBoostedGainMode(enable ? 7 : 0);
}