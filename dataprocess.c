#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include "linerseq.h"

typedef enum{
	SplitTypeA = 0,
	SplitTypeB
}SplitType;

//csv A文件的数据结构
//将所有列数据载入内存

//csv的列数硬编码，减少动态内存分配
typedef struct{
	char *strarr[4];
	//B表的重复记录
	bool is_dup;
}CSV_ROW_A;

//csv B文件
//求第i列的差集，csv b文件只需要加载一列的数据
typedef struct{
	//key str
	char *key_str;
	//convert to unsigned int
	//accelerate sorting
	unsigned int key_int;
}CSV_ROW_B;

void load_csv_file(const char *path, SqList *L, SplitType spty){
	FILE *fp = fopen(path, "r");
	//max line size
	char buf[512];

	while(fgets(buf, sizeof(buf), fp) != NULL){
		//remove newline character
	
		//按照struct CSV_ROW_A 的格式分解
		if(spty == SplitTypeA){
			
			//处理windows下换行符
			//unix 换行符 \n 
			//windows 换行符 \r\n
			
			CSV_ROW_A row_a;
			row_a.is_dup = FALSE;

			unsigned int str_arr_len = ARRAY_LEN(row_a.strarr);

			char **pstr = row_a.strarr;
			char *pb = strtok(buf, ";");
			while(pb != NULL){
				assert(pstr < row_a.strarr+str_arr_len);
				
				size_t pb_len = strlen(pb);
				char *p = (char *)malloc(sizeof(char)*(pb_len+1));
				assert(p != NULL);
				strcpy(p, pb);
				*pstr++ = p;

				pb = strtok(NULL, ";");
			}

			insert_linerseq(L, &row_a);

		}else if(spty == SplitTypeB){
			
			CSV_ROW_B row_b;
			
			char *pb = strtok(buf, ";");
			size_t pb_len = strlen(pb);
			char *p = (char *)malloc(sizeof(char) * (pb_len+1));
			assert(p!=NULL);
			strcpy(p, pb);
			
			row_b.key_str = p;
			insert_linerseq(L, &row_b);
		}
	}
}

void visit_row_a(void *pa){
	CSV_ROW_A *prow = (CSV_ROW_A *) pa;
	char **p = prow->strarr;
	char **q = prow->strarr + ARRAY_LEN(prow->strarr);
	for(; p<q; p++){
		printf("%s;", *p);
	}
}

void visit_row_b(void *pb){
	CSV_ROW_B *prow = (CSV_ROW_B *) pb;
	printf("%s\n", prow->key_str);
}

void difference(SqList *La, SqList *Lb){
	unsigned step_a = La->typesize;
	unsigned step_b = Lb->typesize;
	
	int count = 0;
	for(int i=0; i<La->length; i++){
		CSV_ROW_A *pra = (CSV_ROW_A *)((char *)La->elem + i*step_a);	
		
		for(int j=0; j<Lb->length; j++){
			CSV_ROW_B *prb = (CSV_ROW_B *)((char *)Lb->elem + j*step_b);
			if(strcmp(pra->strarr[0], prb->key_str) == 0){
				count++;
				pra->is_dup = TRUE;
			}
		}
	}
	printf("dup count:%d\n", count);
}

void write_csv_file(const char *path, SqList *list){
	
}

int main(void){
	const char *tableA = "./testA.csv";
	const char *tableB = "./testB.csv";

	SqList csv_list_a;
	SqList csv_list_b;

	init_linerseq(&csv_list_a, sizeof(CSV_ROW_A));
	init_linerseq(&csv_list_b, sizeof(CSV_ROW_B));

	load_csv_file(tableA, &csv_list_a, SplitTypeA);
	load_csv_file(tableB, &csv_list_b, SplitTypeB);
	
	clock_t diff_start = clock();

	//print_linerseq(&csv_list_a, visit_row_a);
	//print_linerseq(&csv_list_b, visit_row_b);

	//parse_csv_b

	//sort_csv_b

	//difference A-B
	difference(&csv_list_a, &csv_list_b);

	//write A-B.csv to disk
	clock_t diff_end = clock();

	printf("diff compute time: %f seconds\n", (double)(diff_end-diff_start)/CLOCKS_PER_SEC); 
	
	return 0;
}

