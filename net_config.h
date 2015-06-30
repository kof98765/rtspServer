#ifndef NET_CONFIG_H
#define NET_CONFIG_H

#include<sys/socket.h>
#include<sys/types.h>
#include <sys/stat.h>
#include<stdio.h>
#include<netdb.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include<unistd.h>

#define LISTEN_MAX 100
typedef struct net_arg
{
	int sock;
	struct sockaddr_in addr;
}net_arg;

#define SERVER_PORT 6000
#define VIDEO_PORT 6001
#define CHAR_PORT 6002

extern void TCPsocket_init(int *sockfd);
extern void UDPsocket_init(int *sockfd);
extern void Bind_socket(int *sockfd,char *ip,int port);
extern int Connect(int *sock,char *ip,int port);

#endif