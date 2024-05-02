#include <bits/stdc++.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/poll.h>

void recv_all(int socket, char *message)
{
    unsigned int have_to_receive, received, rc;

    // receive the lenght of the message
    recv(socket, &have_to_receive, 4, 0);

    received = 0;

    while (received < have_to_receive)
    {
        rc = recv(socket, message + received, have_to_receive, 0);
        received += rc;
    }
}

void send_all(int socket, char *message)
{
    unsigned int message_len, sent, rc;

    message_len = strlen(message);

    send(socket,&message_len,4,0);

    sent = 0;

    while(sent < message_len)
    {
        rc = send(socket,message + sent, message_len, 0);
        sent += rc;
    }
}