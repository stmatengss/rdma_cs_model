/*************************************************************************
 *   > File Name: 
 *   > Author: stmatengss
 *   > Mail: stmatengss@163.com 
 *   > Created Time: 2017年01月04日 星期三 10时41分04秒
 *************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include "define.h"

static char *server = "127.0.0.1";
static char *port = "7471";

struct rdma_cm_id *id;
struct ibv_mr *mr;
char send_msg[16] = "hello world\0";
char recv_msg[16] = "fuck\0";

static int run(void)
{
	struct rdma_addrinfo hints, *res;
	struct ibv_qp_init_attr attr;
	struct ibv_wc wc;
	int ret;

	memset(&hints, 0, sizeof hints);
	hints.ai_port_space = RDMA_PS_TCP;
	//hints.ai_port_space = RDMA_PS_UDP;
	ret = rdma_getaddrinfo(server, port, &hints, &res);
	if (ret) {
		printf("rdma_getaddrinfo %d\n", errno);
		return ret;
	}

	memset(&attr, 0, sizeof attr);
	attr.cap.max_send_wr = attr.cap.max_recv_wr = 1;
	attr.cap.max_send_sge = attr.cap.max_recv_sge = 1;
	attr.sq_sig_all = 1;
	ret = rdma_create_ep(&id, res, NULL, &attr);
	
	rdma_freeaddrinfo(res);
	if (ret) {
		printf("rdma_create_ep %d\n", errno);
		return ret;
	}	
	ret = rdma_connect(id, NULL);
	if (ret) {
		printf("rdma_connect %d\n", ret);
		return 1;
	}

	int srqn = ntohl(*(uint32_t *) id->event->param.conn.private_data);

	/*
	 * send message
	 * */
	struct ibv_send_wr wr, *bad;
	struct ibv_sge sge;

	sge.addr = (uint64_t) (uintptr_t) send_msg;
	sge.length = (uint32_t) sizeof (send_msg);
	sge.lkey = 0;
	wr.wr_id = (uintptr_t) NULL;
	wr.next = NULL;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.opcode = IBV_WR_SEND;
	wr.send_flags = IBV_SEND_INLINE;
	wr.qp_type.xrc.remote_srqn = srqn;

	ret = ibv_post_send(id->qp, &wr, &bad);
	if (ret) {	
			printf("ibv_post_send: %s\n", strerror(errno));
			return ret;
	}

	clock_t begin_time = clock();

	int i;
	for (i = 0; i < iter_num; i ++ ) {
			ret = rdma_get_send_comp(id, &wc);
			if (ret <= 0) {
					printf("rdma_get_send_comp %d\n", ret);
					return 1;
			}
			printf("inf(after):%s\n", recv_msg);
	}

	clock_t end_time =clock();

	printf("Total time is %ld\n", (long)(end_time - begin_time));

	rdma_disconnect(id);
	rdma_dereg_mr(mr);
	rdma_destroy_ep(id);
	return 0;
}

int main(int argc, char **argv)
{
	int op, ret;

	while ((op = getopt(argc, argv, "s:p:")) != -1) {
		switch (op) {
		case 's':
			server = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		default:
			printf("usage: %s\n", argv[0]);
			printf("\t[-s server_address]\n");
			printf("\t[-p port_number]\n");
			exit(1);
		}
	}

	printf("rdma_client: start\n");
	//ret = run();
	printf("rdma_client: end %d\n", ret);
	return 0;
}
