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

struct common_priv_data {

		uint64_t buffer_addr;
		uint32_t buffer_rkey;
		size_t   buffer_length;
};

static char *server = "127.0.0.1";
static char *port = "7471";

struct rdma_cm_id *id;
struct ibv_mr *mr;
struct ibv_mr *back_mr;
char wr_msg[32] = "hello world\0";
char back_msg[16] = "";

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
		attr.cap.max_inline_data = 16;
		attr.qp_type = IBV_QPT_RC;
		attr.qp_context = id;
		attr.sq_sig_all = 1;
		ret = rdma_create_ep(&id, res, NULL, &attr);

		rdma_freeaddrinfo(res);
		if (ret) {
				printf("rdma_create_ep %d\n", errno);
				return ret;
		}

//		mr = rdma_reg_write(id, recv_msg, 16);
		mr = ibv_reg_mr(id->pd, wr_msg, 32, 
						IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE | 
						IBV_ACCESS_REMOTE_READ);
		if (!mr) {
				printf("rdma_reg_msgs %d\n", errno);

				return ret;
		}
/*
		back_mr = ibv_reg_mr(id->pd, back_msg, 16,
						IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE |
						IBV_ACCESS_REMOTE_READ);
		if (!mr) {
				printf("rdma_reg_msgs %d\n", errno);

				return ret;
		}
		*/
		int i;
		long begin_time = time_sec;

		/*
		   for (i = 0; i < iter_num; i ++ ) {
		   ret = rdma_post_recv(id, NULL, recv_msg, 16, mr);
		   if (ret) {
		   printf("rdma_post_recv %d\n", errno);
		   return et;
		   }

		   }
		   */
		const struct common_priv_data *priv_data;

		ret = rdma_connect(id, NULL);
		if (ret) {
				printf("Error code: %s\n", strerror(errno));
				printf("rdma_connect %d\n", ret);
				return 1;
		}

		printf("connect succeed\n");

		if (id->event->param.conn.private_data_len < sizeof(struct common_priv_data)) {
				printf("transfer error!\n");
				return 1;
		}

		priv_data = id->event->param.conn.private_data;
		printf("Remote Buffer: %lld : length %zdBytes : rkey %d\n",
						priv_data->buffer_addr, priv_data->buffer_length,
						priv_data->buffer_rkey);

		uint64_t raddr = priv_data->buffer_addr;
		uint32_t rkey = priv_data->buffer_rkey;

		for (i = 0; i < iter_num; i ++ ) {
				wr_msg[11] = 'a' + i % 26;
				ret = rdma_post_write(id, NULL, wr_msg, 16, mr, 0, raddr, rkey);
				if (ret) {
						printf("Error code %s\n", strerror(errno));
						printf("rdma_get_rec_comp %d\n", ret);
						return 1;
				}
				ret = rdma_post_read(id, NULL, wr_msg + 16, 16, mr, 0, raddr, rkey);
				if (ret) {
						printf("Rounf %d error\n", i);
						printf("Error code %s\n", strerror(errno));
						printf("rdma_get_rec_comp %d\n", ret);
						return 1;
				}
				if (i % 1000 == 0) {
						printf("msg is : %s\n", wr_msg);
				}
//				printf("inf(after):%s\n", recv_msg);
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
