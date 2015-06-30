
/* Standard Include Files */
#include <errno.h>
#include "rtpsession.h" 
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
/* Verification Test Environment Include Files */

#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>




extern "C" {
	#include "v4l2.h"
	struct framebuffer *v4l_get_frame(int fd_v4l);
 	int v4l_capture_start(int fd);
	int v4l_capture_init(char *dev);
	
}



 
struct framebuffer *buffer;

char *dpath= "/dev/video0";
const int SCREEN_WIDTH = 640;  
const int SCREEN_HEIGHT = 480;  
const int SCREEN_BPP = 32;   
using namespace jrtplib;
using namespace std;
// 错误处理函数
void checkerror(int rtperr)
{
	if (rtperr < 0)
	{
		std::cout << "ERROR: " << RTPGetErrorString(rtperr) << std::endl;
		exit(-1);
	}
}
void delay(unsigned long time){
	int i = 0;
	for(i=0;i<100*time;i++);
}
struct pic_data
{
	int index;
	int offset;
	int length;
	char buf[1024];
}pic_data;


int main(int argc, char **argv)
{
	int fd_v4l;
	RTPSession sess;
    	unsigned long destip;
    	int destport;
    	int portbase = 6000;
    	int status, index,ret,i,j,Bn;

    	//char buffer_char[10];

    	if (argc < 3) 
    	{
      		printf("Usage: ./sender dest_ip port\n");
      		printf("Please input like this:./sender 127.0.0.1 8080 /dev/video0\n");
      		return -1;
    	}

    // 获得接收端的IP地址和端口号
	destip = ntohl(inet_addr(argv[1]));
    if (destip == INADDR_NONE) {
      printf("Bad IP address specified\n");
      return -1;
    }
 	printf("ip=%s\n",argv[1]);
    destport = 5000;
//	destport=5000;
	RTPUDPv4TransmissionParams transparams;
	RTPSessionParams sessparams;
	sessparams.SetOwnTimestampUnit(1.0/10.0);		
	sessparams.SetAcceptOwnPackets(true);
	
	
	RTPIPv4Address addr(destip,destport);
	
	transparams.SetPortbase(atoi(argv[2]));
    // 创建RTP会话
    
 	status = sess.Create(sessparams,&transparams);	
	checkerror(status);
	
    // 指定RTP数据接收端
  
	status = sess.AddDestination(addr);
		
    		
    	checkerror(status);

    index = 1;
	 if(argc>3)
			fd_v4l =v4l_capture_init(argv[3]);
	else
		fd_v4l = v4l_capture_init(dpath);
	printf("is open the camera\n");
	v4l_capture_start(fd_v4l);
	printf("now starting capture\n");
    while(1)
    {
		
		buffer=v4l_get_frame(fd_v4l);
	
		memset(&pic_data,0,sizeof(pic_data));
		pic_data.index=index;
		pic_data.length=buffer->length;
		pic_data.offset=0;
		Bn = 1024;
	
    		printf("Sending the %dst pic that Bn = %d,length = %d\n",index,Bn,buffer->length);

 
   		for(i=0;i<=buffer->length;i+=Bn)
    		{
    			j = buffer->length- i;
		
    			if(i == 0)
    			{	
			
				memcpy(pic_data.buf,(void *)(&buffer->start[pic_data.offset]),1024);
				sess.SendPacket(&pic_data,sizeof(pic_data),(unsigned char)0,false,100UL);
				pic_data.offset+=1024;
				continue;
    			}
    			if(j > Bn)
    			{
				memcpy(pic_data.buf,(void *)(&buffer->start[pic_data.offset]),1024);
		
				sess.SendPacket(&pic_data,sizeof(pic_data),(unsigned char)0,false,100UL);
    				pic_data.offset+=1024;
    				printf("%d packet send!\n",i/Bn);
    	
    			}
    			else 
    			{
				memcpy(pic_data.buf,(void *)(&buffer->start[pic_data.offset]),j);
    				sess.SendPacket(&pic_data,sizeof(pic_data),(unsigned char)0,false,100UL);
    				printf("%d packet send!\n",buffer->length/Bn);
				printf("i=%d,j=%d\n",pic_data.offset,j);
    		
    			}    
		
    		}
    		//checkerror(status);
   		printf("Packets of %d pic have been sent!\n",index);
    		index ++;
    

    };
    
    close(fd_v4l);
    return 0;
    	/*
        SDL_Surface *message = NULL;  
    	SDL_Surface *screen = NULL;  
    	SDL_Rect offset;
    	/** 
     	* 初始化SDL环境，具体参数见SDL文档，这里初始化所有环境 
     	* 初始化所有环境的好处是开始时所有的环境都帮你初始化好，后面就不用担心相应的环境是否被初始化 
     	* 但缺点是有些SDL的功能你可能用不着，在这里把他初始化后会造成资源浪费 
     	* 由于是学习，所以为了方便，初始化所有环境 
     	 
    	if( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )  
    	{  
        	return 1;  
   		} 
   		/**设置程序窗口属性，该函数会根据传入的参数创建窗口，返回一个screen 
     	* screen可以看成和窗口对应的一块内存区域，程序在这块内存区域中粘贴图片，做变换 
     	* 最后再将这块区域的数据贴到屏幕上去，这样就得到了游戏中的一幅画面，即一帧 
     	* SCREEN_WIDTH     窗口宽 
     	* SCREEN_HEIGHT    窗口高 
     	* SCREEN_BPP       存储一个像素用的位数，就是我们平时配置显示器时的"颜色质量"，这里用32位 
     	* SDL_SWSURFACE    该参数可以配置是否全屏、窗口内容存储位置之类的东西，详细信息见SDL文档，这里设置为非全屏且窗口内容存储在内存中 
     	  
    	screen = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_SWSURFACE );  
    	if( screen == NULL )  
    	{  
        	return 1;  
    	}          
    	//设置窗口标题和图标，这里暂时不设置图标  
    	SDL_WM_SetCaption( "JPEG_SDLVIDEO", NULL ); 
        
        fd_v4l = v4l_capture_setup();
        for(i=0;i<10;i++)
        {        
        	ret = v4l_capture_test(fd_v4l, "1.jpg");
			if(ret < 0)printf("capture error! \n");
		}
			
			
			/*
    		//加载要显示的图片，这里可以换成你想要显示的任何图片
    		message = IMG_Load( "1_rev.jpg" );   
    		//将要显示的图片粘贴到screen上的左上角去
    		//源图片将要粘贴的位置，这个位置指目的区域上的位置  
    		//SDL的坐标原点在左上角，向右为X正方向，向下为Y正方向     
    		//第二个参数表示将源图片的哪部分粘贴过去，如果为NULL，表示全部  
    		//如果部分粘贴，则在第二个参数中指明取源图片的哪部分   
    		offset.x = 0;  
    		offset.y = 0; 
    		SDL_BlitSurface(message, NULL, screen, &offset);  
  
    		//上面在screen上做的所有操作都是在内存中进行，下面是将screen内存中的内容贴到屏幕窗口中  
    		if( SDL_Flip( screen ) == -1 )return 1;  
    		SDL_Delay(500);   
    	}       
    	//释放加载的图片  
    	SDL_FreeSurface( message );        
    	//释放窗口  
    	SDL_FreeSurface( screen );        
    	//退出SDL环境  
    	SDL_Quit(); 
    	  	   		
		close(fd_v4l);
		return 0;*/
}


