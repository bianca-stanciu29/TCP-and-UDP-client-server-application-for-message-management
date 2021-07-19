#ifndef _UTILS_H_
#define _UTILS_H_

#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <map>
#include <bits/stdc++.h>

//structura pentru un mesaj trimis catre tcp
struct send_msg{
    char IP_UDP[16];
    uint16_t udp_port;
    char topic[51];
    char type[11];
    char valoare_mesaj[2000];
};
// structura pentru un mesaj primit de la un client UDP
struct udp_msg{
    char topic[50];
    uint8_t type;
    char data[1501];
};
//structura pentru client
struct subscription{
    char id[11];
    int sf;
};

#endif