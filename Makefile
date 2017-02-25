EXE = rdma_server_w rdma_client_w 

LDLIBS += -libverbs -lrdmacm
CFLAGS += -std=c99 -D_GNU_SOURCE 

default: $(EXE)

clean:
	rm -rf $(EXE) *.o *~
