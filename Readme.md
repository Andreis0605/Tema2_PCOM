# Tema 2 - PCOM

## Serverul

### Functii si structuri de date folosite pentru implementarea serververului

 - `typedef struct TCP_CLIENT
{
    char id[11];
    vector<string> topics;
    int client_socket;
    bool conected;
} tcp_client;`

    > Aceasta este structura de date folosita in server pentru a retine date despre un anumit client cu un anumit id. Campul ID retine id-ul unui subscriber, vectorul de stringuri retine topicurile la care s-a abonat un client, client_socket retine pe ce socket se comunica cu acel client la un moment dat, iar variabila bool retine daca un client este conenctat sau nu la server la un moment de timp.

- `void print_clients(vector<tcp_client> clients)`

    > Functie auxiliara, folosita pentru debugging. Primeste lista de clienti si afiseaza toate detaliile legate de toti clientii care au fost conectati vreodata  la server

- `int process_connection(char *id, int client_socket, vector<tcp_client> &clients)`

    > Functie apelata in momentul in care un client doreste sa se conecteze la server. Aceasta verifica daca avem o conexiune valida(nu avem alt client cu acelasi ID conectat) si marcheaza clientul ca reconectat sau ca  un client nou si il adauga in lista de clienti.

- `int subscribe_client_to_topic(int client_socket, char *topic, vector<tcp_client> &clients)`

    > Functie care proceseaza cererea unui client de a se abona la un topic. Aceasta verifica daca acel client este deja abonat la topic. Daca da, intoarce eroare. Daca nu, il aboneaza si intoarce faptul ca abonarea a fost valida.

- `int unsubscribe_client_to_topic(int client_socket, char *topic, vector<tcp_client> &clients)`

    > Functie similara cu cea de mai sus, proceseaza o cerere de dezabonare de la un topic. Daca acel client nu era abonat la topicul de la care doreste sa se dezaboneza, intoarce eroare. Daca da, il dezaboneaza si intoarce faptul ca dezabonarea a fost valida.

- `int disconnect_client(int client_socket, vector<tcp_client> &clients)`

    > Functie care deconecteaza un client de la server. Aceasta cauta clientul dupa socketul folosit si il marcheaza ca fiind deconectat.

- `bool is_topic_match(string topic, string subscription)`

    > Functia care verifica daca doua topicuri sunt la fel, primul topic fiind primit de la un client UDP si al doile find un topic la care un client este abonat. Aceasta functie foloseste strtok pentru a imparti cele doua siruri si le parcurge token cu token pentru a verifica portivirea.

- `void send_to_clients(char *topic, char *message, vector<tcp_client> clients)`

    > Functia care decide carui client TCP sa i se trimita un mesaj UDP. Aceasta parcurge lista de clienti si pentru fiecare client conectat verifica daca este abonat la topic-ul din UDP. Daca da, ii trimite mesajul.

### Modul de functionare a serverului

> La pornire, serverul deschide doua socket-uri: unul UDP pe care va primi mesaje de la clientii UDP si unul TCP pe care va asculta dupa conexiuni ale clientilor TCP. Dupa aceea, vom folosi o structura de tip poll pentru multiplexarea intratilor si iesirilor serverului. Acestea vor fi:

- O intrare pentru mesajele primite de la tastatura, adica comanda exit care va duce la inchiderea tuturor clientilor conectati si a serverului.

- O intrare pentru socket-ul UDP. Cand serverul primeste un mesaj UDP, incepe procesarea acestuia, adica transformarea sa in plain text, pe care il trimite clientilor abonati la topicul mesajului respectiv.

- O intrare pentru socket-ul TCP pe care se asculta dupa conexiuni la server. Cand primim cerere de conectare asteptam ID-ul clientului. Daca e valid, marcam clientul ca fiind conectat. Daca nu, trimitem un mesaj clientului respectiv sa se opreasca.

- Cate o intrare pentru fiecare client conectat la un moment dat la server. Acestea se ocupa de cererile de abonare, dezabonare si inchidere ale clientilor.

## Clientul

### Modul de functionare a clientului

> La pornire, clientul deschide un socket TCP pe care va comunica cu serverul. Acesta incearca sa se conecteze la server si sa ii dea serverului id-ul sau. Daca acesta este valid, clientul poarte incepe comunicarea cu serverul. Si aici am pultiplexat IO-ul folosind o structura poll:

- O intrare este pentru citirea de la tastatura, Aceasta preia cererile de subscribe, unsubscribe si exit ale clientului(userului) si le trimite serververului. Daca s-a primit confirmarea de la server, se afiseaza in client(sau se inchide clientul in caz de exit).

- O intrare pentru socket-ul TCP folosit la comunicarea cu serverul. Aceasta se ocupa de primirea mesajelor venite de la clientii UDP si trecute prin server sau inchide clientul in momentul in care primeste aceasta comanda de la server.

## Protocolul de nivel aplicarie folosit.

> Serverul si clientii vor comunica intre ei prin mesaje plain text. Trecerea de la formatul mesajelor UDP la plain text este responsabilitate serverului. Pentru a asigura integritatea mesajelor, am definit urmatorul protocol: Trimit mai intai pe retea un mesaj care contine doar lungimea mesajului adevarat. Receptorul asteapta dupa aceasta dimensiune.(primul mesaj are o dimensiune exacta de 4 bytes). Dupa aceea in emitator ma asigur ca am trimis tot mesajul folosid o bucla pentru send. In receptor, folosesc o bucla pentru primirea a exact numarului de bytes transmis in primul mesaj.

### FUnctii folosite

> In fisierul my_io.cpp am definit cele doua functii(una de receive si una de send) care implementeaza comunicare folosind protocolul definit mai sus

- `void recv_all(int socket, char *message)`

    > Functia care implementeaza primirea mesajelor.

- `void send_all(int socket, char *message)`

    > Functia care implementeaza transmiterea mesajelor.