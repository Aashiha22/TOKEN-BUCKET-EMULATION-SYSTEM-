#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include "cs402.h"

#include "my402list.h"

# define null 0

   
int My402ListLength(My402List* l)
{
	//printf("Prob here15");
	My402ListElem *elem = null;
	elem = (My402ListElem *)malloc(sizeof(My402ListElem));
	int count = 0;
	if(My402ListEmpty(l) == FALSE)
	{
	//	printf("\nList not empty inside length");
		for(elem=My402ListFirst(l);elem!=NULL;elem=My402ListNext(l,elem))
		{
			count = count + 1;
		}
	}
	
	return count;
}
int My402ListEmpty(My402List* l)
{
	//printf("Prob here14");
	if(l->anchor.next == null)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

int My402ListAppend(My402List* l, void* a)
{
	//printf("Prob here13");
//	printf("%p",(struct line_details *)a);
	
	My402ListElem *elem = null;
	elem = (My402ListElem *)malloc(sizeof(My402ListElem));
	//elem->obj = malloc(sizeof(a));
	My402ListElem *lelem = null;
	//lelem = (My402ListElem *)malloc(sizeof(My402ListElem));
	elem->obj = a;
	if(My402ListEmpty(l) ==  TRUE)
	{
		l->anchor.next = elem;
		elem->prev = &l->anchor;
		elem->next = &l->anchor;
		l->anchor.prev = elem;
	//	printf("\nElement inserted while list is null");
	}
	else
	{
		lelem = My402ListLast(l);
		elem->next = &l->anchor;
		elem->prev = lelem;
		lelem->next = elem;
		l->anchor.prev = elem;
	//	printf("\nElement inserted while list is not null");
	}
	return TRUE;	
}
int My402ListPrepend(My402List* l, void* a)
{
	//printf("Prob here12");
	My402ListElem *elem = null;
	elem = (My402ListElem *)malloc(sizeof(My402ListElem));
	My402ListElem *felem = null;
	felem = (My402ListElem *)malloc(sizeof(My402ListElem));
	elem->obj = a;
	if(My402ListEmpty(l) ==  TRUE)
	{
		l->anchor.next = elem;
		elem->prev = &l->anchor;
		elem->next = &l->anchor;
		l->anchor.prev = elem;
	}
	else
	{
		felem = My402ListFirst(l);
		l->anchor.next = elem;
		elem->prev = &l->anchor;
		elem->next = felem;
		felem->prev = elem;
	}
	return TRUE;
}

void My402ListUnlink(My402List*list, My402ListElem*elem)
{
	// printf("eeeeeeeee");

	if(elem==My402ListFirst(list))
	{
		//printf("1&&&");
		if(elem==My402ListLast(list))
		{
		list->anchor.prev=list->anchor.next=NULL;
		list->anchor.obj=NULL;
		list->num_members=0;
		}
		else
		{
			//printf("2&&&");
		My402ListElem* next=elem->next;
		list->anchor.next=next;
		next->prev=&list->anchor;
		list->num_members=list->num_members-1;
		}
	}
	else if(elem==My402ListLast(list))
	{
		//printf("3&&");
		My402ListElem* temp=elem->prev;
		temp->next=&list->anchor;
		list->anchor.prev=temp;
		list->num_members=list->num_members-1;
	}
	else
	{
		//printf("4))))");
		My402ListElem* prev=elem->prev;
		My402ListElem* next=elem->next;
		prev->next=next;
		next->prev=prev;
		list->num_members=list->num_members-1;
	}
	free(elem);
}	


void My402ListUnlinkAll(My402List* l)
{
	int a = My402ListLength(l);
	//printf("List length : %d",a);
	int i;
	for(i=0;i<a-1;i++)
	{
		My402ListElem *elem = null;
		elem = (My402ListElem *)malloc(sizeof(My402ListElem));
		if(My402ListEmpty(l) != TRUE)
		{
			elem=My402ListFirst(l);
			if(elem->next != null)
			{
				elem->prev->next = elem->next;
				elem->next->prev = elem->prev;
				free(elem);
			}
		}
		
	}
	My402ListElem *elem1 = null;
	elem1 = (My402ListElem *)malloc(sizeof(My402ListElem));
	elem1=My402ListFirst(l);
	free(elem1);
	l->anchor.prev = null;
	l->anchor.next = null;
	
}
int  My402ListInsertAfter(My402List* l, void* a, My402ListElem* e)
{
	//printf("Prob here2");
	My402ListElem *elem = null;
	elem = (My402ListElem *)malloc(sizeof(My402ListElem));
	elem->obj = a;
	if(e == null)
	{
		My402ListAppend(l, a);	
	}
	else
	{
		elem->next = e->next;
		e->next->prev = elem;
		e->next = elem;
		elem->prev = e;
	}
	return TRUE;
}
int  My402ListInsertBefore(My402List* l, void* a, My402ListElem* e)
{
	//printf("Prob here3");
	My402ListElem *elem = null;
	elem = (My402ListElem *)malloc(sizeof(My402ListElem));
	elem->obj = a;
	if(e == null)
	{
		My402ListPrepend(l, a);	
	}
	else
	{
		elem->next = e;
		e->prev->next = elem;
		elem->prev = e->prev;
		e->prev = elem;
	}
	return TRUE;
}

My402ListElem *My402ListFirst(My402List* l)
{
	//printf("Prob here4");
	My402ListElem *elem = null;
	elem = (My402ListElem *)malloc(sizeof(My402ListElem));
	if(l->anchor.next == null)
	{
		return null;
	}
	else
	{
		elem = l->anchor.next;
	}
	return elem;
}
My402ListElem *My402ListLast(My402List* l)
{
	//printf("Prob here5");
	My402ListElem *elem = null;
	elem = (My402ListElem *)malloc(sizeof(My402ListElem));
	if(l->anchor.prev == null)
	{
		return null;
	}
	else
	{
		elem = l->anchor.prev;
	}
	return elem;
}
My402ListElem *My402ListNext(My402List* l, My402ListElem* e)
{
	//printf("Prob here6");
	My402ListElem *elem = null;
	elem = (My402ListElem *)malloc(sizeof(My402ListElem));
	if(e == My402ListLast(l))
	{
		elem = null;
	}
	else
	{
		elem = e->next;
	}
	return elem;
}
My402ListElem *My402ListPrev(My402List* l, My402ListElem* e)
{
	//printf("Prob here7");
	My402ListElem *elem = null;
	elem = (My402ListElem *)malloc(sizeof(My402ListElem));
	if(e == My402ListFirst(l))
	{
		elem = null;
	}
	else
	{
		elem = e->prev;
	}
	return elem;
}

My402ListElem *My402ListFind(My402List* l, void* a)
{
	//printf("Prob here8");
	My402ListElem *elem = null;
	elem = (My402ListElem *)malloc(sizeof(My402ListElem));
	My402ListElem *elem1 = null;
	elem1 = (My402ListElem *)malloc(sizeof(My402ListElem));
	int flag = 0;
	if(My402ListEmpty(l) != TRUE)
	{
		for(elem=My402ListFirst(l);elem!=NULL;elem=My402ListNext(l,elem))
		{
			if(elem->obj == a)
			{
				flag = 1;
				elem1 = elem;
			}
		}
		if(flag == 0)
		{
			elem1 = null;
			//printf("\nElement not found");
		}
		
	}
	return elem1;
}



int My402ListInit(My402List* l)
{
	//printf("Prob here9*");
	//l->anchor = (My402ListElem *)malloc(sizeof(My402ListElem));
	l->num_members = 4;
	l->anchor.obj = null;
	l->anchor.prev = null;
	l->anchor.next = null;
	return TRUE;
}

int printlist(My402List* l)
{
	My402ListElem *elem = null;
	elem = (My402ListElem *)malloc(sizeof(My402ListElem));
	printf("\tPrint list\n");
	int c =0;
	if(My402ListEmpty(l) != TRUE)
	{
		for(elem=My402ListFirst(l);elem!=NULL;elem=My402ListNext(l,elem))
		{
			//printf(" Element : %d\n",(int)elem->obj);
			c = c+1;
		}
	}
	return c;
}


