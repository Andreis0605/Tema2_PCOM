#include <bits/stdc++.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/poll.h>

#include "my_io.hpp"

using namespace std;

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    char *stdin_buff = (char *)calloc(1800, sizeof(char));
    char *tcp_buff = (char *)calloc(1800, sizeof(char));

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[2]);
    serv_addr.sin_port = htons(atoi(argv[3]));

    // create the tcp socket
    int tcp_socket;
    if ((tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("client tcp socket creation failed");
        exit(0);
    }

    // disable the nagle algorithm
    int flag = 1;
    if (setsockopt(tcp_socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0)
    {
        perror("nagle algorith disable failed");
        exit(0);
    }
    // send the connect request to the server

    if (connect(tcp_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("client unable to connect to the server");
        exit(0);
    }

    // send the ID to the server
    memcpy(stdin_buff, argv[1], strlen(argv[1]) + 1);
    send_all(tcp_socket, stdin_buff); // change this

    // multiplex the io

    // create the poll structure
    vector<struct pollfd> fds(2);

    // this is the standart input
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    // this is the tcp entry
    fds[1].fd = tcp_socket;
    fds[1].events = POLLIN;

    // start processing the messages
    while (true)
    {
        poll(fds.data(), fds.size(), -1);

        if (fds[0].revents & POLLIN)
        {
            // read what was put to the stdin
            char buffer[200];
            cin.getline(buffer, 200);

            if (strstr(buffer, "exit"))
            {
                // TODO: send a message to the server in order to disconect
                memcpy(tcp_buff, "want to disconnect\0", 19);
                send_all(tcp_socket, tcp_buff);
                memset(tcp_buff, 0, 19);
                recv_all(tcp_socket, tcp_buff);
                if (strstr(tcp_buff, "ok"))
                    break;
            }
            if (strstr(buffer, "unsubscribe "))
            {
                send_all(tcp_socket, buffer);
                recv_all(tcp_socket, tcp_buff);
                if (strstr(tcp_buff, "ok"))
                {
                    cout << "Unsubscribed to topic " << ((char *)(strstr(buffer, "unsubscribe ") + 12)) << '\n';
                    continue;
                }
                else
                    cout << "N-a mers";
            }
            if (strstr(buffer, "subscribe "))
            {
                send_all(tcp_socket, buffer);
                recv_all(tcp_socket, tcp_buff);
                if (strstr(tcp_buff, "ok"))
                    cout << "Subscribed to topic " << ((char *)(strstr(buffer, "subscribe ") + 10)) << '\n';
                else
                    cout << "N-a mers";
            }
        }
        else
        {
            recv_all(tcp_socket, tcp_buff);
            if (strstr(tcp_buff, "disconnect!"))
                break;
        }
    }

    close(tcp_socket);

    return 0;
}