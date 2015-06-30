#include "net_config.h"
void TCPsocket_init(int *sockfd)
{
	 const int on=1;
	*sockfd=socket(AF_INET, SOCK_STREAM, 0);

	  if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))<0)
  {
    printf("initSvr: Socket options set error.\n");
    exit(1);
  }
}
void UDPsocket_init(int *sockfd)
{
	 const int on=1;
	*sockfd= socket(AF_INET, SOCK_DGRAM, 0);
	  if (setsockopt(*sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on))<0)
  {
    printf("initSvr: Socket options set error.\n");
    exit(1);
  }
}
int Connect(int *sock,char *ip,int port)
{
	struct sockaddr_in addr;
	int con1;
	bzero(&addr, sizeof(struct sockaddr));
	addr.sin_family = AF_INET;
	addr.sin_port=htons(port);                         // 客户端视频端口
	addr.sin_addr.s_addr=inet_addr(ip);
	if((con1 = connect(*sock,(struct sockaddr *)&addr,sizeof(struct sockaddr))) != 0)
		{
			return -1;
		}
	return 0;
	
}
void Bind_socket(int *sockfd,char *ip,int port)
{
	
	struct sockaddr_in addr;
	bzero(&addr, sizeof(struct sockaddr));
	addr.sin_family = AF_INET;
	addr.sin_port=htons(port);
	
	if(ip==NULL)
		addr.sin_addr.s_addr=htonl(INADDR_ANY);
	else
		addr.sin_addr.s_addr=inet_addr(ip);
	if(bind(*sockfd,(struct sockaddr *)&addr,sizeof(struct sockaddr))==-1)
		perror("bind");
	
	if(listen(*sockfd, LISTEN_MAX)== -1 )
	{
		perror("listen failed\n");
	}
}
	

	