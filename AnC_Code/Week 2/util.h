#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h> 
#include <x86intrin.h>
#include <sched.h>
#include <stdbool.h>

#define TB 					1ULL<<40
#define GB 					1ULL<<30
#define PAGE 				1UL<<12
#define PAGE_TABLE_BITS		9
#define PAGE_TABLE_MASK		0x1FF
#define PAGE_BITS			12
#define PAGE_MASK			0xFFF
#define PHYS_ADDR_MASK		0xFFFFFFFFFFFF
#define PT_LEVELS			4
#define PAGE_ALIGN_MASK		0xFFFFFFFFF000
#define CACHE_LINE_SIZE		64
#define PL1_ENTRIES			1088			// Haswell L2 TLB Entries
#define PL2_ENTRIES			32
#define PL3_ENTRIES			4

#define ROUNDS				100
#define YIELD_ROUNDS		30

char* allocate_1TB();
unsigned long int extract_pte_offset(char* virtual_address, int pt_level);
char* compose_pte_address(char* pt_base, unsigned long offset);
char* extract_pte_value(char* pte);

int time_access(char* address);

char** page_walk(char* target_buffer, bool verbose);
void flush_tlb(char* target_buffer);
void demo_tlb_flush(char* target_buffer);