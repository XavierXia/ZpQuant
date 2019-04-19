## ==> make 

INC_DIR   = ../../../include
#LIB_DIR   = ../../../linux64
LIB_DIR   = ../../../macos64

CC_CFLAGS = -Wall -o2
#CC_LFLAGS = -lmds_api -lpthread -lm
CC_LFLAGS = -loes_api -lpthread -lm

all:
	gcc $(CC_CFLAGS) -I$(INC_DIR) mds_client.c -L$(LIB_DIR) $(CC_LFLAGS) -o mds_client
	gcc $(CC_CFLAGS) -I$(INC_DIR) mds_subscribe.c -L$(LIB_DIR) $(CC_LFLAGS) -o mds_subscribe

clean:
	rm -f *.o mds_client mds_subscribe
