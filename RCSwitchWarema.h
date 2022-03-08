#pragma once

#include <RCSwitch.h>

class RCSwitchWarema : public RCSwitch
{
  using Base=RCSwitch;
  
  public:
    void sendMC(char* Code,int dLen,int sLen,int repeat,int delay);
    void enableTransmit(int nTransmitterPin);
  
  private:
    int m_transmitterPin{};
};


void RCSwitchWarema::enableTransmit(int nTransmitterPin)
{
  m_transmitterPin = nTransmitterPin;
  
  Base::enableTransmit(nTransmitterPin);
}

void RCSwitchWarema::sendMC(char* sCodeWord, int dataLength, int syncLength, int sendCommand, int sendDelay  )
{

  for (int nRepeat=0; nRepeat<sendCommand; nRepeat++)
  {
    digitalWrite(this->m_transmitterPin, LOW);  
    
    for (int i = 0; i < strlen(sCodeWord); i++) {
        switch (sCodeWord[i]) {
          case 's':
            digitalWrite(this->m_transmitterPin, LOW);
            delayMicroseconds(syncLength);
            break;

          case 'S':
            digitalWrite(this->m_transmitterPin, HIGH);
            delayMicroseconds(syncLength);
            break;

          case '0':
            digitalWrite(this->m_transmitterPin, HIGH);
            delayMicroseconds(dataLength/2);
            digitalWrite(this->m_transmitterPin, LOW);
            delayMicroseconds(dataLength/2);
            break;

          case '1':
            digitalWrite(this->m_transmitterPin, LOW);
            delayMicroseconds(dataLength/2);
            digitalWrite(this->m_transmitterPin, HIGH);
            delayMicroseconds(dataLength/2);
            break;
        }
      }
      digitalWrite(this->m_transmitterPin, LOW);    
      if (nRepeat != sendCommand-1)
      delayMicroseconds(sendDelay);
  }
}
