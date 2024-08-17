#指定生成的文件名
OJB_OUT = test

#指定每一个c文件对应的.o文件
OBJS = shmem.o cJSON.o main.o msg_queue_peer.o 

#指定编译器
CC = gcc

#指定需要的库
ULDFLAGS = -pthread -lm

###########################################
#以下的内容不需要修改
###########################################
all:$(OJB_OUT)

$(OJB_OUT):$(OBJS)
	$(CC) -o $@ $^ $(ULDFLAGS)

dep_files := $(foreach f,$(OBJS),.$(f).d)
dep_files := $(wildcard $(dep_files))

ifneq ($(dep_files),)
  include $(dep_files)
endif

%.o:%.c
	$(CC) -Wp,-MD,.$@.d -c $< -o $@
    
clean:
	rm -rf .*.o.d *.o $(OJB_OUT)
