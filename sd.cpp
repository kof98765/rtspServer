#include <errno.h>
#include "rtpsession.h" 
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"

using namespace jrtplib;
using namespace std;

int main(int argc,char **argv)
{
	
	RTPSession sess;
    	unsigned long destip;
    	int destport;
    	int portbase = 6000;
    	int status, index,ret,i,j,Bn;

    	

    // 获得接收端的IP地址和端口号
	destip = ntohl(inet_addr("127.0.0.1"));
    if (destip == INADDR_NONE) {
      printf("Bad IP address specified\n");
      return -1;
    }
 	
    destport =portbase;
//	destport=5000;
	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;
	sessparams.SetOwnTimestampUnit(1.0/10.0);		
	sessparams.SetAcceptOwnPackets(true);
	
	
	RTPIPv4Address addr(destip,destport);
	
	transparams.SetPortbase(atoi(argv[1]));
    // 创建RTP会话
    
 	status = sess.Create(sessparams,&transparams);	
	
    // 指定RTP数据接收端
	status = sess.AddDestination(addr);

    index = 1;
	while(1)
    	{
   		
			sess.SendPacket((void*)"123456789",10,(unsigned char)0,false,100UL);
			sleep(1);
			sess.SendPacket((void*)"123456789",10,(unsigned char)1,false,1000UL);
			sleep(2);
    	}
    return 0;
    	

}
