#include <time.h>
int json_c_get_random_seed(void){
	return (int)time(NULL) * 433494437;
}
