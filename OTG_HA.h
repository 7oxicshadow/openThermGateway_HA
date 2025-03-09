#ifndef OTG_HA_H
#define OTG_HA_H

extern WiFiClient   espClient;
extern bool wifiResetComplete;
extern float boilerTemperature;
extern float boilerRetTemperature;
extern float modulation;
extern bool  flameState;
extern float targetSetpoint;
extern float roomSetpoint;
extern float roomTemperature;
extern float waterPressure;
extern float dhwSetpoint;
extern float dhwTemperature;
extern float outTemperature;
extern unsigned long timeBurnerActiveMins;

#endif //OTG_HA_H