/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
//==================================
// [1] GET PAGE VA:
//==================================
__inline__ uint32 to_page_va(struct PageInfoElement *ptrPageInfo)
{
	//Get start VA of the page from the corresponding Page Info pointer
	int idxInPageInfoArr = (ptrPageInfo - pageBlockInfoArr);
	return dynAllocStart + (idxInPageInfoArr << PGSHIFT);
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
bool is_initialized = 0;
void initialize_dynamic_allocator(uint32 daStart, uint32 daEnd)
{
    //==================================================================================
    //DON'T CHANGE THESE LINES==========================================================
    //==================================================================================
    {
        assert(daEnd <= daStart + DYN_ALLOC_MAX_SIZE);
        is_initialized = 1;
    }
    //==================================================================================
    //==================================================================================
    //TODO: [PROJECT'25.GM#1] DYNAMIC ALLOCATOR - #1 initialize_dynamic_allocator
    //Your code is here

    //init the boundries(DA Limits)
    dynAllocStart = daStart;
    dynAllocEnd = daEnd;
    //============================================================
    //init pageBlockInfoArr(metadata) 0 & freepages init & freeBlockLists(ll that holds all 8B,16B,..)
    memset(pageBlockInfoArr, 0, sizeof(pageBlockInfoArr));

    LIST_INIT(&freePagesList);

    for (uint8 i = 0; i < (LOG2_MAX_SIZE - LOG2_MIN_SIZE + 1); i++)
    {
        LIST_INIT(&freeBlockLists[i]);
    }


    //============================================================
    uint16 noOfpages=(dynAllocEnd-dynAllocStart)/PAGE_SIZE;
    for (uint16 i=0;i<noOfpages;i++){
        struct PageInfoElement *pg=&pageBlockInfoArr[i];
        pg->block_size=0;
        pg->num_of_free_blocks=0;
        LIST_INSERT_TAIL(&freePagesList, &pageBlockInfoArr[i]);
    }
}
//===========================
// [2] GET BLOCK SIZE:
//===========================
__inline__ uint32 get_block_size(void *va)
{
    //TODO: [PROJECT'25.GM#1] DYNAMIC ALLOCATOR - #2 get_block_size
    //Your code is here
    int virtADD=(int)va;
    for (int ind=0;ind<=DYN_ALLOC_MAX_SIZE / PAGE_SIZE;ind++)
    {
        if (virtADD>=to_page_va(&pageBlockInfoArr[ind])&&virtADD<to_page_va(&pageBlockInfoArr[ind+1]))
        {
            return(pageBlockInfoArr[ind].block_size);
        }
        else{continue;}
    }
    //Comment the following line
    panic("get_block_size() Not implemented yet");
}

//===========================
// 3) ALLOCATE BLOCK:
//===========================
void *alloc_block(uint32 size)
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		assert(size <= DYN_ALLOC_MAX_BLOCK_SIZE);
	}
	//==================================================================================
	//==================================================================================
	//TODO: [PROJECT'25.GM#1] DYNAMIC ALLOCATOR - #3 alloc_block
	//Your code is here
	if(size==0)
		return NULL;

	uint32 N_size=nearest_power_2(size);
	//case 1
		 struct BlockElement* block = NULL;


		    if (!LIST_EMPTY(&freeBlockLists[N_size]))
		    {

		        block = LIST_FIRST(&freeBlockLists[N_size]);

		        LIST_REMOVE(freeBlockLists[N_size],block);

		        struct PageInfoElement* page = block->prev_next_info;
		        page->num_of_free_blocks--;

		        return (void*)block;
		    }

 // case2

		     if(!LIST_EMPTY(&freePagesList))
		    {
		         struct PageInfoElement* newPage = LIST_FIRST(&freePagesList);
		    	LIST_REMOVE(&freePagesList, newPage);
		    	void* newpage_va =to_page_va(newPage);

		        uint32 num_blocks = PAGE_SIZE / size;
		        for (uint32 i = 0; i < num_blocks; i++)
		        	{
		        			struct BlockElement* newBlock = (struct BlockElement*)((uint8*)newpage_va + i * N_size);
		        			LIST_INSERT_HEAD(&freeBlockLists[N_size], newBlock,prev_next_info);
		        	}

		        newPage->block_size=N_size;
		        newPage->num_of_free_blocks=num_blocks;

				block = LIST_FIRST(&freeBlockLists[N_size]);
				LIST_REMOVE(&freeBlockLists[N_size], block,prev_next_info);
				newPage->num_of_free_blocks--;

		        return (void*)block;

		    }

 // case 3

		 	uint32 nextSize=N_size *2 ;
		 	while(nextSize <= DYN_ALLOC_MAX_BLOCK_SIZE)
		 	{
		 		if (!LIST_EMPTY(&freeBlockLists[nextSize]))

		 		   block = LIST_FIRST(&freeBlockLists[nextSize]);

		 				        LIST_REMOVE(freeBlockLists[nextSize],block);

		 				        struct PageInfoElement* page = block->prev_next_info;
		 				        page->num_of_free_blocks--;

		 				        return (void*)block;
		 	}

//case 4
		 	panic("alloc_block(): No free blocks or pages available!");
		 		return NULL;

	//Comment the following line
	panic("alloc_block() Not implemented yet");

	//TODO: [PROJECT'25.BONUS#1] DYNAMIC ALLOCATOR - block if no free block
}
//===========================
// [4] FREE BLOCK:
//===========================
void free_block(void *va)
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		assert((uint32)va >= dynAllocStart && (uint32)va < dynAllocEnd);
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'25.GM#1] DYNAMIC ALLOCATOR - #4 free_block
	//Your code is here
	if(va==NULL)
		return;
	uint32 blockVA= (uint32) va;
	uint32 page_va = blockVA - PGOFF(blockVA);

	uint32 page_index = (page_va - USER_HEAP_START) / PAGE_SIZE;
	struct PageInfoElement *page= &pageBlockInfoArr[page_index];

    if (page->block_size == 0)
        return;

    uint32 blockSize =page->block_size;

    struct BlockElement *block = (struct BlockElement *)va;

    LIST_INSERT_HEAD(&freeBlockLists[blockSize], block, prev_next_info);

    page->num_of_free_blocks++;

    uint32 total_blocks = PAGE_SIZE / blockSize;

    if (page->num_of_free_blocks == total_blocks)
    {

        LIST_INIT(&freeBlockLists[blockSize]);


        return_page((void*)page_va);


        page->block_size = 0;
        page->num_of_free_blocks = 0;

        LIST_INSERT_HEAD(&freePagesList, page, prev_next_info);
    }

	//Comment the following line
	//panic("free_block() Not implemented yet");
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//===========================
// [1] REALLOCATE BLOCK:
//===========================
void *realloc_block(void* va, uint32 new_size)
{
	//TODO: [PROJECT'25.BONUS#2] KERNEL REALLOC - realloc_block
	//Your code is here
	//Comment the following line
	panic("realloc_block() Not implemented yet");
}
