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
	attr.cap.max_send_wr = attr.cap.max_recv_wr = 10;
	attr.cap.max_send_sge = attr.cap.max_recv_sge = 10;
	attr.cap.max_inline_data = 16;

	attr.qp_context = id;
	attr.sq_sig_all = 1;
	ret = rdma_create_ep(&id, res, NULL, &attr);
	
	rdma_freeaddrinfo(res);
	if (ret) {
		printf("rdma_create_ep %d\n", errno);
		return ret;
	}

	printf("inf(pre):%s\n", recv_msg);

	mr = rdma_reg_msgs(id, recv_msg, 16);
	if (!mr) {
		printf("rdma_reg_msgs %d\n", errno);

		return ret;
	}
/*
	ret = rdma_connect(id, NULL);
	if (ret) {
		printf("rdma_connect %d\n", errno);
		return ret;
	}
	printf("rdma_connect_success!\n");
*/
	int i;
	long begin_time = time_sec;
	ret = rdma_post_recv(id, NULL, recv_msg, 16, mr);
	if (ret) {
			printf("rdma_post_recv %d\n", errno);
			return ret;
	}

	ret = rdma_connect(id, NULL);
	if (ret) {
		printf("rdma_connect %d\n", ret);
		return 1;
	}

	for (i = 0; i < iter_num; i ++ ) {
		ret = rdma_get_recv_comp(id, &wc);
		if (ret <= 0) {
			printf("rdma_get_rec_comp %d\n", ret);
			return 1;
		}
		ret = rdma_post_recv(id, NULL, recv_msg, 16, mr);
		if (ret) {
			printf("rdma_post_recv %d\n", errno);
			return ret;
		}

		printf("inf(pre):%s\n", recv_msg);
	}

	long end_time = time_sec;

	printf("Total time is %d", end_time - begin_time);

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
	ret = run();
	printf("rdma_client: end %d\n", ret);
	return ret;
}
