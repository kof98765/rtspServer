LIBS+=-ljrtp -lpthread -ljthread
CFLAGS+=-ISDL -LSDL-Irtp/jthread/include -Irtp/jrtplib/include/jrtplib3 -Lrtp/jthread/lib -Lrtp/jrtplib/lib/ -g
OBJ+=v4l2.o sdl_view.o   imgproc.o
CC=gcc
CXX=g++
ARM_CC=arm-linux-gcc
ARM_CXX=arm-linux-g++
ARM_LIBS=-ljrtplib -lpthread -ljthread
ARM_CFLAGS=-I/usr/local/arm_rtp/include -I/usr/local/arm_rtp/include/jrtplib3 -L/usr/local/arm_rtp/jrtplib/lib -L/usr/local/arm_rtp/jthread/lib -g


all:clean r s  
 
r:$(OBJ)
	@echo BUILDING reciever....
	@g++ -c rtp_recv.cpp $(CFLAGS) $(LIBS) 
	@g++ -o reciever recv.cpp rtp_recv.o sdl_view.o  $(CFLAGS) $(LIBS) -lSDL -lSDL_image
	@g++  rd.cpp $(CFLAGS) $(LIBS) -o RD
s:$(OBJ)
	@echo BUILDING sender....
	@g++ -o sender   v4l2_sendrtp.cpp v4l2.o imgproc.o    $(LIBS) $(CFLAGS) 
	@g++ sd.cpp $(CFLAGS) $(LIBS) -o SD
arm:clean 
	@$(ARM_CC) -c v4l2.c
	@$(ARM_CC) -c imgproc.c
	@echo BUILDING sender....
	@$(ARM_CXX) -o sender   v4l2_sendrtp.cpp v4l2.o imgproc.o    $(ARM_CFLAGS) $(ARM_LIBS) 
	cp sender /srv/tftp/send 

%.o:%.c
	@echo BUILDING $@....
	@$(CC) -c $< $(CFLAGS) $(LIBS)
clean:
	-@rm -rf reciever sender 
	-@rm *.o
