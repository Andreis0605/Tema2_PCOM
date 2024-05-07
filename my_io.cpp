#include <bits/stdc++.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/poll.h>

// my function that receives a tcp message from a sochet
void recv_all(int socket, char *message)
{
    unsigned int have_to_receive, received, rc, ret;

    // receive the lenght of the message
    ret = recv(socket, &have_to_receive, 4, 0);
    if (ret < 0)
    {
        perror("Error receiving a message");
        exit(0);
    }

    received = 0;

    // read from the socket as long as I still need to it
    while (received < have_to_receive)
    {
        rc = recv(socket, message + received, have_to_receive, 0);
        if (rc < 0)
        {
            perror("Error receiving a message");
            exit(0);
        }
        received += rc;
    }
}

// my function that sends a tcp message on a socket
void send_all(int socket, char *message)
{
    unsigned int message_len, sent, rc, ret;

    // get the length of the message
    message_len = strlen(message);

    // send the length of the message
    ret = send(socket, &message_len, 4, 0);
    if (ret < 0)
    {
        perror("Error sending a message");
        exit(0);
    }

    sent = 0;

    // write to the socket as long as I still need to it
    while (sent < message_len)
    {
        rc = send(socket, message + sent, message_len, 0);
        if (rc < 0)
        {
            perror("Error receiving a message");
            exit(0);
        }
        sent += rc;
    }
}