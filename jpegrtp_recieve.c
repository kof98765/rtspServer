#include <stdio.h>
#include <string.h>

#include "rtpsession.h"
#include "rtppacket.h"

// 错误处理函数
void checkerror(int err)
{
  if (err < 0) {
    char* errstr = RTPGetErrorString(err);
    printf("Error:%s\\n", errstr);
    exit(-1);
  }
}

int main(int argc, char** argv)
{
  RTPSession sess;
  int localport;
  int status;
  int timestamp,lengh = 1025,length = 0;
  char buffer[32768];
  unsigned char *RawData;
  
  if (argc != 2) {
    printf("Usage: ./recieve localport\\n");
    return -1;
  }
	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;
	sessparams.SetOwnTimestampUnit(timestampunit);		
	
	sessparams.SetAcceptOwnPackets(true);
	if(portbase!=0)
		transparams.SetPortbase(atoi(argv[2]));
	status = sess.Create(sessparams,&transparams);	
	checkerror(status);
   // 获得用户指定的端口号
 
  // 创建RTP会话
  checkerror(status);
  FILE *file = 0; 	
  int i = 0;
  int j = 1;
        

  do {
    // 接受RTP数据
    status = sess.PollData();
 	// 检索RTP数据源

    if (sess.GotoFirstSourceWithData())
    {

      do {
        RTPPacket* packet;
        // 获取RTP数据报
        
        while ((packet = sess.GetNextPacket()) != NULL) {
        	
           	printf("Got packet %d! of pic %d!\n",i,j);
	   	   	timestamp = packet->GetTimeStamp();
           	lengh=packet->GetPayloadLength();
           	length += lengh;
           	RawData=packet->GetPayload();   //得到实际有效数据
           	printf(" timestamp:%d;lengh=%d\n",timestamp,lengh);
           	memcpy(&buffer[1024*i],RawData,lengh); 
           	i ++;   		 
            delete packet;
          	// 删除RTP数据报
        }
      } while (sess.GotoNextSourceWithData());
    }    
    if(lengh < 1024)
    {
        if((file = fopen("rtp_rev.jpg", "wb")) < 0)
    	{
           	printf("Unable to create y frame recording file\n");
            return -1;
    	}
    	else
    	{
    		fwrite(buffer, length, 1, file);
    		printf("buffer write %d bytes into file.\n",length);
    		fclose(file);
    		i = 0;//write over so reset i
    		j ++ ;//write pic and j++
    		lengh = 1025;
    	}
    }  
  } while(1);

  return 0;
}
