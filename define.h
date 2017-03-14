
struct common_priv_data {

		uint64_t buffer_addr;
		uint32_t buffer_rkey;
		size_t   buffer_length;
};

const int iter_num = 10;

#include<time.h>

typedef unsigned long long ll;

#define LL_LEN 8
#define time_sec (time((time_t *)NULL))
#define PRINT_TIME printf("time: %Lf\n", (long double)clock());
#define PRINT_LINE printf("line: %d\n", __LINE__);
#define MESSAGE_LEN 64 
