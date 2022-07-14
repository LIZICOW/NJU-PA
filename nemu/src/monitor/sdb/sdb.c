#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/vaddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
void free_wp(int n);
void new_wp(char* change);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {

  return -1;
}

static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);


static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si","single step",cmd_si},
  { "info","show the program status",cmd_info},
  { "x","scan the memory",cmd_x},
  { "p","expression ealuation", cmd_p},
  { "w","monitor the expr given", cmd_w},
  { "d", "delete the monitor point n", cmd_d}
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_si(char *args){
	char *arg = strtok(NULL, " ");
	int n = 0;
	if(arg != NULL)
		n = atoi(arg);
	if(n == 0)
		n = 1;
	cpu_exec(n);
	return 0;
}
static int cmd_info(char *args){
	char *arg = strtok(NULL, " ");
	if(strcmp(arg,"r") == 0)
		isa_reg_display();
	return 0;
}

static int cmd_p(char *args){
	//char *arg = strtok(NULL, " ");
	if(args == NULL){
		printf("expression error!\n");
		return 0;
	}
	bool success = 1;
	uint32_t res = expr(args, &success);
	if(!success){
		printf("Expression error\n");
		return 0;
	}
	printf("%d\n", res);
	return 0;
}

static int cmd_w(char *args){
	if(args == NULL){
        printf("expression error!\n");
         return 0;
	}
	new_wp(args);
	printf("Watchpoint set!\n");
	return 0;
}
static int cmd_d(char *args){
	char* arg = strtok(NULL, " ");
	if(args == NULL){
        printf("expression error!\n");
        return 0;
	}
	int n = atoi(arg);
	free_wp(n);
	return 0;
}
static paddr_t hex_to_dec(const char *q){
	char p[9];
	strcpy(p, q);
	paddr_t res = 0;
	int j = 1;
	for(int i = strlen(p) - 1;i >= 0;i--){
		if(p[i] >= 'A' && p[i] <= 'F')
			p[i] += 32;
		if(p[i] >= 'a' && p[i] <= 'f')
			res += (p[i] - 'a' + 10) * j;
		else if(p[i] >= '0' &&p[i] <= '9')
			res += (p[i] - '0') * j;
		else
			printf("Memory input error!\n");
		j *= 16;
	}
	return res;
}

static int cmd_x(char *args){
	char *arg[2];
	arg[0] = strtok(NULL, " ");
	arg[1] = strtok(NULL, " ");
//	printf("%s\n %s\n",arg[0],arg[1]);i
	paddr_t addr = hex_to_dec(arg[1] + 2);
	int n = atoi(arg[0]);
	printf("%s\t\t%-34s%-32s\n", "addr", "16进制", "10进制");
	for(int i = 0;i < n; i++){
		printf("0x%0x:\t", addr + i * 4);
		for(int j = 0;j < 4; j++){
			printf("0x%-4x", vaddr_read(addr + i * 4 + j, 1));
		}
		printf("\t");
		for(int j = 0;j < 4; j++){
 			printf("%-4d ", vaddr_read(addr + i * 4 + j, 1));
		}
		printf("\n");
	}
	return 0;
}

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);
    
	/* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }
    
	/* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
