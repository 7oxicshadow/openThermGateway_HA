#ifndef WIFIPROC_H
#define WIFIPROC_H

extern bool wifiInitComplete;
extern bool wifiResetComplete;

void wifiProcessing(void);
bool wifiConnectedStatus(void);

#endif //WIFIPROC_H