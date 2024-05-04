#include <bits/stdc++.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/poll.h>

#include "my_io.hpp"

using namespace std;

// structure that represents a tcp client
typedef struct TCP_CLIENT
{
    char id[11];
    vector<string> topics;
    int client_socket;
    bool conected;
} tcp_client;

// aux function to print the details of clients
void print_clients(vector<tcp_client> clients)
{
    for (auto &client : clients)
    {
        cout << "Client ID: " << client.id << '\n';
        cout << "Client socket: " << client.client_socket << '\n';
        cout << "Topics: \n";
        for (auto &topic : client.topics)
        {
            cout << topic << '\n';
        }
        cout << client.conected << "\n\n";
    }
}
// function used to add a client to the clients lists, or reconnect a client
// returns 1 on a new connection or reconnection
// returns 0 on a connection with the same id
int process_connection(char *id, int client_socket, vector<tcp_client> &clients)
{
    for (auto &client : clients)
    {
        if (strcmp(id, client.id) == 0)
        {
            // reconnection atempt
            if (client.conected == false)
            {
                // cout << "old client\n";
                client.conected = true;
                client.client_socket = client_socket;
                return 1;
            }
            else
                return 0;
        }
    }

    // we got a new client

    // cout << "new client\n";
    //  create the client entry
    tcp_client aux;
    aux.client_socket = client_socket;
    strcpy(aux.id, id);
    aux.conected = true;

    // add it to the list of clients
    clients.push_back(aux);
    // cout << clients.size() << '\n';
    return 1;
}

// function that subscribes a new client to a topic
// the match is based on the current socket of the client
// returns 1 on succes or 0 if the client is already subscribed
int subscribe_client_to_topic(int client_socket, char *topic, vector<tcp_client> &clients)
{
    string aux(topic);

    for (auto &client : clients)
    {
        if (client.client_socket == client_socket)
        {
            bool ok = true;
            for (auto &topic : client.topics)
            {
                if (topic == aux)
                {
                    ok = false;
                    return 0;
                }
            }
            if (ok)
            {
                client.topics.push_back(aux);
                return 1;
            }
        }
    }
    return 0;
}

// function that unsubscribes a new client to a topic
// the match is based on the current socket of the client
// returns 1 on succes or 0 if the client is already subscribed
int unsubscribe_client_to_topic(int client_socket, char *topic, vector<tcp_client> &clients)
{
    string aux(topic);

    for (auto &client : clients)
    {
        if (client.client_socket == client_socket)
        {
            long int to_delete;
            bool found = false;
            for (long unsigned int i = 0; i < client.topics.size(); i++)
            {
                if (client.topics[i] == aux)
                {
                    to_delete = i;
                    found = true;
                }
            }
            if (found)
            {
                client.topics.erase(client.topics.begin() + to_delete);
                return 1;
            }
        }
    }
    return 0;
}

// function tahat marks a client as disconnected from the server
//  returns 1 on succes or 0 on error
int disconnect_client(int client_socket, vector<tcp_client> &clients)
{
    for (auto &client : clients)
    {
        if (client.client_socket == client_socket)
        {
            client.conected = false;
            cout << "Client " << client.id << " disconnected.\n";
            return 1;
        }
    }
    return 0;
}

// TODO: function that matches two topics
// includes the wildcard mechanic

bool is_topic_match(const std::string topic, const std::string subscription)
{

    std::string regexSubscription = subscription;
    std::regex_replace(regexSubscription, std::regex("\\*"), ".*");
    std::regex_replace(regexSubscription, std::regex("\\+"), "[^/]*");

    std::string regexPattern = "^" + regexSubscription + "$";

    std::regex regex(regexPattern);

    return std::regex_match(topic, regex);
}

// TODO: function that sends the messages to the clients
void send_to_clients(char *topic, char *message, vector<tcp_client> clients)
{
    string aux(topic);

    for (auto &client : clients)
    {
        if (client.conected == true)
        {
            bool sent = false;
            for (long unsigned int i = 0; i < client.topics.size(); i++ && !sent)
            {
                if (is_topic_match(client.topics[i], aux))
                {
                    // cout << "Found match" << '\n';
                    send_all(client.client_socket, message);
                    sent = true;
                }
            }
        }
    }
}

int main(int argc, char **argv)
{

    vector<tcp_client> clients;

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int rc;

    // get the port
    int port = atoi(argv[1]);

    // set the server ip
    struct sockaddr_in serv_addr, tcp_addr, udp_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    socklen_t tcp_len = sizeof(tcp_addr);
    socklen_t udp_len = sizeof(sizeof(udp_addr));

    // open the 2 sockets for UDP and TCP

    // open the udp socket and make a buffer for messages
    char *udp_buff = (char *)calloc(2000, sizeof(char));
    int udp_socket;
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("udp socket creation failed");
        exit(0);
    }

    // make the port reusable
    int enable = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) for udp socket failed");
        exit(0);
    }

    rc = bind(udp_socket, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (rc < 0)
    {
        perror("udp bind fail");
        exit(0);
    }

    // open the tcp socket for receiving data from clients and make a buffer for the messages
    char *tcp_buff = (char *)calloc(2000, sizeof(char));
    int tcp_socket;
    if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("tcp socket creation failed");
        exit(0);
    }

    // make the tcp socket reusable
    if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) for TCP socket failed");
        exit(0);
    }

    rc = bind(tcp_socket, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (rc < 0)
    {
        perror("tcp bind fail");
        exit(0);
    }

    if ((listen(tcp_socket, 10)) != 0)
    {
        perror("tcp listen failed");
        exit(0);
    }

    // disable the Nagle algorithm
    int flag = 1;
    if (setsockopt(tcp_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0)
    {
        perror("nagle algorith disable failed");
        exit(0);
    }

    // create the poll structure
    vector<struct pollfd> fds(3);

    // this is the standart input
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    // this is the udp entry
    fds[1].fd = udp_socket;
    fds[1].events = POLLIN;

    // this is the tcp entry for receiving conexions
    fds[2].fd = tcp_socket;
    fds[2].events = POLLIN;

    // start processing the messages
    while (true)
    {
        poll(fds.data(), fds.size(), -1);
        // chech for standard input
        if (fds[0].revents & POLLIN)
        {
            // read what was put to the stdin
            char buffer[200];
            cin.getline(buffer, 200);

            if (strstr(buffer, "exit"))
            {
                // prepare the message
                memcpy(tcp_buff, "disconnect!\0", 12);
                // send the message to all the clients to close
                for (long unsigned int i = 3; i < fds.size(); i++)
                {
                    // TODO: send the message to close the client
                    send_all(fds[i].fd, tcp_buff);
                    // TODO: close the socket in the tcp_clients list
                    close(fds[i].fd);
                }
                memset(tcp_buff, 0, 12);
                break;
            }
        }
        else if (fds[1].revents & POLLIN)
        {
            // TODO: treat the udp message
            // cout << "mesaj udp primit\n";
            memset(udp_buff, 0, 2000);
            rc = recvfrom(udp_socket, udp_buff, 2000, MSG_DONTWAIT, (struct sockaddr *)&udp_addr, &udp_len);
            // cout << rc << ' ' << strlen(udp_buff) << '\n';

            // TODO: extract the parts of the message
            // get the topic
            char topic[51];
            memcpy(topic, udp_buff, 50);
            unsigned char data_type;
            data_type = udp_buff[50];
            // cout << (int)data_type << '\n';

            // TODO: create the message for the clients
            rc = 0; // will keep the length of the buffer at any point.
            memset(tcp_buff, 0, 2000);

            // get he ip
            inet_ntop(AF_INET, &(udp_addr.sin_addr), tcp_buff, INET_ADDRSTRLEN);
            rc += strlen(tcp_buff);
            memcpy(tcp_buff + rc, ":\0", 2);
            rc++;

            // get the port
            char port_str[6];
            sprintf(port_str, "%d", ntohs(udp_addr.sin_port));
            memcpy(tcp_buff + rc, port_str, strlen(port_str));
            rc += strlen(port_str);
            memcpy(tcp_buff + rc, " - \0", 4);
            rc += 3;

            // copy the topic
            memcpy(tcp_buff + rc, topic, strlen(topic) + 1);
            rc += strlen(topic);
            memcpy(tcp_buff + rc, " - \0", 4);
            rc += 3;

            // copy the data type and the data itself
            if ((int)data_type == 0)
            {
                memcpy(tcp_buff + rc, "INT\0", 4);
                rc += 3;
                memcpy(tcp_buff + rc, " - \0", 4);
                rc += 3;

                // get the data, convert it to string, append it
                unsigned char sign = udp_buff[51];
                if (sign == 1)
                {
                    memcpy(tcp_buff + rc, "-\0", 2);
                    rc++;
                }

                unsigned char aux_buff[4] = {(unsigned char)udp_buff[52], (unsigned char)udp_buff[53], (unsigned char)udp_buff[54], (unsigned char)udp_buff[55]};
                uint32_t number = (aux_buff[0] << 24) | (aux_buff[1] << 16) | (aux_buff[2] << 8) | aux_buff[3];
                rc += snprintf(tcp_buff + rc, 2000, "%u", number);
                tcp_buff[rc] = '\0';
            }

            if ((int)data_type == 1)
            {
                memcpy(tcp_buff + rc, "SHORT_REAL\0", 11);
                rc += 10;
                memcpy(tcp_buff + rc, " - \0", 4);
                rc += 3;

                // get the data, convert it to string, append it
                uint16_t value = ((unsigned char)udp_buff[51] << 8) | (unsigned char)udp_buff[52];
                // cout << value << "\n";
                float float_value = (float)value / 100.0f;
                // cout << float_value << '\n';

                rc += snprintf(tcp_buff + rc, 2000, "%.2f", float_value);
                tcp_buff[rc] = '\0';
            }

            if ((int)data_type == 2)
            {
                memcpy(tcp_buff + rc, "FLOAT\0", 6);
                rc += 5;
                memcpy(tcp_buff + rc, " - \0", 4);
                rc += 3;

                // get the data, convert it to string, append it
                uint8_t sign = (unsigned char)udp_buff[51];
                uint32_t number = ((unsigned char)udp_buff[52] << 24) | ((unsigned char)udp_buff[53] << 16) | ((unsigned char)udp_buff[54] << 8) | (unsigned char)udp_buff[55];
                uint8_t power = (unsigned char)udp_buff[56];

                if (sign == 1)
                {
                    memcpy(tcp_buff + rc, "-\0", 2);
                    rc++;
                }

                double value = (double)number;
                for (int i = 0; i < power; i++)
                {
                    value /= (double)10.0;
                }

                rc += snprintf(tcp_buff + rc, 2000, "%.15g", value);
                tcp_buff[rc] = '\0';
            }

            if ((int)data_type == 3)
            {
                memcpy(tcp_buff + rc, "STRING\0", 7);
                rc += 6;
                memcpy(tcp_buff + rc, " - \0", 4);
                rc += 3;

                strncat(tcp_buff, udp_buff + 51, 1500);
                rc += strnlen(udp_buff + 51, 1500);
            }

            memcpy(tcp_buff + rc, "\n\0", 2);
            rc++;

            // cout << tcp_buff << ' ' << rc << '\n';
            //  TODO: Send the message to the tcp clients suscribe to the topic
            send_to_clients(topic, tcp_buff, clients);
            //  cout << "Trec peste mesaj";
            memset(tcp_buff, 0, 2000);
            memset(udp_buff, 0, 2000);
        }
        else if (fds[2].revents & POLLIN)
        {
            // TODO: accept a new connection
            int new_client_socket = accept(fds[2].fd, (struct sockaddr *)&tcp_addr, &tcp_len);

            recv_all(new_client_socket, tcp_buff);

            // got the client, now add it to the list
            rc = process_connection(tcp_buff, new_client_socket, clients);
            // cout << clients.size() << '\n';
            if (rc == 1)
            {
                // new client message
                cout << "New client " << tcp_buff << " connected from " << inet_ntoa(tcp_addr.sin_addr) << ":" << ntohs(tcp_addr.sin_port) << ".\n";
                // TODO: add it to the fds
                struct pollfd aux;
                aux.fd = new_client_socket;
                aux.events = POLLIN;
                fds.push_back(aux);
            }
            else
            {
                // already have that id
                cout << "Client " << tcp_buff << " already connected.\n";

                // prepare the message
                memcpy(tcp_buff, "disconnect!\0", 12);
                send_all(new_client_socket, tcp_buff);
            }
            memset(tcp_buff, 0, 1800);
            // print_clients(clients);
        }
        else
        {
            for (long unsigned int i = 3; i < fds.size(); i++)
            {
                if (fds[i].revents & POLLIN)
                {
                    recv_all(fds[i].fd, tcp_buff);
                    if (strstr(tcp_buff, "unsubscribe"))
                    {
                        // cout << "SUBSCRIBE REQUEST\n";
                        rc = unsubscribe_client_to_topic(fds[i].fd, tcp_buff + 12, clients);
                        if (rc == 1)
                        {
                            // cout << "ok done";
                            memcpy(tcp_buff, "ok\0", 3);
                            send_all(fds[i].fd, tcp_buff);
                            // print_clients(clients);
                        }
                        else
                        {
                            // cout << "not ok";
                            memcpy(tcp_buff, "no\0", 3);
                            send_all(fds[i].fd, tcp_buff);
                        }
                        continue;
                    }
                    if (strstr(tcp_buff, "subscribe"))
                    {
                        // cout << "SUBSCRIBE REQUEST\n";
                        rc = subscribe_client_to_topic(fds[i].fd, tcp_buff + 10, clients);
                        if (rc == 1)
                        {
                            // cout << "ok done";
                            memcpy(tcp_buff, "ok\0", 3);
                            send_all(fds[i].fd, tcp_buff);
                            // print_clients(clients);
                        }
                        else
                        {
                            // cout << "not ok";
                            memcpy(tcp_buff, "no\0", 3);
                            send_all(fds[i].fd, tcp_buff);
                        }
                    }
                    if (strstr(tcp_buff, "want to disconnect"))
                    {
                        // disconnect a client from the server
                        int aux_socket = fds[i].fd;
                        rc = disconnect_client(fds[i].fd, clients);
                        if (rc == 1)
                        {
                            // cout << "client dis\n";
                            memcpy(tcp_buff, "ok\0", 3);
                            send_all(fds[i].fd, tcp_buff);
                        }
                        fds.erase(fds.begin() + i);
                        close(aux_socket);
                    }
                    memset(tcp_buff, 0, 1800);
                }
            }
        }
    }

    close(udp_socket);
    close(tcp_socket);

    return 0;
}