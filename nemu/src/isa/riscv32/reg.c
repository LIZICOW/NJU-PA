#include <isa.h>
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
  for(int i=0; i < 32; i ++)
    printf("%s\t0x%08x\t%-15u\n", regs[i], cpu.gpr[i]._32, cpu.gpr[i]._32);
  printf("%s\t0x%08x\t%-15u\n", "pc", cpu.pc, cpu.pc);
}

word_t isa_reg_str2val(const char *s, bool *success) {
  word_t ret = 0;
  int i = 0;
  if(strcmp(s, "pc") == 0){
    *success = true;
    return cpu.pc;
  }
  for(;i < 32;i++){
    if(strcmp(regs[i], s) == 0){
      //printf("%s\t0x%08x\t%-15u\n", regs[i], cpu.gpr[i]._32, cpu.gpr[i]._32);
      ret = cpu.gpr[i]._32;
      *success = true;
    }
  }
  if(i == 32)
    success = 0;
  return ret;
}
