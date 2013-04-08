#include "OS.h"
#include <string.h>

typedef struct _OS_block {
	size_t size;
	struct _OS_block *next;
	unsigned char *buffer;
} _OS_block;

static unsigned char _OS_Heap[_OS_HEAP_SIZE]; // ideally equal to max_threads * max_stack_size * sizeof(long) + max_threads * sizeof(_OS_block)
static _OS_block *freeList = (_OS_block*)_OS_Heap;
static OS_SemaphoreType allocSem, freeSem;

void OS_MemoryInit(void)
{
	_OS_block *b;
	// initialize semaphores
	OS_InitSemaphore(&allocSem, 1);
	OS_InitSemaphore(&freeSem, 1);
	// create free space manager
	freeList->size = 0;
	freeList->next = (_OS_block*)((char*)freeList + sizeof(_OS_block));
	freeList->buffer = NULL;
	// make first allocatable block
	b = freeList->next;
	b->size = _OS_HEAP_SIZE - 2*sizeof(_OS_block);
	b->next = NULL;
	b->buffer = ((unsigned char*)b) + sizeof(_OS_block);
	// create periodic free space merger thread
	OS_Add_Periodic_Thread(&OS_mergeFreeList, 100, 7);
}

// implementation of void* calloc(size_t size);
void* OS_alloc(size_t size)
{
	_OS_block *b, *n;
	void* buff;
	OS_bWait(&allocSem);
	
	size = (((size >> 2) + 1) << 2);
	// search for block of correct size
	for(b = freeList; b != NULL && b->next != NULL; b = b->next)
		if(size < b->next->size + sizeof(_OS_block))
			break;
	// no free space found
	if(b == NULL || b->next == NULL)
	{
		OS_bSignal(&allocSem);
		return NULL;
	}
	// allocate b->next to be returned
	buff = b->next->buffer;
	b->next->size = size;
	n = (_OS_block*)(b->next->buffer + size);
	n->size = b->next->size - size - sizeof(_OS_block);
	n->next = b->next->next;
	n->buffer = ((unsigned char*)n) + sizeof(_OS_block);
	b->next = n;
	OS_bSignal(&allocSem);
	return buff;
}

void OS_free(void* ptr)
{
	_OS_block* b, * temp;
	unsigned char* p = ptr;
	OS_bWait(&freeSem);
	
	// get meta info based on ptr
	b = (_OS_block*)(p - sizeof(_OS_block));
	if(b->buffer != ptr)
	{
		OS_bSignal(&freeSem);
		return; // error
	}
	// erase data
	memset(b->buffer, 0xCC, b->size);
	// add to correct place in freeList
	for(temp = b; temp->next != NULL; temp = temp->next)
	{
		// insert b before temp's next if b's buffer pointer is less
		if(temp->next->buffer > b->buffer)
		{
			b->next = temp->next;
			temp->next = b;
		}
	}
	if(temp->next == NULL)
		temp->next = b;
	
	OS_bSignal(&freeSem);
}

void OS_mergeFreeList(void)
{
	_OS_block* b;
	for(b = freeList->next; b != NULL && b->next != NULL; b = b->next)
	{
		if(b->buffer + b->size == (unsigned char*)b->next)
		{
			b->size += b->next->size;
			b->next = b->next->next;
		}
	}
}
