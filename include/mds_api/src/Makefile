## ==> make 

INC_DIR   = ../../../include
LIB_DIR   = ../../../linux64
#LIB_DIR   = ../../../macos64

CC_CFLAGS = -Wall -O2
#CC_LFLAGS = -lmds_api -lpthread -lm
CC_LFLAGS = -loes_api -lgo_http -lpthread -lm 
#CC_SHARE_LFLAGS =  -lthread_pool -Wl,-rpath="../../../linux64"

all:
	#gcc $(CC_CFLAGS) -I$(INC_DIR) mds_client_pool1.c -L$(LIB_DIR) $(CC_LFLAGS) $(CC_SHARE_LFLAGS) -o mds_client_pool1
	gcc $(CC_CFLAGS) -I$(INC_DIR) mds_client_multi_thread.c -L$(LIB_DIR) $(CC_LFLAGS) -o mds_client_multi_thread
	#gcc $(CC_CFLAGS) -I$(INC_DIR) mds_subscribe.c -L$(LIB_DIR) $(CC_LFLAGS) -o mds_subscribe

clean:
	rm -f *.o mds_client mds_subscribe
