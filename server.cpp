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

// auxiliary function to print the details of clients
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
                client.conected = true;
                client.client_socket = client_socket;
                return 1;
            }
            else
                return 0;
        }
    }

    // we got a new client

    //  create the client entry
    tcp_client aux;
    aux.client_socket = client_socket;
    strcpy(aux.id, id);
    aux.conected = true;

    // add it to the list of clients
    clients.push_back(aux);
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
        // searches for the client with the same socket as the one that received the message
        if (client.client_socket == client_socket)
        {
            bool ok = true;
            for (auto &topic : client.topics)
            {
                if (topic == aux)
                {
                    // we cannot subscribe a client to the same topic twice
                    ok = false;
                    return 0;
                }
            }
            if (ok)
            {
                // subscribe the client to the requested topic
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
        // find the client that sent the request based on the socket it communicates on
        if (client.client_socket == client_socket)
        {
            long int to_delete;
            bool found = false;
            // search for the topic to unsubscribe
            for (long unsigned int i = 0; i < client.topics.size(); i++)
            {
                if (client.topics[i] == aux)
                {
                    to_delete = i;
                    found = true;
                }
            }
            // if the client is subscribed to the topic, remove the subscription
            if (found)
            {
                client.topics.erase(client.topics.begin() + to_delete);
                return 1;
            }
        }
    }
    return 0;
}

// function that marks a client as disconnected from the server
// returns 1 on succes or 0 on error
int disconnect_client(int client_socket, vector<tcp_client> &clients)
{
    for (auto &client : clients)
    {
        // find the client that sent the request based on the socket it communicates on
        if (client.client_socket == client_socket)
        {
            client.conected = false;
            cout << "Client " << client.id << " disconnected.\n";
            return 1;
        }
    }
    return 0;
}

// function that matches two topics
// includes the wildcard mechanic
bool is_topic_match(string topic, string subscription)
{
    // make a copy of the strings and convert them to char*
    char *to_check = new char[topic.length() + 1];
    strcpy(to_check, topic.c_str());

    char *reference = new char[subscription.length() + 1];
    strcpy(reference, subscription.c_str());

    // tokenize the first string based on / delimiter
    char *to_check_token = strtok(to_check, "/");
    vector<char *> to_check_vector, reference_vector;

    while (to_check_token != nullptr)
    {
        to_check_vector.push_back(to_check_token);
        to_check_token = strtok(nullptr, "/");
    }

    // tokenize the second string based on / delimiter
    char *reference_token = strtok(reference, "/");
    while (reference_token != nullptr)
    {
        reference_vector.push_back(reference_token);
        reference_token = strtok(nullptr, "/");
    }

    // go over the vectors and Ccheck if we have a match
    long unsigned int i = 0, j = 0;
    for (i = 0, j = 0; i < to_check_vector.size() && j < reference_vector.size();)
    {
        if (strstr(reference_vector[j], "*"))
        {
            // we found the * wildcard
            if (j + 1 == reference_vector.size())
                // if the * is on the last position of the string
                // mark the match
                return true;
            else
            {
                // go over the topic string and try to find a match after the *
                j++;
                i++;
                while (i < to_check_vector.size() && strcmp(reference_vector[j], to_check_vector[i]) != 0)
                {
                    i++;
                }
                if (i == to_check_vector.size())
                    return false;
            }
        }
        else if (strstr(reference_vector[j], "+"))
        {
            // we found the + wildcard
            // go to the next token in the string
            i++;
            j++;
        }
        else
        {
            // no wildcard found
            // see if the tokens are the same
            if (strcmp(reference_vector[j], to_check_vector[i]) == 0)
            {
                i++;
                j++;
            }
            else
                return false;
        }
    }

    // if we finished both vectors, there is a match
    if (i == to_check_vector.size() && j == reference_vector.size())
        return true;

    // if not, there is no match
    return false;
}

// function that sends the messages to the clients
void send_to_clients(char *topic, char *message, vector<tcp_client> clients)
{
    string aux(topic);

    for (auto &client : clients)
    {
        // for each client that is connected search
        // if that client is subscribed to the topic
        if (client.conected == true)
        {
            for (long unsigned int i = 0; i < client.topics.size(); i++)
            {
                if (is_topic_match(aux, client.topics[i]))
                {
                    // if we found a match, send the message to the client
                    send_all(client.client_socket, message);
                    break;
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

    // get the server ip
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

    // bind the socket to the port
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

    // bind the socket to the port
    rc = bind(tcp_socket, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (rc < 0)
    {
        perror("tcp bind fail");
        exit(0);
    }

    // make the tcp socket to listen for new connections
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

    // create the poll structure for multiplexing the IO
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
                    // send the message to close the client
                    send_all(fds[i].fd, tcp_buff);
                    // close the socket in the tcp_clients list
                    close(fds[i].fd);
                }

                // close the server
                memset(tcp_buff, 0, 12);
                break;
            }
        }
        else if (fds[1].revents & POLLIN)
        {
            // process the udp message
            memset(udp_buff, 0, 2000);

            // receive an udp message
            rc = recvfrom(udp_socket, udp_buff, 2000, MSG_DONTWAIT, (struct sockaddr *)&udp_addr, &udp_len);

            // extract the parts of the message

            // get the topic
            char topic[51];
            memcpy(topic, udp_buff, 50);

            // get the data type
            unsigned char data_type;
            data_type = udp_buff[50];

            // create the message for the clients

            rc = 0; // will keep the length of the buffer at any point.
            memset(tcp_buff, 0, 2000);

            // get he ip of the sender
            inet_ntop(AF_INET, &(udp_addr.sin_addr), tcp_buff, INET_ADDRSTRLEN);
            rc += strlen(tcp_buff);
            memcpy(tcp_buff + rc, ":\0", 2);
            rc++;

            // get the port of the sender
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
                // build an int data message
                memcpy(tcp_buff + rc, "INT\0", 4);
                rc += 3;
                memcpy(tcp_buff + rc, " - \0", 4);
                rc += 3;

                // get the data, convert it to string, append it
                unsigned char sign = udp_buff[51];
                unsigned char aux_buff[4] = {(unsigned char)udp_buff[52], (unsigned char)udp_buff[53], (unsigned char)udp_buff[54], (unsigned char)udp_buff[55]};
                uint32_t number = (aux_buff[0] << 24) | (aux_buff[1] << 16) | (aux_buff[2] << 8) | aux_buff[3];

                // check if the number is negative
                if (sign == 1 && number != 0)
                {
                    memcpy(tcp_buff + rc, "-\0", 2);
                    rc++;
                }

                // write the data received as bytes to plain text
                rc += snprintf(tcp_buff + rc, 2000, "%u", number);
                tcp_buff[rc] = '\0';
            }

            if ((int)data_type == 1)
            {

                // build a short_real data message
                memcpy(tcp_buff + rc, "SHORT_REAL\0", 11);
                rc += 10;
                memcpy(tcp_buff + rc, " - \0", 4);
                rc += 3;

                // get the data, convert it to string, append it
                uint16_t value = ((unsigned char)udp_buff[51] << 8) | (unsigned char)udp_buff[52];

                // process the data
                float float_value = (float)value / 100.0f;

                // write the data received as bytes to plain text
                rc += snprintf(tcp_buff + rc, 2000, "%.2f", float_value);
                tcp_buff[rc] = '\0';
            }

            if ((int)data_type == 2)
            {
                // build a float data message
                memcpy(tcp_buff + rc, "FLOAT\0", 6);
                rc += 5;
                memcpy(tcp_buff + rc, " - \0", 4);
                rc += 3;

                // get the data, convert it to string, append it
                uint8_t sign = (unsigned char)udp_buff[51];
                uint32_t number = ((unsigned char)udp_buff[52] << 24) | ((unsigned char)udp_buff[53] << 16) | ((unsigned char)udp_buff[54] << 8) | (unsigned char)udp_buff[55];
                uint8_t power = (unsigned char)udp_buff[56];

                // check if the number is negative
                if (sign == 1 && number != 0)
                {
                    memcpy(tcp_buff + rc, "-\0", 2);
                    rc++;
                }

                // process the number
                double value = (double)number;
                for (int i = 0; i < power; i++)
                {
                    value /= (double)10.0;
                }

                // write the data received as bytes to plain text
                rc += snprintf(tcp_buff + rc, 2000, "%.15g", value);
                tcp_buff[rc] = '\0';
            }

            if ((int)data_type == 3)
            {
                // build a string data message
                memcpy(tcp_buff + rc, "STRING\0", 7);
                rc += 6;
                memcpy(tcp_buff + rc, " - \0", 4);
                rc += 3;

                // write the string as plain text in the message
                strncat(tcp_buff, udp_buff + 51, 1500);
                rc += strnlen(udp_buff + 51, 1500);
            }

            // put a '\n' to the message
            memcpy(tcp_buff + rc, "\n\0", 2);
            rc++;

            // Send the message to the tcp clients suscribe to the topic
            send_to_clients(topic, tcp_buff, clients);

            // clear buffers
            memset(tcp_buff, 0, 2000);
            memset(udp_buff, 0, 2000);
        }
        else if (fds[2].revents & POLLIN)
        {
            // accept a new connection
            int new_client_socket = accept(fds[2].fd, (struct sockaddr *)&tcp_addr, &tcp_len);

            // get the client ID
            recv_all(new_client_socket, tcp_buff);

            // got the client ID, now add it to the list
            rc = process_connection(tcp_buff, new_client_socket, clients);
            if (rc == 1)
            {
                // new client message
                cout << "New client " << tcp_buff << " connected from " << inet_ntoa(tcp_addr.sin_addr) << ":" << ntohs(tcp_addr.sin_port) << ".\n";

                // add it to the fds for IO multiplexing
                struct pollfd aux;
                aux.fd = new_client_socket;
                aux.events = POLLIN;
                fds.push_back(aux);
            }
            else
            {
                // already have that id for a client that already connected
                cout << "Client " << tcp_buff << " already connected.\n";

                // prepare the message and send it
                memcpy(tcp_buff, "disconnect!\0", 12);
                send_all(new_client_socket, tcp_buff);
            }
            memset(tcp_buff, 0, 2000);
        }
        else
        {
            // we got a message from a client, process it
            memset(tcp_buff, 0, 2000);
            for (long unsigned int i = 3; i < fds.size(); i++)
            {
                if (fds[i].revents & POLLIN)
                {
                    // found thw client that sent the request for the server
                    recv_all(fds[i].fd, tcp_buff);
                    if (strstr(tcp_buff, "unsubscribe"))
                    {
                        // received the unsubscribe request, process it
                        rc = unsubscribe_client_to_topic(fds[i].fd, tcp_buff + 12, clients);
                        if (rc == 1)
                        {
                            // unsubscribe succesful, send confirmation
                            memcpy(tcp_buff, "ok\0", 3);
                            send_all(fds[i].fd, tcp_buff);
                        }
                        else
                        {
                            // unsubscribe unsuccesful, send error
                            memcpy(tcp_buff, "no\0", 3);
                            send_all(fds[i].fd, tcp_buff);
                        }
                        continue;
                    }
                    if (strstr(tcp_buff, "subscribe"))
                    {
                        // received the subscribe request, process it
                        rc = subscribe_client_to_topic(fds[i].fd, tcp_buff + 10, clients);
                        if (rc == 1)
                        {
                            // subscribe succesful, send confirmation
                            memcpy(tcp_buff, "ok\0", 3);
                            send_all(fds[i].fd, tcp_buff);
                        }
                        else
                        {
                            // subscribe unsuccesful, send error
                            memcpy(tcp_buff, "no\0", 3);
                            send_all(fds[i].fd, tcp_buff);
                        }
                    }
                    if (strstr(tcp_buff, "want to disconnect"))
                    {
                        // received the disconnect request, process it
                        int aux_socket = fds[i].fd;
                        rc = disconnect_client(fds[i].fd, clients);
                        if (rc == 1)
                        {
                            // disconnect succesful, send confirmation
                            memcpy(tcp_buff, "ok\0", 3);
                            send_all(fds[i].fd, tcp_buff);
                        }

                        // remove the socket from fds and close the socket
                        fds.erase(fds.begin() + i);
                        close(aux_socket);
                    }
                    memset(tcp_buff, 0, 2000);
                }
            }
        }
    }

    // close the main sockets
    close(udp_socket);
    close(tcp_socket);

    return 0;
}