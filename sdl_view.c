
/* Standard Include Files */
#include <errno.h>
    
/* Verification Test Environment Include Files */
#include <sys/types.h>	
#include <sys/stat.h>	
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>    
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <asm/types.h>
#include <sys/mman.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include "SDL/SDL.h"
#include "SDL/SDL_thread.h"
#include "SDL/SDL_audio.h"
#include "SDL/SDL_timer.h"
#include "SDL/SDL_image.h"
static int SCREEN_WIDTH = 640;  
static int SCREEN_HEIGHT = 480;  
static  int SCREEN_BPP = 32;   

SDL_Surface *screen = NULL;  
SDL_Overlay *poverlay=NULL;
int fd_v4l,ret,i;
SDL_Rect offset;
void set_screen(int width,int height,int bpp)
{
	SCREEN_WIDTH=width;
	SCREEN_HEIGHT=height;
	SCREEN_BPP=bpp;
}

int draw_init( void)
{
        
    	/** 
     	* 初始化SDL环境，具体参数见SDL文档，这里初始化所有环境 
     	* 初始化所有环境的好处是开始时所有的环境都帮你初始化好，后面就不用担心相应的环境是否被初始化 
     	* 但缺点是有些SDL的功能你可能用不着，在这里把他初始化后会造成资源浪费 
     	* 由于是学习，所以为了方便，初始化所有环境 
     	*/  
    	if( SDL_Init( SDL_INIT_EVERYTHING ) == -1 )  
    	{  
    		printf("SDL_init fail\n");
		perror("init fail\n");
        	return 1;  
   		} 
   		/**设置程序窗口属性，该函数会根据传入的参数创建窗口，返回一个screen 
     	* screen可以看成和窗口对应的一块内存区域，程序在这块内存区域中粘贴图片，做变换 
     	* 最后再将这块区域的数据贴到屏幕上去，这样就得到了游戏中的一幅画面，即一帧 
     	* SCREEN_WIDTH     窗口宽 
     	* SCREEN_HEIGHT    窗口高 
     	* SCREEN_BPP       存储一个像素用的位数，就是我们平时配置显示器时的"颜色质量"，这里用32位 
     	* SDL_SWSURFACE    该参数可以配置是否全屏、窗口内容存储位置之类的东西，详细信息见SDL文档，这里设置为非全屏且窗口内容存储在内存中 
     	*/  
    	screen = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_SWSURFACE );  
    	if( screen == NULL )  
    	{  
        	return 1;  
    	}          
	poverlay = SDL_CreateYUVOverlay(SCREEN_WIDTH,SCREEN_HEIGHT,SDL_YV12_OVERLAY,screen);  
    	//设置窗口标题和图标，这里暂时不设置图标  
    	SDL_WM_SetCaption( "JPEG_SDLVIDEO", NULL ); 


}

void draw(char *img_path,int index)
{
                SDL_Surface *img = NULL;  
    		//加载要显示的图片，这里可以换成你想要显示的任何图片
    		img = IMG_Load(img_path);   
             
    		//将要显示的图片粘贴到screen上的左上角去
    		//源图片将要粘贴的位置，这个位置指目的区域上的位置  
    		//SDL的坐标原点在左上角，向右为X正方向，向下为Y正方向     
    		//第二个参数表示将源图片的哪部分粘贴过去，如果为NULL，表示全部  
    		//如果部分粘贴，则在第二个参数中指明取源图片的哪部分  
    		// SDL_LockSurface(screen);
    		switch (index){
                case 1:
                             offset.x = 0;  
    		                offset.y = 0; 
    		                SDL_BlitSurface(img, NULL, screen, &offset); 
                             break;

                case 2:
                         offset.x = 320;  
    		             offset.y = 0; 
    		             SDL_BlitSurface(img, NULL, screen, &offset);
                           break;

                case 3:
                    offset.x = 0;  
    		        offset.y = 240; 
    		        SDL_BlitSurface(img, NULL, screen, &offset); 
                          break;

                case 4:
                     offset.x = 320;  
    		         offset.y = 240; 
    		        SDL_BlitSurface(img, NULL, screen, &offset); 
                          break;


            }
    		//上面在screen上做的所有操作都是在内存中进行，下面是将screen内存中的内容贴到屏幕窗口中  
    		
    	         
		 //  SDL_UpdateRect(screen, 0, 0, 0, 0);
               // SDL_UnlockSurface(screen);
    		//SDL_UpdateRects(screen,1,&offset);
   		if( SDL_Flip( screen ) == -1 )
    	        {
    			printf("SDL_Flip error\n");
			exit(0);
                }
    		//SDL_Delay(500);   
    		SDL_FreeSurface( img );   
}      
void draw_destroy()
{
		//释放加载的图片  
    	     
    	//释放窗口  
    	SDL_FreeSurface( screen );        
    	//退出SDL环境  
    	SDL_Quit();   	   		
		close(fd_v4l);
		
}

int flip( unsigned char * src, int width, int height)   
{   
    unsigned char  *outy,*outu,*outv,*out_buffer,*op[3];  
	int y;
    SDL_Rect rect;  
    rect.w = width;  
    rect.h = height;  
    rect.x = 0;  
    rect.y = 0;  
    out_buffer = src;  
    SDL_LockSurface(screen);  
    SDL_LockYUVOverlay(poverlay);  
    outy = out_buffer;  
    outu = out_buffer+width*height*5/4;  
    outv = out_buffer+width*height;  
    for(y=0;y<screen->h && y<poverlay->h;y++)  
    {  
        op[0]=poverlay->pixels[0]+poverlay->pitches[0]*y;  
        op[1]=poverlay->pixels[1]+poverlay->pitches[1]*(y/2);  
        op[2]=poverlay->pixels[2]+poverlay->pitches[2]*(y/2);     
        memcpy(op[0],outy+y*width,width);  
        if(y%2 == 0)  
        {  
            memcpy(op[1],outu+width/2*y/2,width/2);  
            memcpy(op[2],outv+width/2*y/2,width/2);     
        }  
    }  
    SDL_UnlockYUVOverlay(poverlay);  
    SDL_UnlockSurface(screen);         
    SDL_DisplayYUVOverlay(poverlay, &rect);  
    return 0;  
} 
