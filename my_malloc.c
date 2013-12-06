#include "my_malloc.h"
#include <math.h>

/* 
	Julia Ting
	CS 2110
	Final Malloc Homework
	12/5/2013
*/

/* You *MUST* use this macro when calling my_sbrk to allocate the 
 * appropriate size. Failure to do so may result in an incorrect
 * grading!
 */
#define SBRK_SIZE 2048

/* If you want to use debugging printouts, it is HIGHLY recommended
 * to use this macro or something similar. If you produce output from
 * your code then you will receive a 20 point deduction. You have been
 * warned.
 */
 
#ifdef DEBUG
#define DEBUG_PRINT(x) printf(x)
#else
#define DEBUG_PRINT(x)
#endif


/* make sure this always points to the beginning of your current
 * heap space! if it does not, then the grader will not be able
 * to run correctly and you will receive 0 credit. remember that
 * only the _first_ call to my_malloc() returns the beginning of
 * the heap. sequential calls will return a pointer to the newly
 * added space!
 
 Note to self: "beginning of heap" includes meta data
 
 * Technically this should be declared static because we do not
 * want any program outside of this file to be able to access it
 * however, DO NOT CHANGE the way this variable is declared or
 * it will break the autograder.
 */
void* heap;

/* our freelist structure - this is where the current freelist of
 * blocks will be maintained. failure to maintain the list inside
 * of this structure will result in no credit, as the grader will
 * expect it to be maintained here.
 * Technically this should be declared static for the same reasons
 * as above, but DO NOT CHANGE the way this structure is declared
 * or it will break the autograder.
 */
metadata_t* freelist[8];
/**** SIZES FOR THE FREE LIST ****
 * freelist[0] -> 16
 * freelist[1] -> 32
 * freelist[2] -> 64
 * freelist[3] -> 128
 * freelist[4] -> 256
 * freelist[5] -> 512
 * freelist[6] -> 1024
 * freelist[7] -> 2048
 */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~MY_MALLOC~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void* my_malloc(size_t size)
{
	// finds the necessary size of the block and returns corresponding
	// index into freelist
	int listIndex = findSize(size);
	
	/* First malloc call because heap is not set at all */
	if (heap == NULL) {
		// This is causing a segfault and I don't know what about it
		// in my tests but only after I split manually. And
		// in my_sbrk. HNNNNNNNNNNNNNNGGGGGGGGGGGG
		metadata_t *newSpace = (metadata_t*)expandHeap;
		/* Check to see if expandHeap worked or not */
		if (newSpace == NULL) {
			ERRNO = OUT_OF_MEMORY;
			return NULL;
		}
		heap = newSpace;
		newSpace->size = 2048;
		newSpace->in_use = 0;
		addToFreelist(7, newSpace);
	}
	/* If list index is -1, it means the size does not fit into any
	 of the blocks and is > 2048. */
	if (listIndex == -1) {
		ERRNO = SINGLE_REQUEST_TOO_LARGE;
		return NULL;
	}
	
	/* If freelist at this index is not null, there is 
	 a block of that size that is available, so we check
	 if it is null, and then split it if so. */
	if (freelist[listIndex] == NULL) {
		find_block(listIndex);
		/* Follows up on the error code set if the heap expansion
		 doesn't work in find_block */
		if (ERRNO == OUT_OF_MEMORY) {
			return NULL;
		}
	}
	ERRNO = NO_ERROR;
	return assign_block(listIndex);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~MY_MALLOC HELPER FUNCTIONS~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* 
 Recursively finds & splits blocks until it gets the appropriate
 size. If the index is greater than 7, assumes it was because
 there were no blocks and expands the heap size.
 
 currIndex: index we are currently at while iterating recursively
*/
void find_block(int currIndex) {
	
	/* if currIndex > 7 means all slots were null.
	 This means that there is no more/not enough mem, expandHeap */
	if (currIndex > 7) {
		metadata_t *newSpace = expandHeap;
		if (newSpace == NULL) {
			ERRNO = OUT_OF_MEMORY;
			return;
		}
		/* Check if heap expansion is greater than 8KB of space */
		char *math = (char *)newSpace;
		math = math - (long)heap;
		if ((long)math > 8192) {
			ERRNO = OUT_OF_MEMORY;
			return;
		}
		newSpace->size = 2048;
		newSpace->in_use = 0;
		newSpace->next = NULL;
		newSpace->prev = NULL;
		if (freelist[7] == NULL) {
			freelist[7] = newSpace;
		/* Shouldn't need this but did it anyway cause im dumblolz */
		} else {
			metadata_t *curr = freelist[7];
			while (curr->next != NULL) {
				curr = curr->next;
			}
			curr->next = newSpace;
			newSpace->prev = curr;
		}
	}
	/* If this list has no available blocks, go to next biggest
	 slot of blocks. */
	if (freelist[currIndex] == NULL) {
		find_block(currIndex+1);
	/* If it DOES, then our splitting work is done and can return */
	} else {
		return;
	}
	split_block(currIndex+1);
	/* Something in split_block fcked up so need to debug
	if (success == 0) {
		//printf("SPLIT HAS FAILED AT %d\n", currIndex+1);
	}
	*/
}

/* 
 Assumes listIndex slot has blocks. Takes blocks and returns
 ptr to spot right after metadata (free space usable by the caller
 of malloc). 
*/
void* assign_block(int listIndex) {
	metadata_t *block = freelist[listIndex];
	/* This shouldn't happen, but check anyways. This means we 
	 have an incorrect listIndex for assign block, check my_malloc */
	if (block == NULL) {
		return NULL;
	}
	/* If this was the last block in the list, set the slot
	 to null, if not, set slot to next block & the prev to NULL */
	if (block->next == NULL) {
		freelist[listIndex] = NULL;
	} else {
		freelist[listIndex] = block->next;
		metadata_t *newHead = block->next;
		newHead->prev = NULL;
		block->next = NULL;
	}
	/* Found the block size we are looking for, so return pointer
	 to said block after setting correct details */
	block->in_use = 1;
	char *mathPtr = (char *)block; // cast to ensure accurate
	mathPtr = mathPtr + (short)metaSize;  // ptr arithmatic
	metadata_t *result = (metadata_t *)mathPtr;
	return result; // Result is ptr right after metadata
	
}

/* 
 Returns success code on whether or not it splits blocks.

 1 = successful split
 0 = unsuccessful split
*/
int split_block(int listIndex) {
	if (listIndex < 0 || listIndex > 7) {
		return 0;
	}
	if (freelist[listIndex] == NULL) {
		return 0;
	}
	metadata_t *big = freelist[listIndex];
	
	if (big->next == NULL) {
		freelist[listIndex] = NULL;
	} else {
		freelist[listIndex] = big->next;
		metadata_t *newHead = big->next;
		newHead->prev = NULL;
		big->next = NULL;
	}
	/* Doing the actual splitting */
	metadata_t *one = big;
	short halfsize = one->size/2;
	char *math = (char *)one;
	math = math + halfsize;
	metadata_t *two = (metadata_t *)math;
	/* Set the size to be half */
	one->size = one->size/2;
	two->size = one->size;
	one->next = two;
	one->prev = NULL;
	two->prev = one;
	two->next = NULL;
	/* The only time we ever split will be because we want a smaller
	 chunk but could not find any smaller blocks, so the listIndex-1
	 slot will be null. We don't need to check for other blocks */
	freelist[listIndex-1] = one;
	return 1;
}

/*
 Helper function that takes in the user size, adds the size
 of the meta data, and returns back the appropriate index
 in the free list according to the size.
*/
int findSize(size_t size) {
	short result = (short)size + (short)metaSize;
	int index;
	if (result <= 16) {
		index = 0;
	} else if (result <= 32)
		index = 1;
	else if (result <= 64)
		index = 2;
	else if (result <= 128)
		index = 3;
	else if (result <= 256)
		index = 4;
	else if (result <= 512)
		index = 5;
	else if (result <= 1024)
		index = 6;
	else if (result <= 2048)
		index = 7;
	else
		/* returns -1 when greater than 2048, 
		 need to set error code  */
		index = -1;
	return index;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~END OF MY_MALLOC HELPER FUNCTIONS~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~MY_CALLOC~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void* my_calloc(size_t num, size_t size)
{
	void *mal = my_malloc(num*size);
	if (mal == NULL) {
  		ERRNO = OUT_OF_MEMORY;
  		return NULL;
  	}
  	char *ptr = (char *)mal;
  	char *end = ptr + (num*size);
  	while (ptr < end) {
  		*ptr = 0;
  		ptr = ptr+1;
  	}
  	ERRNO = NO_ERROR;
  	return mal;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~MY_FREE~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* 
 Frees a given ptr to space and places the memory back in to the 
 freelist.
*/
void my_free(void* ptr)
{
	char *math = (char *)ptr;
	math = math - metaSize;
	metadata_t *metaptr = (metadata_t *)math;
	if (metaptr->in_use == 0) { 
		ERRNO = DOUBLE_FREE_DETECTED;
		return;
	} else {
		metaptr->in_use = 0;
	}
	metadata_t *curr = metaptr;
	while (curr != NULL) {
		curr = checkBuddy(curr);
	}
	ERRNO = NO_ERROR;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* ~~~~~~~~~~~~~MY_FREE HELPER FUNCTIONS FOLLOWING ~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/* checkBuddy takes in a metadata_t pointer and returns an int
 value determining if it's buddy is free or not. If it is free,
 it merges the buddies and places in the correct slot. If not,
 does not merge, just puts in freelist.
 
 NULL = did not merge
 
 otherwise, returns the pointer to the block that was merged
*/
metadata_t * checkBuddy(metadata_t *one) {
	/* Block of size 2048 will not have a buddy */
	if (one->size == 2048) {
		return NULL;
	}
	int bit_value = checkBit(one);
	metadata_t *buddy = findBuddyAddress(bit_value, one);
	
	/* You didn't find the right buddyAddress lolz */
	if (one->size != buddy->size) {
		ERRNO = OUT_OF_MEMORY;
		return NULL;
	}
	int listIndex = findSize((size_t)(one->size)-metaSize);
	return mergeBuddy(bit_value, listIndex, one, buddy);
}

/*
 Merge buddy takes in the bit value, freelist index, and the two
 buddies. It determines which one is the first, extracts both from
 the freelist, merges them, then puts the block back in the freelist.
*/
metadata_t * mergeBuddy(int bit_value, int listIndex, 
	metadata_t *one, metadata_t* buddy) {
	if (buddy->in_use) {
		// Buddy is not free, just add one to freelist
		addToFreelist(listIndex, one);
		return NULL;
	} else {
		// Buddy IS free, merge. Need to know which block is which.
		listIndex++;
		if (bit_value) {
			/* Block "one" is the right block */
			removeFromFreelist(listIndex, buddy);
			removeFromFreelist(listIndex, one);
			buddy->next = NULL;
			buddy->size = one->size * 2;
			/* Adding buddy because buddy is the left block */
			addToFreelist(listIndex, buddy);
			return buddy;
		} else {
			/* Left buddy */
			
			/* Extract from list */
			removeFromFreelist(listIndex-1, one);
			removeFromFreelist(listIndex-1, buddy);
			one->next = NULL;
			one->size = one->size * 2;
			/* Adding one because one is the left block */
			addToFreelist(listIndex, one);
			return one;
		}
	}
}

/*
 checkBit checks the significant bit depending on the size
 of the metadata structure. It determines if the bit is 1 or 0.
 
 1 - metadata_t one is the right buddy
 0 - metadata_t one is the left buddy
*/
int checkBit(metadata_t *one) {
	char *addressOne = (char *)one;
	long displace = (long)heap;
	/* Displace block addresses to calculate which bit to check */
	addressOne = addressOne - displace;
	
	int bitshift = my_log2(one->size);
	/* check bit value at the location to see if it is a left
	 or a right buddy. */
	long address = (long)addressOne;
	int bit_value = (address >> bitshift) & 1;
	return bit_value;
}

/*
 Determines the address of the buddy for the given metadata_t
 structure and returns a pointer to that address.
*/
metadata_t * findBuddyAddress(int bit_value, metadata_t *one) {
	char * buddyAddress = (char *)one;
	if (bit_value) {
		/* Have right block, so for buddy should subtract */
		buddyAddress = buddyAddress - one->size;
	} else {
		/* Have left block, so for buddy should add */
		buddyAddress = buddyAddress + one->size;
	}
	return (metadata_t *)buddyAddress;
}

/*
 Extracts the block from the given slot in the freelist.
 Checks all possible cases.
*/
metadata_t * removeFromFreelist(int listIndex, metadata_t *block) {
	/* Extract from list */
	if (block->next == NULL && block->prev == NULL) {
		freelist[listIndex] = NULL;
	/* next is NULL but prev is not null */
	} else if (block->next == NULL) {
		metadata_t *prev = block->prev;
		prev->next = NULL;
	/* prev is NULL but next is not */
	} else if (block->prev == NULL) {
		freelist[listIndex] = block->next;
	/* they are BOTH not null */
	} else {
		metadata_t *prev = block->prev;
		prev->next = block->next;
	}
	block->next = NULL;
	block->prev = NULL;
	return block;
}

/*
 Adds the given metadata_t block to the specified freelist index.
*/
void addToFreelist(int listIndex, metadata_t *block) {
	block->next = NULL;
	block->prev = NULL;
	if (freelist[listIndex] == NULL) {
		freelist[listIndex] = block;
	} else {
		metadata_t *curr = freelist[listIndex];
		while (curr->next != NULL) {
			curr = curr->next;
		}
		curr->next = block;
		block->prev = curr;
	}
}

/*
 Homemade log base 2 function for the size. Used to know which
 bit to check for the buddy system.
*/
int my_log2(int size) {
	int result = 0;
	while (size > 1) {
		size = size/2;
		result++;
	}
	return result;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~END OF MY_FREE HELPER FUNCTIONS~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~MY_MEMMOVE~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
void* my_memmove(void* dest, const void* src, size_t num_bytes)
{
	/* dest and src are the same, nothing will change. */
	if ((long)dest == (long)src) {
		return dest;
	}
	char * dst = (char *)dest;
	char * source = (char *)src;
	if ((long)dest > (long)src) {
		long i = (long)source;
		i = i + (long)num_bytes-1;
		while ((long)source >= i) {
			*dst = *source;
			source = source -1;
			dst = dst-1;
		}
	} else {
		long j = (long)source;
		j = j + (long)num_bytes;
		while ((long)source < j) {
			*dst = *source;
			source = source+1;
			dst = dst+1;
		}
	}
  return dest;
}
