#include "util.h"

int main(int argc, char const *argv[])
{
	char* target_buffer = allocate_1TB();

	// page_walk(target_buffer, false);
	demo_tlb_flush(target_buffer);
	return 0;
}