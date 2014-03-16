#ifndef _LINER_SEQ_H
#define _LINER_SEQ_H

#include "predef.h"

#define LIST_INIT_SIZE 100
#define LIST_INCREMENT 10

typedef int (*CMP)(const void *pa, const void *pb);

typedef struct _SqList{
	void *elem;
	unsigned typesize;
	unsigned length;
	unsigned listsize;
} SqList;

//initialize liner
Status init_linerseq(SqList *sq, unsigned typesize);

//destory liner
void destory_linerseq(SqList *sq);

//insert at tail
Status insert_linerseq(SqList *sq, void *pe);

//insert element to linerSeq
//index starts from 1
Status insert_linerseq_index(SqList *sq, int index, void *pe);

//print all elements
void print_linerseq(SqList *sq, void visit(void *elem));

#endif
