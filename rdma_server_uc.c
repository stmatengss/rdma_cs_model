/*************************************************************************
 *   > File Name: 
 *   > Author: stmatengss
 *   > Mail: stmatengss@MESSAGE_LEN3.com 
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
char send_msg[MESSAGE_LEN] = "hello world:";
//uint8_t send_msg[MESSAGE_LEN];
char recv_msg[MESSAGE_LEN] = "fuck\0";
//uint8_t recv_msg[MESSAGE_LEN];

static int run(void)
{
	struct rdma_addrinfo hints, *res;
	struct ibv_qp_init_attr attr;
	struct ibv_wc wc;
	int ret;

	memset(&hints, 0, sizeof hints);
	hints.ai_flags = RAI_PASSIVE;
//	hints.ai_port_space = RDMA_PS_IB;
	hints.ai_port_space = RDMA_PS_TCP;
//	hints.ai_port_space = RDMA_PS_UDP;
	ret = rdma_getaddrinfo(NULL, port, &hints, &res);
	if (ret) {
		printf("rdma_getaddrinfo %d\n", errno);
		return ret;
	}

	memset(&attr, 0, sizeof attr);
	attr.cap.max_send_wr = attr.cap.max_recv_wr = iter_num;
	attr.cap.max_send_sge = attr.cap.max_recv_sge = 1;
	attr.cap.max_inline_data = MESSAGE_LEN;
	attr.sq_sig_all = 0;
//	attr.qp_type = IBV_QPT_UC;
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
	
	mr = rdma_reg_msgs(id, send_msg, MESSAGE_LEN);
	if (!mr) {
		printf("rdma_reg_msgs %d\n", errno);
		return ret;
	}
/*
	ret = rdma_post_recv(id, NULL, recv_msg, MESSAGE_LEN, mr);
	if (ret) {
		printf("rdma_post_recv %d\n", errno);
		return ret;
	}
*/

	ret = rdma_accept(id, NULL);
	if (ret) {
		printf("rdma_accept %d\n", errno);
		return ret;
	}

/*
	ret = rdma_post_recv(id, NULL, recv_msg, MESSAGE_LEN, mr);
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
	for (i = 0; i < iter_num; i ++ ) {
			send_msg[12] = 'a' + i % 26;
			send_msg[13] = '\0';
//			printf("send_msg:%s\n", send_msg);	
			//printf("IBV_SEND_INLINE is %d", IBV_SEND_SOLICITED);
			//printf("IBV_SEND_INLINE is %d", IBV_SEND_INLINE);
			ret = rdma_post_send(id, NULL, send_msg, MESSAGE_LEN, mr, IBV_SEND_INLINE);
			//ret = rdma_post_send(id, NULL, send_msg, MESSAGE_LEN, mr, IBV_SEND_SOLICITED);
			/*
			 * difference in IBV_SEND_FENCE
			 * 				IBV_SEND_SIGNALED
			 * 				IBV_SEND_SOLICITED
			 * 				IBV_SEND_INLINE
			 */
			
			if (ret) {
				printf("rdma_post_send %d\n", errno);
				return ret;
			}
		/*	
			ret = rdma_get_send_comp(id, &wc);
			if (ret <= 0) {
				printf("rdma_get_send_comp %d\n", ret);
				return ret;
			}
		*/	
	}
	clock_t end_time = clock();
/*
	ret = rdma_accept(id, NULL);
	if (ret) {
		printf("rdma_accept error\n");
		return ret;
	}
*/
	printf("Total time is %ld\n", (end_time - begin_time) * 1000 / CLOCKS_PER_SEC);

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
