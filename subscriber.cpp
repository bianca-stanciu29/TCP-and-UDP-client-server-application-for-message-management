#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <map>
#include "help_tcp.h"
#include "helpers.h"
#define BUFLEN (sizeof(send_msg)) // dimensiunea maxima de date
using namespace std;

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    /*
    ID_CLIENT = sir de caractere care reprezinta ID-ul clientului
    IP_SERVER = adresa IPv4 a serverului
    PORT_SERVER = portul pe care serverul asteapta conexiune
    */
    if (argc < 3)
    {
        exit(0);
    }
    int sock_tcp, n;
    char ID_CLIENT[15];
    struct sockaddr_in serv_addr;
    char buffer[BUFLEN];
    send_msg *mesaj_primit;
    //extragere ID
    if (strlen(argv[1]) <= 10)
    {
        strcpy(ID_CLIENT, argv[1]);
    }

    // Crearea unui socket TCP pentru conectarea la server
    sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sock_tcp < 0, "open");

    //Completarea in serv_addr adresa serverului, familia de adrese si portul pentru conectare
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    inet_aton(argv[2], &serv_addr.sin_addr);

    //Crearea conexiune catre server
    int c = connect(sock_tcp, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(c < 0, "connect");

    //trimitere date catre server
    DIE(send(sock_tcp, argv[1], 256, 0) < 0, "ID SEND");

    // se seteaza file descriptorii socketilor pentru server si pentru stdin
    fd_set readFds;
    fd_set tmpFds;
    int fdMax;
    FD_ZERO(&readFds);
    FD_ZERO(&tmpFds);
    FD_SET(sock_tcp, &readFds);
    FD_SET(STDIN_FILENO, &readFds);
    fdMax = sock_tcp;

    while (1)
    {
        tmpFds = readFds;
        FD_SET(0, &tmpFds);
        FD_SET(sock_tcp, &tmpFds);
        int s = select(sock_tcp + 1, &tmpFds, NULL, NULL, NULL);
        DIE(s < 0, "select");

        for (int i = 0; i <= fdMax; i++)
        {
            if (FD_ISSET(i, &tmpFds))
            {
                if (i == sock_tcp)
                {
                    //se primeste mesaj de la sever
                    char buff[BUFLEN];
                    memset(buff, 0, sizeof(send_msg));
                    int primit = recv(sock_tcp, buff, sizeof(send_msg), 0);
                    DIE(primit < 0, "receive");

                    if (primit == 0)
                    {
                        // s-a inchis conexiunea cu clientul curent
                        close(sock_tcp);
                        return 0;
                    }
                    //se verifica daca s-a primit intreg mesajul
                    //daca nu s-a primit se trimit cereri de request pana se primeste intreg mesajul
                    int new_primit = primit;
                    while (new_primit < sizeof(send_msg))
                    {
                        primit = recv(sock_tcp, buff + new_primit, sizeof(send_msg) - new_primit, 0);
                        new_primit += primit;
                    }
                    //se printeaza mesajul conform tiparului impus
                    mesaj_primit = (send_msg *)buff;
                    mesaj_primit->topic[50] = '\0';
                    cout << mesaj_primit->IP_UDP << ":" << mesaj_primit->udp_port << " - " << 
                    mesaj_primit->topic << " - " << mesaj_primit->type << " - "
                         << mesaj_primit->valoare_mesaj << endl;
                }
                else if (i == 0)
                {
                    //s-a primit mesaj de la tastatura
                    memset(buffer, 0, BUFLEN);
                    fgets(buffer, BUFLEN - 1, stdin);

                    //daca mesajul primit este exit atunci se va inchide socketul
                    if (strncmp(buffer, "exit\n", 5) == 0)
                    {
                        close(sock_tcp);
                        return 0;
                    }

                    //daca se primeste o comanda diferita de exit
                    char comanda[2000];
                    strcpy(comanda, buffer);
                    //se extrage primul argument al comenzii
                    char *first_word = strtok(buffer, " ");
                    comanda[strlen(comanda) - 1] = '\0';

                    //se verifica tipul mesajului primit
                    if (strcmp(first_word, "subscribe") == 0 ||
                        strcmp(first_word, "unsubscribe") == 0)
                    {
                        //se calculeaza cate cuvinte contine comanda respectiva
                        int spaces = -1;
                        for (int j = 1; j < strlen(comanda); j++)
                        {
                            if (comanda[j] == ' ')
                            {
                                spaces++;
                            }
                        }
                        //daca comanda incepe cu "subcsribe" si contine 3 cuvinte
                        //se afiseaza mesajul corespunzator
                        if (strcmp(first_word, "subscribe") == 0)
                        {

                            if (spaces == 1)
                            {
                                n = send(sock_tcp, comanda, strlen(comanda), 0);
                                DIE(n < 0, "send");
                                char *second_word = strtok(NULL, " ");
                                char *third_word = strtok(NULL, " ");
                                //se verifica daca se primeste sf-ul corect
                                int val = atoi(third_word);
                                if (val == 1 || val == 0)
                                    cout << "Subscribed to topic.\n";
                            }
                        }
                        //daca comanda incepe cu "unsubscribe" si contine 2 cuvinte
                        //se afiseaza mesajul corespunzator
                        else if (strcmp(first_word, "unsubscribe") == 0)
                        {
                            if (spaces == 0)
                            {
                                n = send(sock_tcp, comanda, strlen(comanda), 0);
                                DIE(n < 0, "send");
                                cout << "Unsubscribed from topic.\n";
                            }
                        }
                    }
                    else
                    {
                        continue;
                    }
                }
            }
        }
    }
    //se inchide socketul
    close(sock_tcp);
    return 0;
}
