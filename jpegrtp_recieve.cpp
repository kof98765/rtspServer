extern "C"
{
#include <stdio.h>
#include <string.h>
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
extern "C" void pool_init(int );
extern "C" int add_pack(int timestamp,int length,char *buf);
extern "C" int prosess_pack(int timestamp,int length,char *buf);

// 错误处理函数
void checkerror(int err)
{
  if (err < 0) {
  std::cerr << RTPGetErrorString(err) << std::endl;
    exit(-1);
  }
}

int main(int argc, char** argv)
{
  RTPSession sess;
  int localport;
  int status;
	int count=0;
  if (argc != 2) {
    printf("Usage: ./recieve localport\n");
    return -1;
  }
  pool_init(1);
RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;
	sessparams.SetOwnTimestampUnit(1.0/10.0);		
	
	sessparams.SetAcceptOwnPackets(true);
	transparams.SetPortbase(atoi(argv[1]));

	
	status = sess.Create(sessparams,&transparams);
	
  checkerror(status);

        
//sess.Poll();
  do {
    // 接受RTP数据
  sess.BeginDataAccess();
 	// 检索RTP数据源
	
    if (sess.GotoFirstSourceWithData())
    {

      do {
        RTPPacket* packet;
        // 获取RTP数据报
        
        while ((packet = sess.GetNextPacket()) != NULL) {
        	
 				RTPSourceData *ssrc=sess.GetCurrentSourceInfo();
	   	   	 	prosess_pack(packet->GetTimestamp(),packet->GetPayloadLength(),(char *)packet->GetPayloadData());
 				printf("csrc:%d\n",ssrc->GetSSRC());
				
		
            sess.DeletePacket(packet);
          	// 删除RTP数据报
          	
        }
      } while (sess.GotoNextSourceWithData());
	 
	
    }    
	
      
	  sess.EndDataAccess();
	  sess.Poll();
  } while(1);

  return 0;
}
