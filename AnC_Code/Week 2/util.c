#include "util.h"



char* compose_pte_address(char* pt_base, unsigned long offset)
{
	return pt_base + offset;
}
char* extract_pte_value(char* pte_address)
{
	uint64_t pte_value = *((uint64_t*)pte_address)&PAGE_ALIGN_MASK;
	return ((char*)pte_value);
}
unsigned long int extract_pte_offset(char* virtual_address, int pt_level)
{
	unsigned long int address = (unsigned long int)virtual_address;
	address = address>>(PAGE_BITS+(PT_LEVELS-pt_level)*PAGE_TABLE_BITS);
	address = address & PAGE_TABLE_MASK;
	address = address<<3;
	
	return address;
}

char* allocate_1TB()
{
	char* addr = mmap(NULL, TB, PROT_READ, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

	if(addr == MAP_FAILED)
	{
		perror("Mapping failed");
		exit(-1);
	}

	else
	{
		for (int i = 0; i < 8; i++)
		{
			*(volatile char*) &addr[i];
		}
		return addr;
	}
}

int time_access(char* address)
{
	int cycles;

    asm volatile("mov %1, %%r8\n\t"
            "lfence\n\t"
            "rdtsc\n\t"
            "mov %%eax, %%edi\n\t"
            "mov (%%r8), %%r8\n\t"
            "lfence\n\t"
            "rdtsc\n\t"
            "sub %%edi, %%eax\n\t"
    : "=a"(cycles)
    : "r"(address)
    : "r8", "edi");

    return cycles;
}



char** page_walk(char* target_buffer, bool verbose)
{
	char read_buffer[9];

	int fd = open("/dev/mem", O_RDONLY);
	if(fd == -1)
	{
		perror("Could not open /dev/mem");
		exit(-1);
	}

	
	lseek(fd, 2*GB, SEEK_SET);
	read(fd, read_buffer, 8);
	if(verbose)
	{
		for (int i = 0; i < 8; i++)
		{
			printf("%uc ", read_buffer[i]);
		}
		printf("\n");
	}
	

	char *pt1_base, *pt2_base, *pt3_base, *pt4_base;
	char* pte_address;
	char *pt1_page_vaddr, *pt2_page_vaddr, *pt3_page_vaddr, *pt4_page_vaddr;
	char *pt1_entry_vaddr, *pt2_entry_vaddr, *pt3_entry_vaddr, *pt4_entry_vaddr;

	char* phys_page_address;
	char* phys_page_virt_address;
	char* phys_address;
	unsigned long int cr3;

	unsigned long int pt_level1_offset = extract_pte_offset(target_buffer, 1);
	unsigned long int pt_level2_offset = extract_pte_offset(target_buffer, 2);
	unsigned long int pt_level3_offset = extract_pte_offset(target_buffer, 3);
	unsigned long int pt_level4_offset = extract_pte_offset(target_buffer, 4);
	unsigned long int page_offset = ((uint64_t)target_buffer) & PAGE_MASK;
	if(verbose)
	{
		printf("L1 offset: %lx\n", pt_level1_offset);
		printf("L2 offset: %lx\n", pt_level2_offset);
		printf("L3 offset: %lx\n", pt_level3_offset);
		printf("L4 offset: %lx\n", pt_level4_offset);
		printf("Page offset: %lx\n", page_offset);
	}
	

	FILE* fptr = fopen("/proc/cr3", "r");
	fscanf(fptr, "%lX", &cr3);

	if(verbose)
		printf("CR3 contents:%lx\n", cr3);

	pt1_base = (char*)cr3;

	if(verbose)
		printf("-----------------------------------------------------------\n");

	pt1_page_vaddr = mmap(NULL, PAGE, PROT_READ, MAP_PRIVATE, fd, (off_t)pt1_base);
	if (pt1_page_vaddr == MAP_FAILED)
	{
		perror("Mapping level 1 failed");
		exit(-1);
	} 
	
	pt1_entry_vaddr = compose_pte_address(pt1_page_vaddr, pt_level1_offset);
	pt2_base = extract_pte_value(pt1_entry_vaddr);

	if(verbose)
	{
		printf("pt1_base: %p\n", pt1_base);
		printf("pt1_page_vaddr: %p\n", pt1_page_vaddr);
		printf("pt1_entry_vaddr: %p\n", pt1_entry_vaddr);
		printf("pt2_base: %p\n", pt2_base);
	}
	
	pt2_page_vaddr = mmap(NULL, PAGE, PROT_READ, MAP_SHARED, fd, (off_t)pt2_base);	
	if (pt2_page_vaddr == MAP_FAILED)
	{
		perror("Mapping level 2 failed");
		exit(-1);
	}

	pt2_entry_vaddr = compose_pte_address(pt2_page_vaddr, pt_level2_offset);
	pt3_base = extract_pte_value(pt2_entry_vaddr);

	if(verbose)
	{
		printf("pt2_page_vaddr: %p\n", pt2_page_vaddr);
		printf("pt2_entry_vaddr: %p\n", pt2_entry_vaddr);
		printf("pt3_base: %p\n", pt3_base);	
	}
	

	pt3_page_vaddr = mmap(NULL, PAGE, PROT_READ, MAP_SHARED, fd, (off_t)pt3_base);	
	if (pt3_page_vaddr == MAP_FAILED)
	{
		perror("Mapping level 3 failed");
		exit(-1);
	}

	pt3_entry_vaddr = compose_pte_address(pt3_page_vaddr, pt_level3_offset);
	pt4_base = extract_pte_value(pt3_entry_vaddr);

	if(verbose)
	{
		printf("pt3_page_vaddr: %p\n", pt3_page_vaddr);
		printf("pt3_entry_vaddr: %p\n", pt3_entry_vaddr);
		printf("pt4_base: %p\n", pt4_base);
	}
	

	pt4_page_vaddr = mmap(NULL, PAGE, PROT_READ, MAP_SHARED, fd, (off_t)pt4_base);	
	if (pt4_page_vaddr == MAP_FAILED)
	{
		perror("Mapping level 4 failed");
		exit(-1);
	}

	pt4_entry_vaddr = compose_pte_address(pt4_page_vaddr, pt_level4_offset);
	phys_page_address = extract_pte_value(pt4_entry_vaddr);

	if(verbose)
	{
		printf("pt4_page_vaddr: %p\n", pt3_page_vaddr);
		printf("pt4_entry_vaddr: %p\n", pt3_entry_vaddr);
		printf("phys_page_address: %p\n", phys_page_address);
	}
	

	phys_address = phys_page_address + page_offset;
	if(verbose)
	{
		printf("Physical address: %p\n", phys_address);
	}
	
	char** tlb_entries = (char**) malloc(PT_LEVELS*sizeof(char*));
	tlb_entries[0] = pt1_entry_vaddr;
	tlb_entries[1] = pt2_entry_vaddr;
	tlb_entries[2] = pt3_entry_vaddr;
	tlb_entries[3] = pt4_entry_vaddr;

	return tlb_entries;
}

void flush_tlb(char* target_buffer)
{
	unsigned long int stride = PAGE;
	char* starting_flush_addr = target_buffer + CACHE_LINE_SIZE;
	_mm_lfence();
	_mm_mfence();

	for (int i = 0; i < 3*PL1_ENTRIES; i++)
	{
		*(volatile char*) ((unsigned long int) starting_flush_addr + i*stride);
	}
	stride <<= 9;
	_mm_lfence();
	_mm_mfence();

	for (int i = 0; i < 30*PL2_ENTRIES; i++)
	{
		*(volatile char*) ((unsigned long int) starting_flush_addr + i*stride);
	}
	stride <<= 9;
	_mm_lfence();
	_mm_mfence();

	for (int i = 0; i < 30*PL3_ENTRIES; i++)
	{
		*(volatile char*) ((unsigned long int) starting_flush_addr + i*stride);
	}
	_mm_lfence();
	_mm_mfence(); 

}

void demo_tlb_flush(char* target_buffer)
{
	int time_1[PT_LEVELS], time_2[PT_LEVELS];

	char** pt_entries = (char**) malloc(PT_LEVELS*sizeof(char*));
	pt_entries = page_walk(target_buffer, false);

	for (int i = 0; i < YIELD_ROUNDS; i++)
		sched_yield();

	printf("Level:\t\t\t1\t2\t3\t4\n");
	printf("-------------------------------------------------------\n");
	printf("Target Untouched:\t");

	flush_tlb(target_buffer);
	for (int i = 0; i < PT_LEVELS; i++)
	{
		_mm_clflush(pt_entries[i]);
	}

	for (int i = 0; i < PT_LEVELS; ++i)
	{
		time_1[i] = time_access(pt_entries[i]);
	}

	for (int i = 0; i < PT_LEVELS; ++i)
	{
		printf("%d\t", time_1[i]);
	}
	printf("\n");

	printf("Target Touched:\t\t");
	flush_tlb(target_buffer);
	for (int i = 0; i < PT_LEVELS; i++)
	{
		_mm_clflush(pt_entries[i]);
	}
	*((volatile char*) target_buffer);
	for (int i = 0; i < PT_LEVELS; ++i)
	{
		time_2[i] = time_access(pt_entries[i]);
	}

	for (int i = 0; i < PT_LEVELS; ++i)
	{
		printf("%d\t", time_2[i]);
	}
	printf("\n");

	// printf("No TLB Flush: %d\n", time_1);
	// printf("With TLB Flush: %d\n", time_2);

}