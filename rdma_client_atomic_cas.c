/*************************************************************************
 *   > File Name: 
 *   > Author: stmatengss
 *   > Mail: stmatengss@LL_LEN3.com 
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
//char send_msg[LL_LEN] = "hello world\0";
ll temp = 1ULL;
ll *send_msg = &temp;
char recv_msg[LL_LEN] = "fuck\0";

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
	attr.cap.max_send_wr = attr.cap.max_recv_wr = 10000;
	attr.cap.max_send_sge = attr.cap.max_recv_sge = 1;
	attr.cap.max_inline_data = LL_LEN;
	attr.qp_type = IBV_QPT_RC;
	attr.qp_context = id;
	attr.sq_sig_all = 0;
	ret = rdma_create_ep(&id, res, NULL, &attr);
	
	rdma_freeaddrinfo(res);
	if (ret) {
		printf("rdma_create_ep %d\n", errno);
		return ret;
	}

	printf("inf(pre):%s\n", recv_msg);

	mr = ibv_reg_mr(id->pd, (char *)send_msg, LL_LEN, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);
	if (!mr) {
		printf("rdma_reg_msgs %d\n", errno);
		return ret;
	}

	printf("buffer addr: %lld, len: %zdByte, lkey: %d\n", 
					send_msg, mr->length, mr->rkey);

	struct common_priv_data private_data = {
			.buffer_addr = (uint64_t)send_msg,
			.buffer_rkey = mr->rkey,
			.buffer_length = mr->length,
	};

	struct rdma_conn_param conn_param;
	memset(&conn_param, 0, sizeof conn_param);
	conn_param.private_data = &private_data;
	conn_param.private_data_len = sizeof private_data;
	conn_param.responder_resources = 2;
	conn_param.retry_count = 5;
	conn_param.rnr_retry_count = 5;

	int i;
	long begin_time = time_sec;

	ret = rdma_connect(id, &conn_param);
	if (ret) {
		printf("rdma_connect %d\n", ret);
		return 1;
	}

	struct common_priv_data *priv_data;

	priv_data = id->event->param.conn.private_data;
	uint64_t raddr = priv_data->buffer_addr;
	uint32_t rkey = priv_data->buffer_rkey;
	printf("Remote key: %lld, rkey:%d\n", raddr, rkey);

	for (i = 0; i < iter_num; i ++ ) {
//			send_msg[12] = 'a' + i % 26;
//			send_msg[13] = '\0';
			printf("send_msg:%lld\n", *send_msg);	

			struct ibv_sge sg;
			struct ibv_send_wr wr;
			struct ibv_send_wr *bad_wr;

			memset(&sg, 0,sizeof sg);
			sg.addr = (uintptr_t)send_msg;
			sg.length = LL_LEN;
			sg.lkey = mr->lkey;

			memset(&wr, 0,sizeof wr);
			wr.wr_id = 0;
			wr.sg_list = &sg;
			wr.num_sge = 1;
			wr.opcode = IBV_WR_ATOMIC_CMP_AND_SWP;
			wr.send_flags = IBV_SEND_SIGNALED;
			wr.wr.atomic.remote_addr = raddr;
			wr.wr.atomic.rkey = rkey;
			wr.wr.atomic.compare_add = 0ULL;
			wr.wr.atomic.swap = 2ULL;

			ret = ibv_post_send(id->qp, &wr, &bad_wr);
			if (ret) {
				printf("rdma_post_send %d\n", errno);
				printf("error: %s\n", strerror(errno));
				return ret;
			}
//			while ( ibv_poll_cq(id->recv_cq, 1, &wc) == 0 );
			/*
			ret = rdma_get_recv_comp(id, &wc);
			if (ret <= 0) {
					printf("rdma_get_rec_comp %d\n", ret);
					return 1;
			}
			printf("inf(after):%s\n", recv_msg);
			*/
	}

	long end_time = time_sec;

	printf("Total time is %ld\n", end_time - begin_time);

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
