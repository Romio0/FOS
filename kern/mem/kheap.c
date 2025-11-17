#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include <kern/conc/sleeplock.h>
#include <kern/proc/user_environment.h>
#include <kern/mem/memory_manager.h>
#include "../conc/kspinlock.h"


// --- Definitions ---
struct AllocHeader {
    uint32 size_in_pages;
    uint32 magic;
};
#define ALLOC_MAGIC 0x4B484550

// --- Free Block List structures ---
struct FreeBlock {
    uint32 start_va;
    uint32 size_in_pages;
    LIST_ENTRY(FreeBlock) prev_next_info;
};
static LIST_HEAD(FreeBlockList, FreeBlock) free_block_list;

// --- Spinlock ---
static struct kspinlock kheap_lock;
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//==============================================
// [1] INITIALIZE KERNEL HEAP:
//==============================================
//TODO: [PROJECT'25.GM#2] KERNEL HEAP - #0 kheap_init [GIVEN]
//Remember to initialize locks (if any)
void kheap_init()
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		initialize_dynamic_allocator(KERNEL_HEAP_START, KERNEL_HEAP_START + DYN_ALLOC_MAX_SIZE);
		set_kheap_strategy(KHP_PLACE_CUSTOMFIT);
		kheapPageAllocStart = dynAllocEnd + PAGE_SIZE;
		kheapPageAllocBreak = kheapPageAllocStart;
	}
	//==================================================================================
	//==================================================================================
}

//==============================================
// [2] GET A PAGE FROM THE KERNEL FOR DA:
//==============================================
int get_page(void* va)
{
	int ret = alloc_page(ptr_page_directory, ROUNDDOWN((uint32)va, PAGE_SIZE), PERM_WRITEABLE, 1);
	if (ret < 0)
		panic("get_page() in kern: failed to allocate page from the kernel");
	return 0;
}

//==============================================
// [3] RETURN A PAGE FROM THE DA TO KERNEL:
//==============================================
void return_page(void* va)
{
	unmap_frame(ptr_page_directory, ROUNDDOWN((uint32)va, PAGE_SIZE));
}

//==================================================================================//
//============================ HELPER FUNCTIONS ==================================//
//==================================================================================//

static void* find_exact_fit(uint32 size_in_bytes)
{
    uint32 required_pages = size_in_bytes / PAGE_SIZE;
    struct FreeBlock *current_block;

    LIST_FOREACH(current_block, &free_block_list) {
        if (current_block->size_in_pages == required_pages) {
            uint32 found_va = current_block->start_va;
            LIST_REMOVE(&free_block_list, current_block);
            free_block(current_block);
            return (void*)found_va;
        }
    }
    return NULL;
}

static void* find_worst_fit(uint32 size_in_bytes)
{
    uint32 required_pages = size_in_bytes / PAGE_SIZE;
    struct FreeBlock *current_block;
    struct FreeBlock *biggest_block_found = NULL;

    LIST_FOREACH(current_block, &free_block_list) {
        if (current_block->size_in_pages >= required_pages) {
            if (biggest_block_found == NULL || current_block->size_in_pages > biggest_block_found->size_in_pages) {
                biggest_block_found = current_block;
            }
        }
    }

    if (biggest_block_found != NULL) {
        uint32 found_va = biggest_block_found->start_va;
        if (biggest_block_found->size_in_pages == required_pages) {
            LIST_REMOVE(&free_block_list, biggest_block_found);
           free_block(biggest_block_found);
        } else {
            biggest_block_found->start_va = biggest_block_found->start_va + size_in_bytes;
            biggest_block_found->size_in_pages = biggest_block_found->size_in_pages - required_pages;
        }

        return (void*)found_va;
    }

    return NULL;
}

static void* extend_the_break(uint32 size_in_bytes)
{
    void* new_block_va = NULL;

    if (kheapPageAllocBreak + size_in_bytes <= KERNEL_HEAP_MAX) {
        new_block_va = (void*)kheapPageAllocBreak;
        kheapPageAllocBreak = kheapPageAllocBreak + size_in_bytes;
        return new_block_va;
    } else {
        return NULL;
    }
}
//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===================================
// [1] ALLOCATE SPACE IN KERNEL HEAP:
//===================================
void* kmalloc(unsigned int size)
{
    //TODO: [PROJECT'25.GM#2] KERNEL HEAP - #1 kmalloc

    if (size == 0) return NULL;

    uint32 el_hagm_el_feli = size;
    if (size > DYN_ALLOC_MAX_BLOCK_SIZE) {
        el_hagm_el_feli += sizeof(struct AllocHeader);
    }

    if (el_hagm_el_feli <= DYN_ALLOC_MAX_BLOCK_SIZE) {
        return alloc_block(size);
    } else {
        acquire_lock(&kheap_lock); // Use 'acquire_lock'

        uint32 el_hagm_el_mohazab = ROUNDUP(el_hagm_el_feli, PAGE_SIZE);
        void* el_address_el_mokhasas_ma_el_header = NULL;

        // find_exact_fit

        uint32 el_safahat_el_mazbota = el_hagm_el_mohazab / PAGE_SIZE;
        struct FreeBlock *el_block_el_hali;
        LIST_FOREACH(el_block_el_hali, &free_block_list) {
            if (el_block_el_hali->size_in_pages == el_safahat_el_mazbota) {
                uint32 el_address_el_mawgod = el_block_el_hali->start_va;
                LIST_REMOVE(&free_block_list, el_block_el_hali);
                free_block(el_block_el_hali);
                el_address_el_mokhasas_ma_el_header = (void*)el_address_el_mawgod;
                break;
            }
        }


        if (el_address_el_mokhasas_ma_el_header == NULL) {
            // find_worst_fit
            struct FreeBlock *el_block_el_akbar = NULL;
            LIST_FOREACH(el_block_el_hali, &free_block_list) {
                if (el_block_el_hali->size_in_pages >= el_safahat_el_mazbota) {
                    if (el_block_el_akbar == NULL || el_block_el_hali->size_in_pages > el_block_el_akbar->size_in_pages) {
                        el_block_el_akbar = el_block_el_hali;
                    }
                }
            }

            if (el_block_el_akbar != NULL) {
                uint32 el_address_el_mawgod = el_block_el_akbar->start_va;
                if (el_block_el_akbar->size_in_pages == el_safahat_el_mazbota) {
                    LIST_REMOVE(&free_block_list, el_block_el_akbar);
                    free_block(el_block_el_akbar);
                } else {
                    el_block_el_akbar->start_va = el_block_el_akbar->start_va + el_hagm_el_mohazab;
                    el_block_el_akbar->size_in_pages = el_block_el_akbar->size_in_pages - el_safahat_el_mazbota;
                }
                el_address_el_mokhasas_ma_el_header = (void*)el_address_el_mawgod;
            }

        }

        if (el_address_el_mokhasas_ma_el_header == NULL) {
            //  extend_the_break
            void* el_address_el_gded = NULL;
            if (kheapPageAllocBreak + el_hagm_el_mohazab <= KERNEL_HEAP_MAX) {
                el_address_el_gded = (void*)kheapPageAllocBreak;
                kheapPageAllocBreak = kheapPageAllocBreak + el_hagm_el_mohazab;
                el_address_el_mokhasas_ma_el_header = el_address_el_gded;
            } else {
                el_address_el_mokhasas_ma_el_header = NULL;
            }

        }

        if (el_address_el_mokhasas_ma_el_header == NULL) {
            release_lock(&kheap_lock); // Use 'release_lock'
            return NULL;
        }

        for (uint32 el_va_el_hali = (uint32)el_address_el_mokhasas_ma_el_header; el_va_el_hali < (uint32)el_address_el_mokhasas_ma_el_header + el_hagm_el_mohazab; el_va_el_hali += PAGE_SIZE) {
            if (get_page((void*)el_va_el_hali) < 0) {
                release_lock(&kheap_lock); // Use 'release_lock'
                panic("kmalloc: Failed to allocate physical page - out of memory!");
            }
        }

        struct AllocHeader* el_header = (struct AllocHeader*)el_address_el_mokhasas_ma_el_header;
        el_header->size_in_pages = el_hagm_el_mohazab / PAGE_SIZE;
        el_header->magic = ALLOC_MAGIC;

        release_lock(&kheap_lock); // Use 'release_lock'

        return (void*)((uint32)el_address_el_mokhasas_ma_el_header + sizeof(struct AllocHeader));
    }
    //TODO: [PROJECT'25.BONUS#3] FAST PAGE ALLOCATOR
}//=================================
// [2] FREE SPACE FROM KERNEL HEAP:
//=================================
void kfree(void* virtual_address)
{
	//TODO: [PROJECT'25.GM#2] KERNEL HEAP - #2 kfree
	//Your code is here

	 if (virtual_address == NULL) return;

		// if it invalid address
	    if ((uint32)virtual_address < kheapPageAllocStart || (uint32)virtual_address >= KERNEL_HEAP_MAX)
	    {
	    	 panic("kfree: Invalid address - not in any allocator range!");
	    }

	   // if pages in block allocator
	if((uint32)virtual_address>=dynAllocStart && (uint32)virtual_address<dynAllocEnd)
	{
		free_block(virtual_address);
		return;
	}


    	// if it in page allocate

    	spinlock_acquire(&kheap_lock);

    struct AllocHeader* header = (struct AllocHeader*)(virtual_address - sizeof(struct AllocHeader));

    if (header->magic != ALLOC_MAGIC)
    {
          spinlock_release(&kheap_lock);
          panic("kfree: Invalid allocation header - corrupted memory or double-free!");
    }


       uint32 num_pages = header->size_in_pages;
       uint32 block_start_va = (uint32)header;
       uint32 total_size_bytes = num_pages * PAGE_SIZE;

       for (uint32 fva = block_start_va; fva < block_start_va + total_size_bytes; fva += PAGE_SIZE)
       {
           return_page((void*)fva);
       }

       struct FreeBlock* new_free_block = (struct FreeBlock*)alloc_block(sizeof(struct FreeBlock));
       if (new_free_block == NULL)
       {
           spinlock_release(&kheap_lock);
           panic("kfree: Out of memory - cannot allocate FreeBlock structure!");
       }

       new_free_block->start_va = block_start_va;
       new_free_block->size_in_pages = num_pages;

       struct FreeBlock *current_block;
       struct FreeBlock *prev_block = NULL;

       LIST_FOREACH(current_block, &free_block_list)
    	{
           if (current_block->start_va > block_start_va)
           {
               break; // find the correct position needed
           }
           prev_block = current_block;
       }
       if (prev_block == NULL)
       {
              //if it in the first position
              LIST_INSERT_HEAD(&free_block_list, new_free_block);
       }
       else

       {
              // Insert AFTER prev_block
              LIST_INSERT_AFTER(&free_block_list, prev_block, new_free_block);
       }

       coalesce_free_blocks();

       // handle unused space
       if (block_start_va + total_size_bytes == kheapPageAllocBreak)
       {
               kheapPageAllocBreak = block_start_va;
               LIST_REMOVE(&free_block_list, new_free_block);
               free_block(new_free_block);
       }

       spinlock_release(&kheap_lock);

	//Comment the following line
	//panic("kfree() is not implemented yet...!!");
}

//=================================
// [3] FIND VA OF GIVEN PA:
//=================================
unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'25.GM#2] KERNEL HEAP - #3 kheap_virtual_address
	//Your code is here
	  struct FrameInfo *frame_info = to_frame_info(physical_address);

	    if (frame_info == NULL || frame_info->va == 0)
	        return 0;

	    uint32 offset = PGOFF(physical_address);
	    return frame_info->va + offset;
	//Comment the following line
	panic("kheap_virtual_address() is not implemented yet...!!");

	/*EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED */
}

//=================================
// [4] FIND PA OF GIVEN VA:
//=================================
unsigned int kheap_physical_address(unsigned int va)
{
	//TODO: [PROJECT'25.GM#2] KERNEL HEAP - #4 kheap_physical_address
	//Your code is here

	uint32 *ptr_pageTable =NULL;
	struct FrameInfo *elFrame=get_frame_info(ptr_page_directory,(void *)va,&ptr_pageTable);

	 // If not mapped, return 0
	    if (elFrame == NULL)
	        return 0;

	    uint32 offset = PGOFF(va);
	    uint32 physical_address = to_physical_address(elFrame)+ offset ;

	    return physical_address;


	//Comment the following line
	panic("kheap_physical_address() is not implemented yet...!!");

	/*EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED */
}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

extern __inline__ uint32 get_block_size(void *va);

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'25.BONUS#2] KERNEL REALLOC - krealloc
	//Your code is here
	//Comment the following line
	panic("krealloc() is not implemented yet...!!");
}
