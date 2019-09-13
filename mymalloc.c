//Christian Jarani
//CS 449, Professor Misurda
//Custom Malloc & Free
//3/20/16
#include <stdio.h>

//NOTE: Will sometimes get stuck in an infinite loop when run with Misurda's driver program, but only on occasion. 
//      It doesn't happen every time.
//ALSO: Cannot set tail to sbrk(size+sizeof(Node)+sizeof(Node)) in first malloc condition.
//		Every other sbrk call works except for that one and I don't know why

typedef struct Node
{	
		struct Node *prev;	   		   //Address to previous node
		struct Node *next;    		  //Address to next node
		int size;		     		 //Size of allocation
		int allocated;				//Allocation status: 0 = unallocated, 1 = allocated
} Node;	  						   //Initial structure declaration

static struct Node *head;				//Represents bottom of the heap
static struct Node *tail;   		   //Represents top of the heap
static struct Node *last_searched;	  //Represents last node searched by next-fit algorithm
static int list_size = 0;

void *my_nextfit_malloc(int size)
{
	if(list_size == 0)										  	    	  //Initial Case: no previous allocations
	{	
		head = (Node*)sbrk(0);        					     	  		//Set head of list to current address of brk
		tail = (Node*)sbrk((int*)size+sizeof(Node)+sizeof(Node));      //Set brk to end of allocated space (size + space for head node + space for tail node)
		if (tail == (Node*)-1)						  		    	  //Print error message if unsuccessful. Must cast -1 as void pointer to be compared to void pointer request
		{
			return NULL;
		}
		*head = (Node){tail,tail,size,0};	  	//Initialize head node data 
		*tail = (Node){head,head,0,0};		   //Initialize tail node data
		last_searched = head;		 		  //Initialize last_searched to head node for next allocation request
		list_size = 1;						 //Increment list size by two (head node & tail node)
		
		return (void*)tail->prev+sizeof(Node);
	}
	else  //Begin Next-Fit Algorithm: searches pre-existing blocks, starting from the last block searched, for space to fit allocation request
	{
		if(list_size == 1){
			tail = (Node*)sbrk(size+sizeof(Node)+sizeof(Node));
			head->next = tail;
			head->prev = tail;
		}

		struct Node *current = last_searched;	  //Set initial position to last node checked by next-fit
		struct Node *previous;					 //Previous position checked
		int nodes_searched;						//Number of allocation nodes read by next-fit

	    for(nodes_searched = 0; nodes_searched < list_size; nodes_searched++)
	    {	
	    	previous = current;
	    	current = current->next;
	    	if(!current->allocated && current->size >= size)	     //If block is unallocated and can handle requested size, end search and allocate space
	    	{	
	    		last_searched = current;						   //Update last block searched
	    		last_searched->allocated = 1;					  //Set block as allocated
	    		return (void*)last_searched+sizeof(Node);	     //Return pointer to beginning of unallocated space
	    	}
	    } //End next-fit
	}
    //If no existing unallocated space is found, request more
    void *request = (void*)sbrk(size+sizeof(Node));		    //Set brk to end of allocated space (size + space for tail node)
    if (request == (void*)-1)						  	   //Print error message if unsuccessful. Must cast -1 as void pointer to be compared to void pointer request
	{
		return NULL;
	}

	Node *temp = tail;			 	   //Create temp node pointer to hold old tail's address
    tail->next = (Node*)request;	  //Set tail->next to new brk address
    tail->allocated = 1;			 //Mark newly-allocated space as allocated
    tail = (Node*) request;			//Move tail to new brk address
    tail->prev = temp;		       //Set new tail's prev field to old tail's address
    tail->next = head;		      //Circularly-link list by setting tail's next field to head
    head->prev = tail;			 //Complete circular-link by setting head's prev field to tail
    list_size++;				//Increment list size

    return (void*)tail->prev+sizeof(Node);
}

void my_free(void *ptr)
{
	if(list_size == 0){		//If no memory has been allocated yet
		return;
	}
	//Begin freeing of space
	struct Node *current = head;	  		  //Set initial position to head
	struct Node *previous;					 //Previous position checked
	int nodes_searched;						//Number of allocation nodes read by next-fit

	//Iterate through each node in the list and compare to requested block to free
	for(nodes_searched = 0; nodes_searched < list_size; nodes_searched++)
	{	
	   	previous = current;
	   	current = current->next;
	   	if(current == ptr)				 	    //If block in list equals block requested to be freed, free the block
	   	{	
	   		struct Node *to_free = current;   //Set as block to free
	   		to_free->allocated = 0;  		 //Mark block to free as unallocated

	   		//If previous block is unallocated, coallesce the blocks
	   		if((to_free->prev)->allocated == 0 && to_free != head)
	   		{
		   		(to_free->prev)->size += to_free->size + sizeof(Node);	  //Add size of current block to free + size of representative node to previous node's size
		   		(to_free->prev)->next = to_free->next; 				     //Have previous block point to block after the one requested to be freed
		   		to_free = (to_free)->prev;							    //Set block to free to beginning of newly-coallesced block (garbage collection)
		   		list_size--;										   //Decrement size of list due to coallesce

		   		if(to_free->next == head)	 							   //If node pointed to by block to free is head of list (bottom of heap)
		   		{														    
		   			tail = (Node*)sbrk(-(to_free->size+sizeof(Node)));	 //Block now defined as tail of list (top of heap), so set brk back by its size to free it
		   			list_size--;										//Decrement size of list due to changing brk
		   			return;
		   		}
		   		else return;
	   		}
	   		//Else if next block is unallocated, coallesce the blocks
	   		else if((to_free->next)->allocated == 0)
	   		{
	   			if(to_free->next == head)	 //If node pointed to by block to free is head of list (bottom of heap)
	   			{
	   				tail = (Node*)sbrk(-(to_free->size+sizeof(Node)));	//Block to free now defined as tail of list (top of heap), so set brk back by its size to free it
	   				list_size--;									   //Decrement size of list due to changing brk
	   				return;
	   			} 
	   			else 						 //Else, continue with coallesce
	   			{
	   				to_free->size += (to_free->next)->size + sizeof(Node);	  //Add size of next block + size of representative node to size of block to free
	   				to_free->next = (to_free->next)->next;					 //Have block to free point to the block after the area just coallesced (garbage collection)
	   				list_size--;										    //Decrement size of list due to coallesce

					if(to_free->next == head)	 							   //If node pointed to by block to free is head of list (bottom of heap)
		   			{											    
		   				tail = (Node*)sbrk(-(to_free->size+sizeof(Node)));	 //Block now defined as tail of list (top of heap), so set brk back by its size to free it
		   				list_size--;										    //Decrement size of list due to coallesce
		   				return;
		   			}
		   			else return;
	   			}
	   		}
	   		//No free blocks adjacent to block to free, so don't coallesce
	   		else return;
	   	}
	} //End search
	if(nodes_searched == list_size)		//If block requested can't be found, return current brk as free space
	{
		return;
	}
}