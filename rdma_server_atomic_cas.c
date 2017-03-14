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

static char *port = "7471";

struct rdma_cm_id *listen_id, *id;
struct ibv_mr *mr;
ll temp = 0ULL;
ll *send_msg = &temp;
//char send_msg[LL_LEN] = "hello world:a\0";
//uint8_t send_msg[LL_LEN];
char recv_msg[LL_LEN] = "fuck\0";
//uint8_t recv_msg[LL_LEN];

static int run(void)
{
	struct rdma_addrinfo hints, *res;
	struct ibv_qp_init_attr attr;
	struct ibv_wc wc;
	int ret;

	printf("pre: %lld\n", *send_msg);

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
	attr.cap.max_send_wr = attr.cap.max_recv_wr = 10000;
	attr.cap.max_send_sge = attr.cap.max_recv_sge = 1;
	//attr.cap.max_inline_data = LL_LEN;
	attr.sq_sig_all = 0;
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

	struct rdma_event_channel *channel;
	channel = rdma_create_event_channel();
	if (!channel) {
			printf("create_event_channel error\n");
			return 1;
	}

	ret = rdma_migrate_id(id, channel);
	if (ret) {
			printf("rdma_migrate_id : %s\n", strerror(errno));
			return ret;
	}

	mr = ibv_reg_mr(id->pd, (char *)send_msg, LL_LEN, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);
	if (!mr) {
			printf("ibv_reg_mr: %s", strerror(errno));
			return 1;
	}
	struct common_priv_data priv_data = {
			.buffer_addr = (uint64_t)send_msg,
			.buffer_rkey = mr->rkey,
			.buffer_length = mr->length,
	};

	struct rdma_conn_param conn_param;
	memset(&conn_param, 0, sizeof conn_param);
	conn_param.private_data = &priv_data;
	conn_param.private_data_len = sizeof priv_data;
	conn_param.responder_resources = 2;
	conn_param.retry_count = 5;
	conn_param.rnr_retry_count = 5;

	
	ret = rdma_accept(id, &conn_param);
	if (ret) {
		printf("rdma_accept %d\n", errno);
		return ret;
	}
	
	struct rdma_cm_event *event;
	ret = rdma_get_cm_event(channel, &event);
	if (ret) {
			printf("fuck\n");
	}
	if (event->event == RDMA_CM_EVENT_ESTABLISHED) {
			printf("connect ready\n");
	} else {
			printf("error\n");
			return 1;
	}
	rdma_ack_cm_event(event);

	/*
	PRINT_LINE
	struct common_priv_data *private_data;
	private_data = id->event->param.conn.private_data;
	PRINT_LINE
	uint64_t raddr = private_data->buffer_addr;
	uint32_t rkey = private_data->buffer_rkey;
	printf("Remote key: %lld, rkey:%d\n", raddr, rkey);
*/
	int i;
	long begin_time = time_sec;
	PRINT_LINE
	for (i = 0; i < iter_num; i ++ ) {
			
		/*	
			ret = rdma_get_send_comp(id, &wc);
			if (ret <= 0) {
				printf("rdma_get_send_comp %d\n", ret);
				return ret;
			}
		*/	
			
			struct rdma_cm_event *event;
			if (rdma_get_cm_event(id->channel, &event)) {
					printf("error: %s", strerror(errno));
					break;
			}
			switch (event->event) {
					case RDMA_CM_EVENT_DISCONNECTED:
							printf("Disconnenct\n");
							printf("msg is :%lld\n", *send_msg);
							goto disconnect;
					default:
							printf("Event: %s\n", rdma_event_str(event->event));
							break;
			}
			rdma_ack_cm_event(event);

//			while ( ibv_poll_cq(id->recv_cq, 1, &wc) == 0 );
			printf("msg is: %lld\n", *send_msg);
	}
	PRINT_LINE
	long end_time = time_sec;

disconnect:
	printf("msg is: %lld\n", *send_msg);

	printf("Total time is %ld\n", end_time - begin_time);

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
