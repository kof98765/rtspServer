#include <stdio.h>
int is_huffman(unsigned char *buf)
{
	int i = 0;
    	unsigned char *pbuf = buf;

    while (((pbuf[0] << 8) | pbuf[1]) != 0xffda)
    {
        if (i++ > 2048)
        {
            return 0;
        }
        
        if (((pbuf[0] << 8) | pbuf[1]) == 0xffc4)
        {
            return 1;
        }

        pbuf++;
    }

    return 0;
}

int mjpeg(unsigned char* buf,int length)
{
		unsigned char *ptcur = buf;			
		int i1,i;		
		for(i1=0; i1<length; i1++)        
			{        	
				if((buf[i1] == 0x000000FF) && (buf[i1+1] == 0x000000C4))         	
					{				
					//printf("huffman table finded! \nbuf.bytesused = %d\nFFC4 = %d \n",buf.bytesused,i1);				
					break;      			
					} 		
				}		
		if(i1 == length)		
			{			
				printf("huffman tables don't exist!!!!!!!!!!!\n");					
				return -1;		
				}		
		else 		
			{			
				for(i=0; i<length; i++)        	
					{				
						if((buf== (unsigned char *)0x000000FF) && (buf== (unsigned char *)0x000000D8)) break;						
						ptcur++;			
						}			
				printf("i = %d,FF = %02x,D8 = %02x\n",i,buf[i],buf[i+1]);	
							
					
				return i;
		}
}