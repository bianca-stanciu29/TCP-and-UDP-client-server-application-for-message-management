#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <string.h>
#include <netinet/tcp.h>
#include <map>
#include <bits/stdc++.h>
#define BUFLEN (sizeof(send_msg)) // dimensiunea maxima de date
#include "help_tcp.h"
#include "helpers.h"
using namespace std;
//functie care primeste un mesaj_udp si completeaza mesajul care trebuie trimis tcp-ului
void format_mesaj(udp_msg *buff_udp, send_msg *trimis)
{ //se copiaza topicul in structura care se va trimite
    strncpy(trimis->topic, buff_udp->topic, 50);
    //se adauga terminatorul de sir
    trimis->topic[50] = '\0';
    int num;
    double num_real;
    //se evalueaza cazurile:INT, SHORT_REAL, FLOAT, STRING
    if (buff_udp->type == 0)
    {
        //Conversie din network byte order in intreg
        num = ntohl(*(uint32_t *)(buff_udp->data + 1));
        //atribuirea tipului
        strcpy(trimis->type, "INT");
        //se verifica semnul numarului
        if (buff_udp->data[0])
        {
            num *= -1;
        }
        //atribuirea valorii
        sprintf(trimis->valoare_mesaj, "%d", num);
    }
    else if (buff_udp->type == 1)
    {
        //conversie din network byte order in numar real
        num_real = ntohs(*(uint16_t *)(buff_udp->data));
        //atribuirea tipului
        strcpy(trimis->type, "SHORT_REAL");
        //se calculeaza modului numarului impartit la 100
        num_real /= 100;
        //atribuirea valorii
        sprintf(trimis->valoare_mesaj, "%.2f", num_real);
    }
    else if (buff_udp->type == 2)
    {
        //Conversie din network byte order in intreg
        num_real = ntohl(*(uint32_t *)(buff_udp->data + 1));
        //atribuirea tipului
        strcpy(trimis->type, "FLOAT");
        //se obtine modului impartind numarul la puterea negativa a lui 10
        num_real /= pow(10, buff_udp->data[5]);
        //se verifica semnul numarului
        if (buff_udp->data[0] == 1)
        {
            num_real *= -1;
        }
        //atribuirea valorii
        sprintf(trimis->valoare_mesaj, "%lf", num_real);
    }
    else if (buff_udp->type == 3)
    {
        //atribuirea tipului
        strcpy(trimis->type, "STRING");
        //atribuirea valorii
        strcpy(trimis->valoare_mesaj, buff_udp->data);
    }
}
int main(int argc, char *argv[])
{
    //se dezactiveaza buffering-ul la afisare
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    char buffer[BUFLEN];
    int sock_tcp, new_sock, port_no, sock_udp;
    struct sockaddr_in serv_addr;
    int n, i, ret;
    socklen_t client, clilen_tcp;
    int flag = 1;
    //set pentru a retine clientii cu id unic
    unordered_set<string> clienti_actuali;
    //map-uri pentru a retine conexiune socket-id si id-socket
    unordered_map<int, string> conex_socket_id;
    unordered_map<string, int> conex_id_socket;
    //structura de abonati
    subscription abonati;
    //structura mesajului udp primit
    udp_msg *mesaj_udp;
    //structura mesajului udp trimis catre tcp
    send_msg mesaj_trimis;
    //map in care retin topicul si lista de abonati la topicul respectiv
    unordered_map<string, vector<subscription>> canal;
    //se creaza socketul UDP
    sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sock_udp < 0, "socket_udp");
    //se creaza socketul TCP
    sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sock_tcp < 0, "socket_tcp");

    //se atribuie numarul portului
    port_no = atoi(argv[1]);

    /*Setare struct sockaddr_in pentru a asculta pe portul respectiv pentru UDP*/
    struct sockaddr_in addr_udp;
    socklen_t socklen = sizeof(addr_udp);
    struct sockaddr_in addr_tcp;
    clilen_tcp = sizeof(addr_tcp);
    addr_udp.sin_family = AF_INET;
    addr_udp.sin_port = htons(port_no);
    addr_udp.sin_addr.s_addr = INADDR_ANY;

    // Completare in addr_tcp adresa serverului pentru bind, familia de adrese si portul rezervat pentru server
    memset((char *)&addr_tcp, 0, sizeof(addr_tcp));
    addr_tcp.sin_family = AF_INET;
    addr_tcp.sin_port = htons(port_no);
    addr_tcp.sin_addr.s_addr = INADDR_ANY;

    // se asociaza socketul UDP cu portul ales
    int b_udp = bind(sock_udp, (struct sockaddr *)&addr_udp, sizeof(addr_udp));
    DIE(b_udp < 0, "bind udp");

    // se asociaza socketul UDP cu portul ales
    int b_tcp = bind(sock_tcp, (struct sockaddr *)&addr_tcp, sizeof(struct sockaddr_in));
    DIE(b_tcp < 0, "bind tcp");

    DIE(setsockopt(sock_tcp, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0, "turn off");
    //se face socketul pasiv pentru a primi cereri de conexiune de la client
    int listen_tcp = -1;
    ret = listen(sock_tcp, INT_MAX);

    //Setarea descriptorilor socketilor
    fd_set tmp_fds, read_fds;
    int fdmax;
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    FD_SET(sock_tcp, &read_fds);
    FD_SET(sock_udp, &read_fds);
    FD_SET(0, &read_fds);
    fdmax = sock_tcp;
    char id[256];

    while (1)
    {
        tmp_fds = read_fds;
        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        for (i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &tmp_fds))
            {
                if (i == sock_udp)
                {
                    //se primeste mesaj de la clientul udp
                    int r = recvfrom(sock_udp, buffer, sizeof(udp_msg), 0, (struct sockaddr *)&addr_udp, &socklen);
                    DIE(r < 0, "received udp");
                    //se completeaza ip si portul pentru mesajul care trebuie trimis
                    strcpy(mesaj_trimis.IP_UDP, inet_ntoa(addr_udp.sin_addr));
                    mesaj_trimis.udp_port = ntohs(addr_udp.sin_port);
                    mesaj_udp = (udp_msg *)(buffer);
                    string s = mesaj_udp->topic;
                    //se apeleaza functia de completare a restului de date
                    format_mesaj(mesaj_udp, &mesaj_trimis);
                    //se gaseste topicul la care vreau sa trimit
                    if (canal.find(s) != canal.end())
                    {
                        //parcurg verctorul de clienti de la topicul gasit
                        for (subscription j : canal[s])
                        {
                            string aux = j.id;

                            //se gaseste socketul cu id-ul j
                            if (clienti_actuali.find(aux) != clienti_actuali.end())
                            {

                                int sk = conex_id_socket.find(aux)->second;
                                //se trimite mesajul clientului
                                mesaj_trimis.topic[50] = '\0';
                                DIE(send(sk, &mesaj_trimis, sizeof(send_msg), 0) < 0, "send");
                            }
                        }
                    }
                }
                else if (i == 0) //se primeste mesaj de la tastatura
                {
                    if (fgets(buffer, sizeof(buffer), stdin) != NULL)
                    {
                        if (strncmp(buffer, "exit", 4) == 0) //daca s-a  primit comanda exit
                        {
                            //se parcurg toate socketurile tcp si se inchid
                            for (int k = 0; k <= fdmax; k++)
                            {
                                if (k != sock_tcp && k != sock_udp && FD_ISSET(k, &read_fds))
                                {
                                    close(k);
                                }
                                //se inchid socketurile tcp si udp
                                close(sock_udp);
                                close(sock_tcp);
                                exit(0);
                            }
                            break;
                        }
                    }
                    else
                    {
                        continue;
                    }
                }
                else if (i == sock_tcp)
                {
                    // a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
                    // pe care serverul o accepta
                    client = sizeof(addr_tcp);
                    new_sock = accept(sock_tcp, (struct sockaddr *)&addr_tcp, &clilen_tcp);

                    DIE(new_sock < 0, "accept");
                    //se primeste id-ul
                    recv(new_sock, id, 256, 0);
                    //se cauta id-ul in lista de clienti actuali conectati la server
                    auto got = clienti_actuali.find(id);
                    //se verifica daca mai exista in set
                    if (got != clienti_actuali.end())
                    {

                        printf("Client %s already connected.\n", id);
                        //daca exista se va trimite catre client comanda de exit
                        strcpy(mesaj_trimis.valoare_mesaj, "exit\n");
                        //send(new_sock,&mesaj_trimis, sizeof(send_msg),0);
                        //se inchide socketul
                        close(new_sock);
                    }
                    else
                    {
                        FD_SET(new_sock, &read_fds);
                        //se insereaza in lista de clienti actuali noul client conectat
                        clienti_actuali.insert(id);

                        conex_socket_id[new_sock] = id; //legatura intre socket si id
                        conex_id_socket[id] = new_sock; //legare intre id si socket

                        // se dezactiveaza algoritmul lui Nagle pentru conexiunea la clientul TCP
                        setsockopt(new_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

                        // se adauga noul socket intors de accept() la multimea descriptorilor de citire

                        if (new_sock > fdmax)
                        {
                            fdmax = new_sock;
                        }
                        //se printeaza mesajul corespunzator conectarii unui nou client
                        printf("New client %s connected from %s:%hu.\n", id,
                               inet_ntoa(addr_tcp.sin_addr), ntohs(addr_tcp.sin_port));
                    }
                }
                else
                {
                    // s-au primit date pe unul din socketii de client,
                    // asa ca serverul trebuie sa le receptioneze
                    memset(buffer, 0, BUFLEN);

                    n = recv(i, buffer, sizeof(buffer), 0);
                    if (n == 0)
                    {

                        // conexiunea s-a inchis
                        //din clienti actuali se va sterge clientul care s-a deconectat
                        clienti_actuali.erase(clienti_actuali.find(conex_socket_id[i]));
                        //se afiseaza mesajul corespunzator
                        printf("Client %s disconnected.\n", const_cast<char *>(conex_socket_id[i].c_str()));
                        //se elimina id-ul si din lista de id-socket
                        conex_id_socket.erase(conex_socket_id[i]);
                        close(i);
                        // se scoate din multimea de citire socketul inchis
                        FD_CLR(i, &read_fds);
                    }
                    else
                    {
                        //daca s-a primit o comanda de subscribe sau unsubscribe
                        char topic[256];
                        char m_primit[256];
                        strcpy(m_primit, buffer);
                        char *token = strtok(m_primit, " ");
                        //daca s-a primit de subscribe
                        if (strncmp(buffer, "subscribe", 9) == 0)
                        {
                            //se adauga in structura de abonati id-ul
                            strcpy(abonati.id, const_cast<char *>(conex_socket_id[i].c_str()));

                            token = strtok(NULL, " ");
                            strcpy(topic, token);
                            abonati.sf = atoi(strtok(NULL, " "));
                            int exista = 0;
                            //se parcurge lista de tipicuri
                            //daca acelasi client s-a abonat la acelasi canal dar cu alt sf attunci
                            //se actualizeaza sf-ul
                            for (int j = 0; j < canal[topic].size(); j++)
                            {
                                string aux = canal[topic][j].id;

                                if (aux == conex_socket_id[i])
                                {
                                    exista = 1;
                                    canal[topic][j].sf = abonati.sf;
                                    break;
                                }
                            }
                            //daca se aboneaza un client la alt topic se adauga in map
                            if (exista == 0)
                            {
                                canal[topic].push_back(abonati);
                            }
                        }
                        //daca s-a primit comanda de unsubcsribe
                        else if (strncmp(buffer, "unsubscribe", 11) == 0)
                        {
                            token = strtok(NULL, " ");
                            strcpy(topic, token);
                            char mesaj_trimis[256] = "Unsubscribed from topic.";
                            //se cauta topicul de la care s-a dat unsubscribe
                            auto aux = canal.find(topic);
                            if (aux != canal.end())
                            {
                                //se parcurge vectorul de abonti
                                for (auto it = (*aux).second.begin(); it != (*aux).second.end(); ++it)
                                {
                                    //se cauta id-ul si se elimina clientul
                                    if (it->id == conex_socket_id[i])
                                    {

                                        it = (*aux).second.erase(it);

                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    close(sock_tcp);
    return 0;
}