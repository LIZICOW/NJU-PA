#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]))
enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_NOTEQ, TK_NUM, TK_HEX, TK_OR, TK_AND, TK_REG, TK_PNT, TK_MINUS
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"!=", TK_NOTEQ},
  {"\\-", '-'},
  {"\\*",'*'},
  {"/", '/'},
  {"\\(", '('},
  {"\\)", ')'},
  {"\\b[0-9]+\\b", TK_NUM},
  {"\\%", '%'},
  {"0([xX]([a-fA-F0-9]+))", TK_HEX},
  {"\\|\\|", TK_OR},
  {"&&", TK_AND},
  {"\\$(\\$0|ra|pc|[sgt]p|t[0-6]|s[0-9]|a[0-7]|s1[0-1])", TK_REG}
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[128] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;
static int flag = 0;
static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
	if(rules[i].token_type == TK_NOTYPE)
	  continue;
	if(rules[i].token_type == '*'){
	  if(flag > 0 && (tokens[flag - 1].type == TK_NUM || tokens[flag - 1].type == TK_HEX || tokens[flag - 1].type == ')'))
	    tokens[flag].type = '*';
	  else
	    tokens[flag].type = TK_PNT;
	}	
	else if(rules[i].token_type == '-'){
	  if(flag > 0 && (tokens[flag - 1].type == TK_NUM || tokens[flag - 1].type == TK_HEX || tokens[flag - 1].type == ')'))
	    tokens[flag].type = '-';
	  else
	    tokens[flag].type = TK_MINUS;
	}
	else
	  tokens[flag].type = rules[i].token_type;
        switch (rules[i].token_type) {
          case TK_NUM:
          case TK_HEX:
          case TK_REG:
            strncpy(tokens[flag].str, substr_start, substr_len);
            break;
          case '+':
          case '-':
          case '*':
          case '/':
          case '(':
          case ')':
	  case '%':
          case TK_EQ:
          case TK_NOTEQ:
          case TK_AND:
          case TK_OR:
            break;
          default: TODO();break;
        }
        flag++;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool check_parentheses(int p, int q, bool *success){
	if(tokens[p].type != '(' || tokens[q].type != ')')
		return false;
	int BracketNum = 0;/* When encounter '(' ++, otherwise --  */
	if(p + 1 == q){/* Nothing in the brackets  */
		success = false;
		return false;
	}
	/* Check if the brackets match correctly  */
	for(int i = p + 1;i < q;i++){
		if(tokens[i].type == '(')
			BracketNum++;
		else if(tokens[i].type == ')')
			BracketNum--;
		if(BracketNum < 0)
			return false;
	}
	if(BracketNum != 0){
		success = false;
		return false;
	}
	/* Check whether expression is enclosed by a pair of parentheses
	bool LeftOrRight = 0;  left 0  right 1 
	for(int i = p;i < q;i++){
        if(tokens[i].type == '(' && LeftOrRight == 1)
			return false;
		if(tokens[i].type == ')' && LeftOrRight == 0)
			return false;
		if(tokens[i].type == '(' )
    		LeftOrRight = 0;
	    else if(tokens[i].type == ')')
     		LeftOrRight = 1;
	} */
	return true;
}
static uint32_t hex_to_dec(const char *q){
	char p[9];
	strcpy(p, q);
	uint32_t res = 0;
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
static uint32_t eval(int p, int q, bool* success){
  if(p > q){
    success = false;
    return 0;
  }
  else if(p + 1 == q)
  {
    if(tokens[p].type == TK_MINUS && (tokens[q].type == TK_NUM || tokens[q].type == TK_HEX)){
      if(tokens[q].type == TK_NUM){
        return -atoi(tokens[q].str);
      }
      else if(tokens[q].type == TK_HEX){
        return -hex_to_dec(tokens[q].str + 2);
      }
    }
   // else if(tokens[p].type == TK_PNT && tokens[q].type == TK_REG){
   //   return isa_reg_str2val(tokens[q].str, success);
   // }
    else{
      success = false;
      return 0;
    }
  }
  else if(p == q){
    if(tokens[p].type == TK_NUM){
      return atoi(tokens[p].str);
    }
    else if(tokens[p].type == TK_HEX){
      return hex_to_dec(tokens[p].str + 2);
    }
    else if(tokens[q].type == TK_REG){
      return isa_reg_str2val(tokens[q].str + 1, success);
    }
    else{
      success = false;
      return 0;
    }
  }
  else if(check_parentheses(p, q, success) == true){
    return eval(p + 1, q - 1, success);
  }
  else{
    int op[] = {-1, -1, -1, -1};
	int InThePnt = 0;
	for(int i = q;i >= p;i--){
	  if(tokens[i].type == ')')
	    InThePnt++;
	  else if(tokens[i].type == '(')
	    InThePnt--;
	  if(InThePnt > 0)
  	    continue;
	  switch(tokens[i].type){
	    case TK_OR:
	    case TK_AND:
	      if(op[0] == -1)
	        op[0] = i;
	      break;
	    case TK_EQ:
	    case TK_NOTEQ:
	      if(op[1] == -1)
	        op[1] = i;
	    case '+':
	    case '-':
	      if(op[2] == -1)
	        op[2] = i;
	      break;
	    case '*':
	    case '/':
	    case '%':
	      if(op[3] == -1)
	        op[3] = i;
	      break;
	    
	    default:break;
	  }
    }
  //  printf("%d %d %d %d", op[0],op[1],op[2],op[3]);
    uint32_t val1 = 0;
    uint32_t val2 = 0;
    for(int i = 0;i < ARRAY_SIZE(op); i++){
      if(op[i] != -1){
        val1 = eval(p, op[i] - 1, success);
        val2 = eval(op[i] + 1, q, success); 
       
        uint32_t res;
        switch (tokens[op[i]].type) {
          case '+': res = val1 + val2;break;
          case '-': res = val1 - val2;break;
          case '*': res = val1 * val2;break;
          case '/': res = val1 / val2;break;
          case '%': res = val1 % val2;break;
          case TK_OR: res = val1 || val2;break;
          case TK_AND: res = val1 && val2;break;
          case TK_EQ: res = (val1 == val2);break;
          case TK_NOTEQ: res = (val1 != val2);break;
          default: assert(0);
        } 
        //printf("%u %d:%c %u = %u\n",val1, op[i],tokens[op[i]].type, val2, res);
        return res;
        break;
      }    
    }
  }
  return 0;
}


word_t expr(char *e, bool *success) {
  memset(tokens, 0, sizeof(tokens));
  flag = 0;
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  
  /* TODO: Insert codes to evaluate the expression. */
  word_t res = eval(0, flag - 1, success);
  
  
  //TODO();

  return res;
}
