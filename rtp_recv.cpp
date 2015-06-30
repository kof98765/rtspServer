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

extern "C" int draw_init();
extern "C" void draw(char *path,int index);
void *rtp_recv();
unsigned char buffer[3276800];
int status;
struct pic_data
{
	int index;
	int offset;
	int length;
	char buf[1024];
};





struct pic_data *pic_data;
RTPSession sess;


// 错误处理函数
void checkerror(int err)
{
  if (err < 0) {
  std::cerr << RTPGetErrorString(err) << std::endl;
    exit(-1);
  }
}
int prosess_pack(int timestamp,int len,char*buf,int index)
{
	static int tmp=0;
       static int cnt=0;
	static int length;
	FILE *file;
     char img_name[10];
	snprintf(img_name,10,"%d.jpg",index+1);
	
	pic_data=(struct pic_data *)buf;
      // printf("offset=%d\n",pic_data->offset);
      memcpy(&buffer[pic_data->offset]+819200*index,pic_data->buf,1024);
       if(pic_data->length-pic_data->offset<1024) 
       {
         //只有当一帧数据接收相对完整时才写入屏幕
           
                
                if((file = fopen(img_name, "wb")) < 0)
        		{
        	   			printf("Unable to create y frame recording file\n");
        	   			 exit(-1);
        		}
				
        		printf("receive %d,lentgth=%d,index=%d\n",pic_data->offset,pic_data->length,index);		
        		//printf("buffer write %d bytes into file.\n",length);
        		fwrite(buffer+index*819200, pic_data->length, 1, file);
        		fclose(file);
        			//exit(0);
        		
        		draw(img_name,index+1);
			
           
       }
  
   
	
                   
	

}
int rtp_recv_init()
{

	
	
 	
	
	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;
	sessparams.SetOwnTimestampUnit(1.0/10.0);	
	sessparams.SetAcceptOwnPackets(true);
	draw_init();
	
		
	transparams.SetPortbase(5000);
		
	status = sess.Create(sessparams,&transparams);
	checkerror(status);
	rtp_recv();
		
		
	 		
	
}
void *rtp_recv()
{
	unsigned int list[10],i=0,index,SSRC;
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
					SSRC=ssrc->GetSSRC();
					
 					printf("ssrc:%d\n",SSRC);
					/*
					for(i=0;i<4;i++)
					{
						
							//printf("list[%d]=%d\n",i,list[i]);
							
						if(list[i]!=0)
						{
							if(SSRC!=list[i])
							{
								continue;
							}
							else
								break;
						}
						else
						{

							list[i]=SSRC;
							
							break;
						}
						
						
							
					}
					*/
					//printf("%d,index=%d\n",list[i],i);
        				if((packet = sess.GetNextPacket()) != NULL) {
        		
	   	   	 			prosess_pack(packet->GetTimestamp(),packet->GetPayloadLength(),(char *)packet->GetPayloadData(),i);
				
						
           		 			sess.DeletePacket(packet);
          				// 删除RTP数据报
          				
       		 		}
      			} while (sess.GotoNextSourceWithData());
	 
	
  		 }    
	
      
	  sess.EndDataAccess();
	 

  } while(1);

  return 0;
}



