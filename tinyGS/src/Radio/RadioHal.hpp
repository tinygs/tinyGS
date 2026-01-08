#include "Arduino.h"
#include <RadioLib.h>
#ifndef RADIOHAL_HPP
#define RADIOHAL_HPP


class IRadioHal {
public:
  virtual int16_t begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain, float tcxoVoltage) = 0;
  virtual int16_t begin() = 0;
  virtual int16_t beginFSK(float freq = 434.0, float br = 48.0, float freqDev = 50.0, float rxBw = 125.0, int8_t power = 10, uint16_t preambleLength = 16, bool enableOOK = false, float tcxoVoltage = 1.6, bool useRegulatorLDO = false) = 0;
  virtual int16_t autoLDRO() = 0;
  virtual int16_t forceLDRO(bool enable) = 0;
  virtual int16_t setCRC(uint8_t len,	uint16_t initial = 0x1D0F, uint16_t polynomial = 0x1021, bool inverted = true ) = 0;
  virtual int16_t setDataShaping(uint8_t sh) = 0;
  virtual void setPacketReceivedAction(void (*func)(void)) = 0;
  virtual void clearPacketReceivedAction() = 0;
  virtual int16_t startReceive() = 0;
  virtual int16_t transmit(uint8_t* data, size_t len, uint8_t addr = 0) = 0;
  virtual int16_t sleep() = 0;
  virtual size_t getPacketLength(bool update = true) = 0;
  virtual int16_t readData(uint8_t* data, size_t len) = 0;
  virtual float getRSSI(bool packet = true, bool skipReceive = false) = 0;
  virtual float getSNR() = 0;
  virtual float getFrequencyError(bool autoCorrect = false) = 0;
  virtual int16_t fixedPacketLengthMode(uint8_t len) = 0;
  virtual int16_t setSyncWord(uint8_t* syncWord, uint8_t len) = 0;
  virtual int16_t setFrequency(float freq) = 0;
  virtual int16_t setEncoding(uint8_t encoding) = 0;
  virtual void setRfSwitchPins(uint8_t rxEnPin, uint8_t txEnPin) = 0;
  virtual int16_t setWhitening(bool enabled, uint16_t initial) = 0;  
  virtual int16_t invertIQ (bool enable) = 0;
  virtual int16_t explicitHeader() = 0;
  virtual int16_t implicitHeader(size_t len) = 0;
};


template <typename T>
class RadioHal : public IRadioHal {
public:
  RadioHal(Module* mod)
  {
    radio = new T(mod);
  }

  int16_t begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, uint8_t gain, float tcxoVoltage);
  int16_t begin();
  int16_t beginFSK(float freq = 434.0, float br = 48.0, float freqDev = 50.0, float rxBw = 125.0, int8_t power = 10, uint16_t preambleLength = 16, bool enableOOK = false, float tcxoVoltage = 1.6, bool useRegulatorLDO = false);

  int16_t autoLDRO();

  int16_t forceLDRO(bool enable);
  int16_t invertIQ(bool enable);
  int16_t explicitHeader(); 
  int16_t implicitHeader(size_t len);

  int16_t setCRC(uint8_t len,	uint16_t initial = 0x1D0F, uint16_t polynomial = 0x1021, bool inverted = true );
  
  int16_t setDataShaping(uint8_t sh)
  {
    return radio->setDataShaping(sh);
  }

  void setPacketReceivedAction(void (*func)(void));
  
  void clearPacketReceivedAction()
  {
    return radio->clearPacketReceivedAction();
  }
 
  int16_t startReceive();

  int16_t transmit(uint8_t* data, size_t len, uint8_t addr = 0)
  {
    return radio->transmit(data, len, addr);
  }

  int16_t sleep()
  {
    return radio->sleep();
  }

  size_t getPacketLength(bool update = true)
  {
    return radio->getPacketLength(update);
  }

  int16_t readData(uint8_t* data, size_t len)
  {
    return radio->readData(data, len);
  }

  float getRSSI(bool packet = true, bool skipReceive = false);


  float getSNR()
  {
    return radio->getSNR();
  }

  float getFrequencyError(bool autoCorrect = false);

  int16_t fixedPacketLengthMode(uint8_t len);

  int16_t setSyncWord(uint8_t* syncWord, uint8_t len)
  {
    return radio->setSyncWord(syncWord, len);
  }
  
  int16_t setFrequency(float freq) 
  {
    return radio->setFrequency(freq);
  }

  int16_t setEncoding(uint8_t encoding);

  void setRfSwitchPins(uint8_t rxEnPin, uint8_t txEnPin)
  {
    radio->setRfSwitchPins(rxEnPin, txEnPin);
  }

  int16_t setWhitening(bool enabled, uint16_t initial);

private:
  T* radio;
};


#endif
