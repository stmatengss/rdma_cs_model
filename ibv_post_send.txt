## Description

ibv_post_send() posts a linked list of Work Requests (WRs) to the Send Queue of a Queue Pair (QP). ibv_post_send() go over all of the entries in the linked list, one by one, check that it is valid, generate a HW-specific Send Request out of it and add it to the tail of the QP's Send Queue without performing any context switch. The RDMA device will handle it (later) in asynchronous way. If there is a failure in one of the WRs because the Send Queue is full or one of the attributes in the WR is bad, it stops immediately and return the pointer to that WR. The QP will handle Work Requests in the Send queue according to the following rules:

If the QP is in RESET, INIT or RTR state an immediate error should be returned. However, they may be some low-level driver that won't follow this rule (to eliminate extra check in the data path, thus providing better performance) and posting Send Requests at one or all of those states may be silently ignored.
If the QP is in RTS state, Send Requests can be posted and they will be processed.
If the QP is in SQE or ERROR state, Send Requests can be posted and they will be completed with error.
If the QP is in SQD state, Send Requests can be posted, but they won't be processed.
The struct ibv_send_wr describes the Work Request to the Send Queue of the QP, i.e. Send Request (SR).

struct ibv_send_wr {
	uint64_t		wr_id;
	struct ibv_send_wr     *next;
	struct ibv_sge	       *sg_list;
	int			num_sge;
	enum ibv_wr_opcode	opcode;
	int			send_flags;
	uint32_t		imm_data;
	union {
		struct {
			uint64_t	remote_addr;
			uint32_t	rkey;
		} rdma;
		struct {
			uint64_t	remote_addr;
			uint64_t	compare_add;
			uint64_t	swap;
			uint32_t	rkey;
		} atomic;
		struct {
			struct ibv_ah  *ah;
			uint32_t	remote_qpn;
			uint32_t	remote_qkey;
		} ud;
	} wr;
};
## Here is the full description of struct ibv_send_wr:

wr_id	A 64 bits value associated with this WR. If a Work Completion will be generated when this Work Request ends, it will contain this value
next	Pointer to the next WR in the linked list. NULL indicates that this is the last WR
sg_list	Scatter/Gather array, as described in the table below. It specifies the buffers that will be read from or the buffers where data will be written in, depends on the used opcode. The entries in the list can specify memory blocks that were registered by different Memory Regions. The message size is the sum of all of the memory buffers length in the scatter/gather list
num_sge	Size of the sg_list array. This number can be less or equal to the number of scatter/gather entries that the Queue Pair was created to support in the Send Queue (qp_init_attr.cap.max_send_sge). If this size is 0, this indicates that the message size is 0
opcode	The operation that this WR will perform. This value controls the way that data will be sent, the direction of the data flow and the used attributes in the WR. The value can be one of the following enumerated values:
IBV_WR_SEND - The content of the local memory buffers specified in sg_list is being sent to the remote QP. The sender doesn’t know where the data will be written in the remote node. A Receive Request will be consumed from the head of remote QP's Receive Queue and sent data will be written to the memory buffers which are specified in that Receive Request. The message size can be [0, 2^{31}] for RC and UC QPs and [0, path MTU] for UD QP
IBV_WR_SEND_WITH_IMM - Same as IBV_WR_SEND, and immediate data will be sent in the message. This value will be available in the Work Completion that will be generated for the consumed Receive Request in the remote QP
IBV_WR_RDMA_WRITE - The content of the local memory buffers specified in sg_list is being sent and written to a contiguous block of memory range in the remote QP's virtual space. This doesn't necessarily means that the remote memory is physically contiguous. No Receive Request will be consumed in the remote QP. The message size can be [0, 2^{31}]
IBV_WR_RDMA_WRITE_WITH_IMM - Same as IBV_WR_RDMA_WRITE, but Receive Request will be consumed from the head of remote QP's Receive Queue and immediate data will be sent in the message. This value will be available in the Work Completion that will be generated for the consumed Receive Request in the remote QP
IBV_WR_RDMA_READ - Data is being read from a contiguous block of memory range in the remote QP's virtual space and being written to the local memory buffers specified in sg_list. No Receive Request will be consumed in the remote QP. The message size can be [0, 2^{31}]
IBV_WR_ATOMIC_FETCH_AND_ADD - A 64 bits value in a remote QP's virtual space is being read, added to wr.atomic.compare_add and the result is being written to the same memory address, in an atomic way. No Receive Request will be consumed in the remote QP. The original data, before the add operation, is being written to the local memory buffers specified in sg_list
IBV_WR_ATOMIC_CMP_AND_SWP - A 64 bits value in a remote QP's virtual space is being read, compared with wr.atomic.compare_add and if they are equal, the value wr.atomic.swap is being written to the same memory address, in an atomic way. No Receive Request will be consumed in the remote QP. The original data, before the compare operation, is being written to the local memory buffers specified in sg_list
send_flags	Describes the properties of the WR. It is either 0 or the bitwise OR of one or more of the following flags:
IBV_SEND_FENCE - Set the fence indicator for this WR. This means that the processing of this WR will be blocked until all prior posted RDMA Read and Atomic WRs will be completed. Valid only for QPs with Transport Service Type IBV_QPT_RC
IBV_SEND_SIGNALED - Set the completion notification indicator for this WR. This means that if the QP was created with sq_sig_all=0, a Work Completion will be generated when the processing of this WR will be ended. If the QP was created with sq_sig_all=1, there won't be any effect to this flag
IBV_SEND_SOLICITED - Set the solicited event indicator for this WR. This means that when the message in this WR will be ended in the remote QP, a solicited event will be created to it and if in the remote side the user is waiting for a solicited event, it will be woken up. Relevant only for the Send and RDMA Write with immediate opcodes
IBV_SEND_INLINE - The memory buffers specified in sg_list will be placed inline in the Send Request. This mean that the low-level driver (i.e. CPU) will read the data and not the RDMA device. This means that the L_Key won't be checked, actually those memory buffers don't even have to be registered and they can be reused immediately after ibv_post_send() will be ended. Valid only for the Send and RDMA Write opcodes
imm_data	(optional) A 32 bits number, in network order, in an SEND or RDMA WRITE opcodes that is being sent along with the payload to the remote side and placed in a Receive Work Completion and not in a remote memory buffer
wr.rdma.remote_addr	Start address of remote memory block to access (read or write, depends on the opcode). Relevant only for RDMA WRITE (with immediate) and RDMA READ opcodes
wr.rdma.rkey	r_key of the Memory Region that is being accessed at the remote side. Relevant only for RDMA WRITE (with immediate) and RDMA READ opcodes
wr.atomic.remote_addr	Start address of remote memory block to access
wr.atomic.compare_add	For Fetch and Add: the value that will be added to the content of the remote address. For compare and swap: the value to be compared with the content of the remote address. Relevant only for atomic operations
wr.atomic.swap	Relevant only for compare and swap: the value to be written in the remote address if the value that was read is equal to the value in wr.atomic.compare_add. Relevant only for atomic operations
wr.atomic.rkey	r_key of the Memory Region that is being accessed at the remote side. Relevant only for atomic operations
wr.ud.ah	Address handle (AH) that describes how to send the packet. This AH must be valid until any posted Work Requests that uses it isn't considered outstanding anymore. Relevant only for UD QP
wr.ud.remote_qpn	QP number of the destination QP. The value 0xFFFFFF indicated that this is a message to a multicast group. Relevant only for UD QP
wr.ud.remote_qkey	Q_Key value of remote QP. Relevant only for UD QP
The following table describes the supported opcodes for each QP Transport Service Type:

Opcode	UD	UC	RC
IBV_WR_SEND	X	X	X
IBV_WR_SEND_WITH_IMM	X	X	X
IBV_WR_RDMA_WRITE		X	X
IBV_WR_RDMA_WRITE_WITH_IMM		X	X
IBV_WR_RDMA_READ			X
IBV_WR_ATOMIC_CMP_AND_SWP			X
IBV_WR_ATOMIC_FETCH_AND_ADD			X
struct ibv_sge describes a scatter/gather entry. The memory buffer that this entry describes must be registered until any posted Work Request that uses it isn't considered outstanding anymore. The order in which the RDMA device access the memory in a scatter/gather list isn't defined. This means that if some of the entries overlap the same memory address, the content of this address is undefined.

struct ibv_sge {
	uint64_t		addr;
	uint32_t		length;
	uint32_t		lkey;
};
Here is the full description of struct ibv_sge:

addr	The address of the buffer to read from or write to
length	The length of the buffer in bytes. The value 0 is a special value and is equal to 2^{31} bytes (and not zero bytes, as one might imagine)
lkey	The Local key of the Memory Region that this memory buffer was registered with
Sending inline'd data is an implementation extension that isn't defined in any RDMA specification: it allows send the data itself in the Work Request (instead the scatter/gather entries) that is posted to the RDMA device. The memory that holds this message doesn't have to be registered. There isn't any verb that specifies the maximum message size that can be sent inline'd in a QP. Some of the RDMA devices support it. In some RDMA devices, creating a QP with will set the value of max_inline_data to the size of messages that can be sent using the requested number of scatter/gather elements of the Send Queue. If others, one should specify explicitly the message size to be sent inline before the creation of a QP. for those devices, it is advised to try to create the QP with the required message size and continue to decrease it if the QP creation fails. While a WR is considered outstanding:

If the WR sends data, the local memory buffers content shouldn't be changed since one doesn't know when the RDMA device will stop reading from it (one exception is inline data)
If the WR reads data, the local memory buffers content shouldn't be read since one doesn't know when the RDMA device will stop writing new content to it
Parameters

Name	Direction	Description
qp	in	Queue Pair that was returned from ibv_create_qp()
wr	in	Linked list of Work Requests to be posted to the Send Queue of the Queue Pair
bad_wr	out	A pointer to that will be filled with the first Work Request that its processing failed
Return Values

Value	Description
0	On success
errno	On failure and no change will be done to the QP and bad_wr points to the SR that failed to be posted
EINVAL	Invalid value provided in wr
ENOMEM	Send Queue is full or not enough resources to complete this operation
EFAULT	Invalid value provided in qp
## Examples

1) Posting a WR with the Send operation to an UC or RC QP:

struct ibv_sge sg;
struct ibv_send_wr wr;
struct ibv_send_wr *bad_wr;
 
memset(&sg, 0, sizeof(sg));
sg.addr	  = (uintptr_t)buf_addr;
sg.length = buf_size;
sg.lkey	  = mr->lkey;
 
memset(&wr, 0, sizeof(wr));
wr.wr_id      = 0;
wr.sg_list    = &sg;
wr.num_sge    = 1;
wr.opcode     = IBV_WR_SEND;
wr.send_flags = IBV_SEND_SIGNALED;
 
if (ibv_post_send(qp, &wr, &bad_wr)) {
	fprintf(stderr, "Error, ibv_post_send() failed\n");
	return -1;
}

2) Posting a WR with the Send with immediate operation to an UD QP:

struct ibv_sge sg;
struct ibv_send_wr wr;
struct ibv_send_wr *bad_wr;
 
memset(&sg, 0, sizeof(sg));
sg.addr	  = (uintptr_t)buf_addr;
sg.length = buf_size;
sg.lkey	  = mr->lkey;
 
memset(&wr, 0, sizeof(wr));
wr.wr_id      = 0;
wr.sg_list    = &sg;
wr.num_sge    = 1;
wr.opcode     = IBV_WR_SEND_WITH_IMM;
wr.send_flags = IBV_SEND_SIGNALED;
wr.imm_data   = htonl(0x1234);
wr.wr.ud.ah          = ah;
wr.wr.ud.remote_qpn  = remote_qpn;
wr.wr.ud.remote_qkey = 0x11111111;
 
if (ibv_post_send(qp, &wr, &bad_wr)) {
	fprintf(stderr, "Error, ibv_post_send() failed\n");
	return -1;
}

3) Posting a WR with an RDMA Write operation to an UC or RC QP:

struct ibv_sge sg;
struct ibv_send_wr wr;
struct ibv_send_wr *bad_wr;
 
memset(&sg, 0, sizeof(sg));
sg.addr	  = (uintptr_t)buf_addr;
sg.length = buf_size;
sg.lkey	  = mr->lkey;
 
memset(&wr, 0, sizeof(wr));
wr.wr_id      = 0;
wr.sg_list    = &sg;
wr.num_sge    = 1;
wr.opcode     = IBV_WR_RDMA_WRITE;
wr.send_flags = IBV_SEND_SIGNALED;
wr.wr.rdma.remote_addr = remote_address
wr.wr.rdma.rkey        = remote_key;
 
if (ibv_post_send(qp, &wr, &bad_wr)) {
	fprintf(stderr, "Error, ibv_post_send() failed\n");
	return -1;
}

4) Posting a WR with an RDMA Write with immediate operation to an UC or RC QP:

struct ibv_sge sg;
struct ibv_send_wr wr;
struct ibv_send_wr *bad_wr;
 
memset(&sg, 0, sizeof(sg));
sg.addr	  = (uintptr_t)buf_addr;
sg.length = buf_size;
sg.lkey	  = mr->lkey;
 
memset(&wr, 0, sizeof(wr));
wr.wr_id      = 0;
wr.sg_list    = &sg;
wr.num_sge    = 1;
wr.opcode     = IBV_WR_RDMA_WRITE_WITH_IMM;
wr.send_flags = IBV_SEND_SIGNALED;
wr.imm_data   = htonl(0x1234);
wr.wr.rdma.remote_addr = remote_address
wr.wr.rdma.rkey        = remote_key;
 
if (ibv_post_send(qp, &wr, &bad_wr)) {
	fprintf(stderr, "Error, ibv_post_send() failed\n");
	return -1;
}

5) Posting a WR with an RDMA Read operation to a RC QP:

struct ibv_sge sg;
struct ibv_send_wr wr;
struct ibv_send_wr *bad_wr;
 
memset(&sg, 0, sizeof(sg));
sg.addr	  = (uintptr_t)buf_addr;
sg.length = buf_size;
sg.lkey	  = mr->lkey;
 
memset(&wr, 0, sizeof(wr));
wr.wr_id      = 0;
wr.sg_list    = &sg;
wr.num_sge    = 1;
wr.opcode     = IBV_WR_RDMA_READ;
wr.send_flags = IBV_SEND_SIGNALED;
wr.wr.rdma.remote_addr = remote_address
wr.wr.rdma.rkey        = remote_key;
 
if (ibv_post_send(qp, &wr, &bad_wr)) {
	fprintf(stderr, "Error, ibv_post_send() failed\n");
	return -1;
}

6) Posting a WR with a Compare and Swap operation to a RC QP:

struct ibv_sge sg;
struct ibv_send_wr wr;
struct ibv_send_wr *bad_wr;
 
memset(&sg, 0, sizeof(sg));
sg.addr	  = (uintptr_t)buf_addr;
sg.length = buf_size;
sg.lkey	  = mr->lkey;
 
memset(&wr, 0, sizeof(wr));
wr.wr_id      = 0;
wr.sg_list    = &sg;
wr.num_sge    = 1;
wr.opcode     = IBV_WR_ATOMIC_CMP_AND_SWP;
wr.send_flags = IBV_SEND_SIGNALED;
wr.wr.atomic.remote_addr = remote_address
wr.wr.atomic.rkey        = remote_key;
wr.wr.atomic.compare_add = 0ULL; /* expected value in remote address */
wr.wr.atomic.swap        = 1ULL; /* the value that remote address will be assigned to */
 
if (ibv_post_send(qp, &wr, &bad_wr)) {
	fprintf(stderr, "Error, ibv_post_send() failed\n");
	return -1;
}

7) Posting a WR with a Fetch and Add operation to a RC QP:

struct ibv_sge sg;
struct ibv_send_wr wr;
struct ibv_send_wr *bad_wr;
 
memset(&sg, 0, sizeof(sg));
sg.addr	  = (uintptr_t)buf_addr;
sg.length = buf_size;
sg.lkey	  = mr->lkey;
 
memset(&wr, 0, sizeof(wr));
wr.wr_id      = 0;
wr.sg_list    = &sg;
wr.num_sge    = 1;
wr.opcode     = IBV_WR_ATOMIC_FETCH_AND_ADD;
wr.send_flags = IBV_SEND_SIGNALED;
wr.wr.atomic.remote_addr = remote_address
wr.wr.atomic.rkey        = remote_key;
wr.wr.atomic.compare_add = 1ULL; /* value to be added to the remote address content */
 
if (ibv_post_send(qp, &wr, &bad_wr)) {
	fprintf(stderr, "Error, ibv_post_send() failed\n");
	return -1;
}
8) Posting a WR with the Send operation to an UC or RC QP with zero bytes:

struct ibv_send_wr wr;
struct ibv_send_wr *bad_wr;
 
memset(&wr, 0, sizeof(wr));
wr.wr_id      = 0;
wr.sg_list    = NULL;
wr.num_sge    = 0;
wr.opcode     = IBV_WR_SEND;
wr.send_flags = IBV_SEND_SIGNALED;
 
if (ibv_post_send(qp, &wr, &bad_wr)) {
	fprintf(stderr, "Error, ibv_post_send() failed\n");
	return -1;
}
FAQs

Does ibv_post_send() cause a context switch?

No. Posting a SR doesn't cause a context switch at all; this is why RDMA technologies can achieve very low latency (below 1 usec).

How many WRs can I post?

There is a limit to the maximum number of outstanding WRs for a QP. This value was specified when the QP was created.

Can I know how many WRs are outstanding in a Work Queue?

No, you can't. You should keep track of the number of outstanding WRs according to the number of posted WRs and the number of Work Completions that you polled.

Does the remote side is aware of the fact that RDMA operations are being performed in its memory?

No, this is the idea of RDMA.

If the remote side isn't aware of RDMA operations are being performed in its memory, isn't this a security hole?

Actually, no. For several reasons:

In order to allow incoming RDMA operations to a QP, the QP should be configured to enable remote operations
In order to allow incoming RDMA access to a MR, the MR should be registered with those remote permissions enabled
The remote side must know the r_key and the memory addresses in order to be able to access remote memory
What will happen if I will deregister an MR that is used by an outstanding WR?

When processing a WR, if one of the MRs that are specified in the WR isn't valid, a Work Completion with error will be generated. The only exception for this is posting inline data.

What is the benefit from using IBV_SEND_INLINE?

Using inline data usually provides better performance (i.e. latency).

What is the difference between inline data and immediate data?

Using immediate data means that out of band data will be sent from the local QP to the remote QP: if this is an SEND opcode, this data will exist in the Work Completion, if this is a RDMA WRITE opcode, a WR will be consumed from the remote QP's Receive Queue. Inline data influence only the way that the RDMA device gets the data to send; The remote side isn't aware of the fact that it this WR was sent inline.

I called ibv_post_send() and I got segmentation fault, what happened?

There may be several reasons for this to happen:
1) At least one of the sg_list entries is in invalid address
2) In one of the posted SRs, IBV_SEND_INLINE is set in send_flags, but one of the buffers in sg_list is pointing to an illegal address
3) The value of next points to an invalid address
4) Error occurred in one of the posted SRs (bad value in the SR or full Work Queue) and the variable bad_wr is NULL
5) A UD QP is used and wr.ud.ah points to an invalid address

Help, I've posted and Send Request and it wasn't completed with a corresponding Work Completion. What happened?

In order to debug this kind of problem, one should do the following:

Verify that a Send Request was actually posted
Wait enough time, maybe a Work Completion will eventually be generated
Verify that the logical port state of the RDMA device is IBV_PORT_ACTIVE
Verify that the QP state is RTS
If this is an RC QP, verify that the timeout value that was configured in ibv_modify_qp() isn't 0 since if a packet will be dropped, this may lead to infinite timeout
If this is an RC QP, verify that the timeout and retry_cnt values combination that were configured in ibv_modify_qp() doesn't indicate that long time will pass before a Work Completion with IBV_WC_RETRY_EXC_ERR will be generated
If this is an RC QP, verify that the rnr_retry value that was configured in ibv_modify_qp() isn't 7 since this may lead to retry infinite time in case of RNR flow
If this is an RC QP, verify that the min_rnr_timer and rnr_retry values combination that were configured in ibv_modify_qp() doesn't indicate that long time will pass before a Work Completion with IBV_WC_RNR_RETRY_EXC_ERR will be generated
How can I send a zero bytes message?

In order to send a zero byes message, no matter what is the opcode, the num_sge must be set to zero.

Can I (re)use the Send Request after ibv_post_send() returned?

Yes. This verb translates the Send Request from the libibverbs abstraction to a HW-specific Send Request and you can (re)use both the Send Request and the s/g list within it.
