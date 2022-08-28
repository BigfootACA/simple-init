#include"zipint.h"
#include<stdlib.h>

ZIP_EXTERN bool zip_secure_random(zip_uint8_t*buffer,zip_uint16_t length){
	static bool seed=false;
	if(!seed){
		srand((unsigned int)time(NULL));
		seed=true;
	}
	for(zip_uint16_t i=0;i<length;i++)
		buffer[i]=(zip_uint8_t)rand();
	return true;
}

zip_uint32_t zip_random_uint32(void){
	zip_uint32_t v;
	zip_secure_random((zip_uint8_t*)&v,sizeof(v));
	return v;
}
