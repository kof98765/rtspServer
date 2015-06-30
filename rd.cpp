extern "C"
{
#include <stdio.h>
#include <string.h>
#include <pthread.h>
}
#include <stdlib.h>
#include <iostream>
#include "rtpsession.h"
#include "rtppacket.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtpsourcedata.h"
using namespace jrtplib;
using namespace std;
	RTPSession sess;
extern "C" int draw_init();
extern "C" void draw(char *path,int index);
void *rtp_recv();
unsigned char buffer[3276800];
int status;
int rtp_recv_init()
{

	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;
	sessparams.SetOwnTimestampUnit(1.0/10.0);	
	sessparams.SetAcceptOwnPackets(true);
	//draw_init();
	
		
	transparams.SetPortbase(6000);
		
	status = sess.Create(sessparams,&transparams);
	//checkerror(status);
	rtp_recv();
		
		
	 		
	
}
void *rtp_recv()
{
	int list[10],i,index;
	bzero(list,10);
	do {
    // 接受RTP数据
    		sess.Poll();
 		sess.BeginDataAccess();
 	// 检索RTP数据源
    		if (sess.GotoFirstSourceWithData())
    		{
			
     			 do {
      				  	RTPPacket* packet;
        			// 获取RTP数据报
         				RTPSourceData *ssrc=sess.GetCurrentSourceInfo();
					
 					printf("ssrc:%d\n",ssrc->GetSSRC());
					
        				if((packet = sess.GetNextPacket()) != NULL) {
        					printf("get a payload %d\n",packet->GetPayloadType());
	   	   	 			printf("the timestamp is %u\n",packet->GetTimestamp());		
           		 			sess.DeletePacket(packet);
          				// 删除RTP数据报
          				
       		 		}
      			} while (sess.GotoNextSourceWithData());
	 
	
  		 }    
	
      
	  sess.EndDataAccess();
	 

  } while(1);

  return 0;
}

int main()
{
	rtp_recv_init();
}
