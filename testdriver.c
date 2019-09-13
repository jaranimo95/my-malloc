//Christian Jarani
//CS 449, Professor Misurda
//Custom Malloc & Free
//3/20/16
#include <stdio.h>
#include "mymalloc.h"

int main(){
	int count;
	for(count = 0; count < 20; count++){
		void *free = my_nextfit_malloc(count);
		my_free(free);
	}
}