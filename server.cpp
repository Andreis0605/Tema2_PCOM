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

// function used to add a client to the clients lists, or reconnect a client
// returns 1 on a new connection or reconnection
// returns 0 on a connection with the same id
int process_connection(char *id, int client_socket, vector<tcp_client> clients)
{
    for (auto &client : clients)
    {
        if (strcmp(id, client.id) == 0)
        {
            // reconnection atempt
            if (client.conected == false)
            {
                client.conected = true;
                client.client_socket = client_socket;
                return 1;
            }
            else
                return 0;
        }
    }

    // we got a new client

    // create the client entry
    tcp_client aux;
    aux.client_socket = client_socket;
    strcpy(aux.id, id);
    aux.conected = true;

    // add it to the list of clients
    clients.push_back(aux);
    return 1;
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
    char *udp_buff = (char *)calloc(1800, sizeof(char));
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
    char *tcp_buff = (char *)calloc(1800, sizeof(char));
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

                break;
            }
        }
        else if (fds[1].revents & POLLIN)
        {
            // TODO: treat the udp message
            ;
        }
        else if (fds[2].revents & POLLIN)
        {
            // TODO: accept a new connection
            /*int new_client_socket = accept(fds[2].fd, (struct sockaddr *)&tcp_addr, &tcp_len);

            recv_all(new_client_socket, tcp_buff);

            // got the client, now add it to the list
            rc = process_connection(tcp_buff, new_client_socket, clients);
            if (rc == 1)
            {
                // new client message
                cout << "New client " << tcp_buff << " connected from " << inet_ntoa(tcp_addr.sin_addr) << ":" << ntohs(tcp_addr.sin_port) << ".";

                // TODO: add it to the fds
            }
            else
            {
                // already have that id
                cout << "Client " << tcp_buff << " already connected.";

                //prepare the message
                memcpy(tcp_buff,"disconnect!\0", 12);
                send_all(new_client_socket, tcp_buff);
            }*/
        }
        else
        {
            // TODO: Process the input from a client
            ;
        }
    }

    close(udp_socket);
    close(tcp_socket);

    return 0;
}