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
#include <arpa/inet.h>
#include "define.h"

static char *port = "7471";

#define PRINT_LINE printf("line: %d\n", __LINE__);

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

		struct rdma_event_channel *event_channel = rdma_create_event_channel();
		//
		//	ret = rdma_create_ep(&listen_id, res, NULL, &attr);

		ret = rdma_create_id(event_channel, &listen_id, NULL, RDMA_PS_TCP);
		if (ret) {
				printf("rdma_create id %s\n", strerror(errno) );
				return ret;
		}

		struct sockaddr_in sin;
		sin.sin_family = AF_INET;
		sin.sin_port = htons(atoi(port));
		sin.sin_addr.s_addr = INADDR_ANY;
//		ret = rdma_bind_addr(listen_id, (struct sockaddr *)&sin);
		ret = rdma_bind_addr(listen_id, res->ai_src_addr);
		if (ret) {
				printf("rdma_bind_addr %s\n", strerror(errno) );
				return ret;
		}
		
//		printf("==:%d\n", res->ai_src_addr->sin_family == AF_INET);
//		printf("==:%d\n", res->ai_src_addr->sin_port == htons(atoi(port)));
//		printf("==:%d\n", res->ai_src_addr->sin_addr.s_addr == INADDR_ANY);

/*
		ret = rdma_resolve_addr(listen_id, res->ai_src_addr, res->ai_dst_addr, 2000);
		if (ret) {
				printf("rdma_resolve_addr %s\n", strerror(errno) );
				return ret;
		}

		ret = rdma_resolve_route(listen_id, 2000);
		if (ret) {
				printf("rdma_resolve_route %s\n", strerror(errno) );
				return ret;
		}
*/
		memset(&attr, 0, sizeof attr);
		attr.cap.max_send_wr = attr.cap.max_recv_wr = 10000;
		attr.cap.max_send_sge = attr.cap.max_recv_sge = 1;
		attr.cap.max_inline_data = 16;
		attr.sq_sig_all = 1;
		attr.qp_type = IBV_QPT_RC;


		PRINT_LINE
		ret = rdma_listen(listen_id, 0);
		if (ret) {
				printf("rdma_listen %d\n", errno);
				return ret;
		}

/*
 * FIXME
		
		struct sockaddr_in sin;
		sin.sin_family = AF_INET;
		sin.sin_port = htons(atoi(port));



		ret = rdma_bind_addr(listen_id, (struct sockaddr_in *)&sin);
		if (ret) {
				printf("rdma_bind_addr %s\n", strerror(errno));
				return ret;
		}

		ret = rdma_create_qp(listen_id, )
*/
		struct rdma_cm_event *event;
		ret = rdma_get_cm_event(listen_id->channel, &event);
		if (ret) {
				printf("rdma_get_cm_event %s\n", strerror(errno));
				return ret;
		}

		if (event->event != RDMA_CM_EVENT_CONNECT_REQUEST) {
				return 1;
		}

		id = event->id;

		rdma_ack_cm_event(listen_id->event);

/*
 * FIXME
		ret = rdma_get_request(listen_id, &id);
		if (ret) {
				printf("rdma_get_request %s\n", strerror(errno) );
				return ret;
		}
*/
		rdma_freeaddrinfo(res);


		struct ibv_pd *pd;
		pd = ibv_alloc_pd(id->verbs);
		if (!pd) {
				printf("ibv_alloc_pd: %s\n", strerror(errno));
				return 1;
		}

		struct ibv_comp_channel *comp_channel;
		comp_channel = ibv_create_comp_channel(id->verbs);
		if (!comp_channel) {
				printf("ibv_create_comp_channel: %s\n", strerror(errno));
				return 1;
		}

		struct ibv_cq *cq;
		cq = ibv_create_cq(id->verbs, 2, NULL, comp_channel, 0);
		if (!cq) {
				printf("ibv_create_cq %s\n", strerror(errno));
				return 1;
		}

		ret = ibv_req_notify_cq(cq, 0);
		if (ret) {
				printf("ibv_req_notify_cq: %s\n", strerror(errno));
				return ret;
		}

		PRINT_LINE
		ret = rdma_create_qp(id, pd, &attr); 
		if (ret) {
				printf("rdma_create_qp: %s\n", strerror(errno));
				return ret;
		}

		mr = rdma_reg_msgs(id, send_msg, 16);
		if (!mr) {
				printf("rdma_reg_msgs %d\n", errno);
				return ret;
		}
		/*
		   ret = rdma_post_recv(id, NULL, recv_msg, 16, mr);
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
