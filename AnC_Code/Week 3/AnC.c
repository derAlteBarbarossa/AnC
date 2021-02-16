#include "util.h"

int main(int argc, char const *argv[])
{
	char* target_buffer = allocate_32TB();
	char* llc_eviction_buffer = allocate_eviction_buffer();
	

	derandomise(target_buffer, llc_eviction_buffer);
	return 0;
}