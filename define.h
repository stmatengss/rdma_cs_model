
const int iter_num = 10000;

#include<time.h>

#define time_sec (time((time_t *)NULL))
#define PRINT_TIME printf("time: %Lf\n", (long double)clock());
#define PRINT_LINE printf("line: %d\n", __LINE__);
#define MESSAGE_LEN 64 
