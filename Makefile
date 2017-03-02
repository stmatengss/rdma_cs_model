EXE = rdma_server_w rdma_client_w rdma_server_rc rdma_client_rc rdma_client_rc_us rdma_server_rc_us \
	 rdma_server_rc_w rdma_client_rc_w

LDLIBS += -libverbs -lrdmacm
CFLAGS += -std=c99 -D_GNU_SOURCE 

default: $(EXE)

clean:
	rm -rf $(EXE) *.o *~
