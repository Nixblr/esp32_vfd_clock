#ifndef WIFISTA_H
#define WIFISTA_H

void wifi_init_sta(void);
void wifiStaChangeAP(const char *ssid, const char *pass);
void wifiStaGetAP(char *ssid, char *pass);

#endif