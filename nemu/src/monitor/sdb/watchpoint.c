#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[128];/* store expression */
  bool success;
  uint32_t res;/* value of expr */
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
	wp_pool[i].success = true;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
void new_wp(char *expression){
	if(free_ == NULL){
		printf("no more free monitor\n");
		assert(0);
	}
	WP* p = free_;
	free_ = free_ -> next;
	p -> next = head;
	head = p;
	strcpy(p -> expr, expression);
	p -> res = expr(p -> expr, &(p -> success));
}

void free_wp(int n){
	WP* p = head;
	if(n >= NR_WP){
		printf("number too big\n");
		assert(0);
	}
	if(p -> NO == n){
		head = p -> next;
		p -> next = free_;
		free_ = p;
		return;
	}	
	while(p -> next != NULL){
		if(p -> next -> NO == n){
			WP* wp = p -> next;
			p -> next = wp -> next;
			wp -> next = free_;
			free_ = wp;
			printf("Delete the No.%d watchpoint\n", n);
			break;	
		}
		p = p -> next;
	}
	if(p -> next == NULL){
		printf("can not find the monitor: No.%d\n", n);
		assert(0);
	}
}

void scan_wp(bool* change){
	WP* p = head;
	while(p){
		uint32_t old = p -> res;	
		p -> res = expr(p -> expr, &(p -> success));
		if(!p -> success){
			printf("Watchpoint Error!!NO: %d, expr: %s \n", p -> NO, p -> expr);
			assert(0);
		}
		if(old != p -> res){
			*change = true;
			printf("Watchpoint:\tNo:%2d\texpr:%s\tOld_value:%0x\t%u\n", p -> NO, p -> expr, old, old);
			printf("          :\tNo:%2d\texpr:%s\tNew_value:%0x\t%u\n", p -> NO, p -> expr, p -> res, p -> res);
		}
		p = p -> next;
	}
}

void display_wp(){
	if(head == NULL){
		printf("NONE WATCHPOINT\n");
		return;
	}
	WP* p = head;
	printf("N0\texpr\tvalue_hex\tvalue_dec\n");
	while(p){
		printf("%d\t%s\t0x%0x\t%u\n", p -> NO, p -> expr, p -> res, p ->res);
		p = p -> next;
	}
}

