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
#include <string.h>

#define TB 					1ULL<<40
#define GB 					1ULL<<30
#define PAGE 				4096
#define PAGE_TABLE_BITS		9
#define PAGE_TABLE_MASK		0x1FF
#define PAGE_BITS			12
#define PAGE_MASK			0xFFF
#define PHYS_ADDR_MASK		0xFFFFFFFFFFFF
#define PT_LEVELS			4
#define CACHE_LINE_SIZE		64
#define LLC_EVICT_PAGES   	8192

#define ROUNDS				100
#define THRESHOLD 			290

#define TCACHE_EVICT_ACCESS 50
#define PT1_STRIDE PAGE
#define PT2_STRIDE PAGE * 512L
#define PT3_STRIDE PAGE * 512L * 512L
#define PT4_STRIDE PAGE * 512L * 512L * 512L



char* allocate_32TB();
char* allocate_eviction_buffer();

int time_access(char* address);

bool setup_tcache_eviction_sets();
void evict_llc(int offset, char* llc_evict_buffer);

void derandomise(char* target_buffer, char* llc_eviction_buffer);
