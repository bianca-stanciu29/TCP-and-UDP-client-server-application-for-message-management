 STANCIU BIANCA-ANDREEA, 325CC
                                    TEMA2_PROTOCOALE DE COMUNICATII
                       Aplicatie client-server TCP si UDP pentru gestionarea mesajelor

SERVER: -in cadrul serverului am realizat o legatura intre clientii din platforma(clientii UDP si clientii TCP).
        -am folosit: un unordered_set in care voi retine clientii actuali conectati la server(clienti_actuali)
                   : un unordered_map care imi va retine conexiune dintre socket si id-ul clientului(conex_socket_id)
                   : un unordered_map care imi va retine conexiune dintre id si socket(conex_id_socket)
                   : o structura de abonati formata din {id,sf}
                   : o structura pentru mesajul_udp_primit(udp_msg *mesaj_udp) formata din {topic, type, data}
                   : o structura pentru mesajul udp care va fi trimis catre tcp format din {id_udp, port, topic, type,
                   valoare_mesaj}
                   : un unordered_map care imi va retine topicurile si un vector de abonati corespunzatori topicului
                    unordered_map<string, vector<subscription>> canal;

        -am urmat pasii din cadrul laboratorului si anume: am creat socket-urile(sock_udp, sock_tcp), am completat
        adresa serverelor la care ma conectez pentru bind, familia de adrese si portul rezervat serverului, am setat
        descriptorii socketilor.

        -daca se primeste mesaj de la un client udp: *completez ip-ul si portul pentru mesajul care trebuie trimis
                                                     *pentru a converti restul datelor conform cerintei am creat functia
                                                     format_mesaj care are drept argumente structura mesajului udp si
                                                     structura mesajului pe care clientul tcp. In functie de type se va
                                                     alege formatul : INT, SHORT_REAL, FLOAT, STRING.

        Functia format_mesaj : -se copiaza topicul in structura care se va trimite si am adaugat terminatorul de sir
                                -daca type = 0  primul octet este octetul de semn si in functie de acesta se va inmulti
                  cu -1 sau nu, si am aplicat ntohl pentru conversia din network byte order in intreg pentru numarul pe
                  care o sa il pun in valoarea_mesajului pe care o sa il trimit, iar in tipul mesajului =  "INT"
                                -daca type = 1 am folosit ntohs pentru conversie din network byte order in numar real,
                  am calculat modulul impartind la 100 numarul si am atribuit rezultatul valorii_mesajului cu 2
                  zecimale, iar tipul mesajului = "SHORT_REAL"
                                -daca type = 2 am folosit ntohl si am obtinut modulul impartind numarul la puterea
                  negativa a lui 10, in functie de octetul de semn se va inmulti cu -1 sau nu, iar tipul mesajului =
                  "FLOAT"
                                -daca type = 3 tipul mesajului = "STRING", iar in valorea mesajului se va copia stringul
                  trimis de udp.
                                                *caut topicul in map-ul canal si voi parcurge vectorul de abonati la
                                                acel topic pentru a trimite mesajul dorit clientilor abonati

        -daca se primeste un mesaj de la tastatura verific daca s-a primit comanda exit, iar in caz afirmativ voi
        parcurge toate socketurile tcp si le voi inchide, voi inchide si sochetul tcp si udp.

        -daca a venit o cerere de conexiune de pe un socket inactiv, socketul o accepta, voi cauta id-ul in lista de
        clienti actuali conectati la server. Daca id-ul exista in lista de clienti actuali atunci voi trimite comanda
        de exit catre socketul respectiv si voi inchide socketul. Altfel daca se conecteaza un client noi voi actualiza
        map-urile de conex_id_sock si conex_sock_id, dezactivez algoritmul lui Nagle pentru conexiunea la clientul TCP
        si voi printa mesajul corespunzator conectarii unui nou client

        -daca a venit o cerere de pe unul din socketii de client, serverul le va receptiona. Daca conexiunea s-a inchis,
        din clientii actuali se va sterge clientul care s-a deconectat si voi afisa mesajul corespunzator, voi elimina
        id-ul din lista id-socket si voi scoate din multimea de citire socketul inchis. Daca s-a primit o comanda de
        subscribe in structura de abonati voi aduga id-ul si sf-ul. Voi parcurge lista de topicuri si daca acelasi
        client s-a abonat la acelasi canal dar cu alt sf atunci se actualizeaza sf-ul, altfel daca se aboneaza clientul
        la alt topic il voi adauga in map-ul de canale. Daca s-a primit o comanda de unsubscribe voi cauta topicul si
        voi parcurge lista de abonati, eleminand id-ul clientului.


Subscriber: -am creat socketul tcp pentru conectarea la server, am completat adresa serverului, familia de adrese si
            portul pentru conectare.
            -am creat conexiunea catre server, am trimis date catre server si am setat file descriptorii socketilor
            pentru server si pentru stdin

            -daca se primeste un mesaj de la server se verifica daca s-a primit intreg mesajul si daca nu s-a primit se
            trimit cereri de request pana se va primi intreg mesajul, urmand sa fie printat in formatul corespunator.
            -daca se primeste mesaj de la tastatura se verifica daca este comanda de exit si se va inchide socketul,
             altfel se verifica tipul mesajului primit: daca este de subscribe sau unsubscribe. pentru comanda data am
             calculat cate cuvinte contine. Daca s-a primit subscribe si sunt 3 cuvinte si sf-ul este 1 sau 0 atunci
             se printeaza mesajul corespunzator. Analog pentru unsubscribe, se verifica daca sunt 2 cuvinte si se
             printeaza mesajul.
             -la final se inchide socketul.
