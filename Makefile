EXE = rdma_server_rc rdma_client_rc rdma_client_rc_us rdma_server_rc_us \
	rdma_server_rc_w rdma_client_rc_w  rdma_server_uc rdma_client_uc \
	rdma_server_atomic_cas rdma_client_atomic_cas \
#r 
#rdma_server_client_ud_imm
LDLIBS += -libverbs -lrdmacm
CFLAGS += -std=c99 -D_GNU_SOURCE 

default: $(EXE)

clean:
	rm -rf $(EXE) *.o *~
