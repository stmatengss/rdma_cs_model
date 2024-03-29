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
	attr.cap.max_send_wr = attr.cap.max_recv_wr = 10;
	attr.cap.max_send_sge = attr.cap.max_recv_sge = 10;
	attr.cap.max_inline_data = 16;
	attr.sq_sig_all = 1;
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
	
	mr = ibv_reg_mr(id->pd, wr_msg, 16, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);
	if (!mr) {
		printf("rdma_reg_msgs %d\n", errno);
		return ret;
	}

	printf("buffer addr: %lld, len: %zdByte, lkey: %d\n", 
					wr_msg, mr->length, mr->rkey);

	struct common_priv_data private_data = {
			.buffer_addr = (uint64_t)wr_msg,
			.buffer_rkey = mr->rkey,
			.buffer_length = mr->length,
	};

	struct rdma_conn_param conn_param;
	memset(&conn_param, 0, sizeof conn_param);
	conn_param.private_data = &private_data;
	conn_param.private_data_len = &private_data;
	conn_param.responder_resources = 2;
	conn_param.retry_count = 5;
	conn_param.rnr_retry_count = 5;

	ret = rdma_accept(id, conn_param);
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
	long begin_time = time_sec;
	for (i = 0; i < iter_num; i ++ ) {
			send_msg[12] = 'a' + i % 26;
			send_msg[13] = '\0';
			printf("send_msg:%s\n", send_msg);	
			//printf("IBV_SEND_INLINE is %d", IBV_SEND_SOLICITED);
			//printf("IBV_SEND_INLINE is %d", IBV_SEND_INLINE);
			ret = rdma_post_send(id, NULL, send_msg, 16, mr, IBV_SEND_INLINE);
			//ret = rdma_post_send(id, NULL, send_msg, 16, mr, IBV_SEND_SOLICITED);
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
			ret = rdma_get_send_comp(id, &wc);
			if (ret <= 0) {
				printf("rdma_get_send_comp %d\n", ret);
				return ret;
			}
	}
	long end_time = time_sec;
/*
	ret = rdma_accept(id, NULL);
	if (ret) {
		printf("rdma_accept error\n");
		return ret;
	}
*/
	printf("Total time is %d", end_time - begin_time);

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
