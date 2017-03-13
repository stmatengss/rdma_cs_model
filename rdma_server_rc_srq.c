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
#include <netdb.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>
#include "define.h"

static char *port = "7471";

struct rdma_cm_id *listen_id, *id;
struct ibv_mr *mr;
char send_msg[16] = "hello world:";
//uint8_t send_msg[16];
char recv_msg[16] = "fuck\0";
//uint8_t recv_msg[16];

static int run(void)
{
		struct rdma_addrinfo hints, *res;
		struct ibv_qp_init_attr attr;
		struct ibv_wc wc;
		struct rdma_conn_param param;
		int ret;

		memset(&hints, 0, sizeof hints);
		hints.ai_flags = RAI_PASSIVE;
		hints.ai_port_space = RDMA_PS_TCP;
		//	hints.ai_port_space = RDMA_PS_UDP;
		ret = rdma_getaddrinfo(NULL, port, &hints, &res);
		if (ret) {
				printf("rdma_getaddrinfo %d\n", errno);
				return ret;
		}

		memset(&attr, 0, sizeof attr);
		attr.cap.max_send_wr = attr.cap.max_recv_wr = 1;
		attr.cap.max_send_sge = attr.cap.max_recv_sge = 1;
		//	attr.cap.max_inline_data = 16;
		//	attr.sq_sig_all = 0;
		attr.qp_type = IBV_QPT_RC;
		ret = rdma_create_ep(&listen_id, res, NULL, &attr);
		rdma_freeaddrinfo(res);
		if (ret) {
				printf("rdma_create_ep %d\n", errno);
				return ret;
		}

		ret = rdma_listen(listen_id, 0);
		if (ret) {
				printf("rdma_listen %d\n", errno);
				return ret;
		}

		ret = rdma_get_request(listen_id, &id);
		if (ret) {
				printf("rdma_get_request %d\n", errno);
				return ret;
		}

		struct ibv_srq_init_attr srq_attr;
		srq_attr.attr.max_wr = 1;
		srq_attr.attr.max_sge = 1;
		srq_attr.attr.srq_limit = 0;
		srq_attr.srq_context = id;

		ret = rdma_create_srq(id, NULL, &srq_attr);
		if (ret) {
				printf("rdma_create_srq: %s\n", strerror(errno));
				return ret;
		}

		int srqn;
		ibv_get_srq_num(id->srq, &srqn);
		srqn = htonl(srqn);

		mr = rdma_reg_msgs(id, recv_msg, 16);
		if (!mr) {
				printf("rdma_reg_msgs %d\n", errno);
				return ret;
		}

		ret = rdma_post_recv(id, NULL, recv_msg, 16, mr);
		if (ret) {
				printf("rdma_post_recv %d\n", errno);
				return ret;
		}

		memset(&param, 0, sizeof(param) );
		param.private_data = &srqn;
		param.private_data_len = sizeof srqn;
		ret = rdma_accept(id, NULL);
		if (ret) {
				printf("rdma_accept %d\n", errno);
				return ret;
		}

		/*
		   ret = rdma_post_recv(id, NULL, recv_msg, 16, mr);
		   if (ret) {
		   printf("rdma_post_recv %d\n", errno);
		   return ret;
		   }
		   */
		/*
		   ret = rdma_get_recv_comp(id, &wc);
		   if (ret <= 0) {
		   printf("rdma_get_recv_comp %d\n", ret);
		   return ret;
		   }
		   */

		int i;
		clock_t begin_time = clock();
		for (i = 0; i < 1; i ++ ) {

				ret = rdma_get_recv_comp(id, &wc);
				if (ret) {
						printf("rdma_post_send %d\n", errno);
						return ret;
				}
				printf("recv: %s\n", recv_msg);
		}
		clock_t end_time = clock();
		/*
		   ret = rdma_accept(id, NULL);
		   if (ret) {
		   printf("rdma_accept error\n");
		   return ret;
		   }
		   */
		printf("Total time is %ld\n", (long)(end_time - begin_time) );

		rdma_disconnect(id);
		rdma_dereg_mr(mr);
		rdma_destroy_ep(id);
		rdma_destroy_ep(listen_id);
		return 0;
}

int main(int argc, char **argv)
{
		int op, ret;

		while ((op = getopt(argc, argv, "p:")) != -1) {
				switch (op) {
						case 'p':
								port = optarg;
								break;
						default:
								printf("usage: %s\n", argv[0]);
								printf("\t[-p port_number]\n");
								exit(1);
				}
		}

		printf("rdma_server: start\n");
		ret = run();
		printf("rdma_server: end %d\n", ret);
		return ret;
}
