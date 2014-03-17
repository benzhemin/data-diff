#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include "linerseq.h"

typedef enum{
	CSVTypeA = 0,
	CSVTypeB
}CSVType;

//每行数据保存到buf_str中
typedef struct {
	char *raw_str;
}RawLine;

//csv A文件的数据结构
//将所有列数据载入内存

//csv的列数硬编码
typedef struct{
	char *strarr[4];
	//该行的key是否在B表出现
	bool is_dup;
}CSV_ROW_A;

//csv B文件
//求第i列的差集，csv b文件只需要加载一列的数据
typedef struct{
	//key str
	char *key_str;
}CSV_ROW_B;

void load_csv_file(const char *path, SqList *L){
	FILE *fp = fopen(path, "r");

	char buf[512];
	while(fgets(buf, sizeof(buf), fp) != NULL){
		size_t len = strlen(buf);
		char *p = (char *)malloc(sizeof(char) * (len+1));
		assert(p != NULL);
		strcpy(p, buf);

		RawLine rawline;
		rawline.raw_str = p;

		insert_linerseq(L, &rawline);
	}

	fclose(fp);
}

void parse_csv_list(SqList *rawL, SqList *L, CSVType spty){
	
	for(int i=0; i<rawL->length; i++){
		RawLine rawline = *((RawLine *)rawL->elem + i);

		//按照struct CSV_ROW_A 的格式分解
		if(spty == CSVTypeA){
			
			//处理windows下换行符
			//unix 换行符 \n 
			//windows 换行符 \r\n
			
			CSV_ROW_A row_a;
			row_a.is_dup = FALSE;

			unsigned int strarr_len = ARRAY_LEN(row_a.strarr);

			char **pstr = row_a.strarr;
			char *pb = strtok(rawline.raw_str, ";");
			while(pb != NULL){
				assert(pstr < row_a.strarr+strarr_len);
				
				size_t pb_len = strlen(pb);
				char *p = (char *)malloc(sizeof(char)*(pb_len+1));
				assert(p != NULL);
				strcpy(p, pb);
				*pstr++ = p;

				pb = strtok(NULL, ";");
			}

			insert_linerseq(L, &row_a);

		}else if(spty == CSVTypeB){
			
			CSV_ROW_B row_b;
			
			char *pb = strtok(rawline.raw_str, ";");
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

unsigned write_csv_file(const char *path, SqList *L){
	FILE *fp = fopen(path, "w+");

	const char *sep_str = ";";
	unsigned step = L->typesize;
	char *pe = (char *)L->elem;

	unsigned count = 0;
	for(int i=0; i<L->length; i++){
		CSV_ROW_A *prow = (CSV_ROW_A *) (pe + step*i);

		if (prow->is_dup == FALSE){
			count++;
			unsigned pstrlen = ARRAY_LEN(prow->strarr);
			for(int j=0; j<pstrlen; j++){
				fputs(prow->strarr[j], fp);

				//写入分隔符
				if (j != pstrlen-1){
					fputs(sep_str, fp);
				}else{
					//如果需要写入换行符
				}
			}	
		}
	}
	fclose(fp);

	return count;
}


//A-B 方法一
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
				break;
			}
		}
	}
	//printf("dup count:%d\n", count);
}

void strategy_one(SqList *La, SqList *Lb){
	difference(La, Lb);
}


//A-B 方法二
void remove_dup(SqList *Lb){
	unsigned step = Lb->typesize;
	int dup_count = 0;

	for(int i=0; i<Lb->length; i++){
		char *pelem = (char *)Lb->elem + step * i;
		const char *key_cur = ((CSV_ROW_B *)pelem)->key_str;
		
		for(int j=i+1; j<Lb->length; j++){
			char *pe = (char *)Lb->elem + step * j;
			const char *key_tp = ((CSV_ROW_B *)pe)->key_str;

			if(strcmp(key_cur, key_tp) == 0){
				dup_count++;
				continue;
			}
			break;
		}

		if(dup_count != 0){
			char *pdst = (char *)Lb->elem + step * (i+1);
			char *psrc = (char *)Lb->elem + step * (i+dup_count+1);
			char *pend = (char *)Lb->elem + step * Lb->length;

			memmove(pdst, psrc, pend-psrc);
			Lb->length -= dup_count;
			dup_count = 0;
		}
	}

}

bool binary_search(SqList *Lb, const char *key){
	assert(Lb->length > 0);

	char *pe = (char *)Lb->elem;
	unsigned step = Lb->typesize;

	unsigned low = 0;
	unsigned high = Lb->length - 1;
	unsigned mid;

	while(low <= high){
		mid = low + ((high-low)/2);

		CSV_ROW_B *pr = (CSV_ROW_B *) (pe + step*mid);
		
		int cmp_res = strcmp(pr->key_str, key);

		if(cmp_res == 0){
			return TRUE;
		}else if(cmp_res > 0){
			high = mid - 1;
		}else{
			low = mid + 1;
		}
	}
	return FALSE;
}

void difference_bi(SqList *La, SqList *Lb){
	unsigned step_a = La->typesize;
	unsigned step_b = Lb->typesize;
	
	int count = 0;
	for(int i=0; i<La->length; i++){
		CSV_ROW_A *pra = (CSV_ROW_A *)((char *)La->elem + i*step_a);	
		
		if(binary_search(Lb, pra->strarr[0])){
			count++;
			pra->is_dup = TRUE;
		}
	}
	//printf("dup count:%d\n", count);
}

int key_cmp(const void *pa, const void *pb){
	CSV_ROW_B *pra = (CSV_ROW_B *)pa;
	CSV_ROW_B *prb = (CSV_ROW_B *)pb;

	return strcmp(pra->key_str, prb->key_str);
}

void strategy_two(SqList *La, SqList *Lb){
	qsort(Lb->elem, Lb->length, Lb->typesize, key_cmp);	
	remove_dup(Lb);
	difference_bi(La, Lb);
}

int main(void){
	clock_t start = clock();

	const char *csv_file_A = "./testA.csv";
	const char *csv_file_B = "./testB.csv";
	const char *csv_file_output = "./diffA-B.csv";

	SqList csv_raw_list_a;
	SqList csv_raw_list_b;

	init_linerseq(&csv_raw_list_a, sizeof(RawLine));
	init_linerseq(&csv_raw_list_b, sizeof(RawLine));

	clock_t load_start = clock();
	load_csv_file(csv_file_A, &csv_raw_list_a);
	load_csv_file(csv_file_B, &csv_raw_list_b);
	clock_t load_end = clock();
	printf("load csv consume:\t %f seconds\n", (double)(load_end-load_start)/CLOCKS_PER_SEC); 

	
	SqList csv_list_a;
	SqList csv_list_b;

	init_linerseq(&csv_list_a, sizeof(CSV_ROW_A));
	init_linerseq(&csv_list_b, sizeof(CSV_ROW_B));

	clock_t parse_start = clock();	
	parse_csv_list(&csv_raw_list_a, &csv_list_a, CSVTypeA);
	parse_csv_list(&csv_raw_list_b, &csv_list_b, CSVTypeB);
	clock_t parse_end = clock();
	printf("parse csvA,B consume:\t %f seconds\n", (double)(parse_end-parse_start)/CLOCKS_PER_SEC); 

	clock_t diff_start = clock();

	//difference A-B
	//strategy_one(&csv_list_a, &csv_list_b);

	strategy_two(&csv_list_a, &csv_list_b);

	//write A-B.csv to disk
	clock_t diff_end = clock();
	printf("compute A-B consume:\t %f seconds\n", (double)(diff_end-diff_start)/CLOCKS_PER_SEC); 
	
	clock_t write_start = clock();
	unsigned w_lines = write_csv_file(csv_file_output, &csv_list_a);
	clock_t write_end = clock();
	printf("write csv consume:\t %f seconds\n", (double)(write_end-write_start)/CLOCKS_PER_SEC); 	

	clock_t end = clock();
	printf("program total consume:\t %f seconds\n", (double)(end-start)/CLOCKS_PER_SEC);

	printf("totoal write:%u\n", w_lines);

	return 0;
}

