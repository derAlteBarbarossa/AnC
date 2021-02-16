#include "util.h"

int main(int argc, char const *argv[])
{
	char* target_buffer = allocate_1TB();

	demo_tlb_flush(target_buffer);
	return 0;
}