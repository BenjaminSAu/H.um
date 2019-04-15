/* alu.h
James Cameron and Benjamin Auerbach
Comp40 HW6
*/
#include <stdint.h>
#include "memory.h"

#ifndef ALU_H
#define ALU_H

typedef struct Alu_Obj
{
    uint32_t registers[8];
} *Alu_Obj;

/*Name: execute
  Input: 4 ints, Mem_Obj, Alu_Obj
  Returns: int
  Does: Takes in the instruction and registers needed for
         an instruction and executes the correct instruction. 
         Return is a signifier of the state of the program counter.
*/
extern int execute(Alu_Obj alu, Mem_Obj mem, int ins, int rA, int rB, int rC);
/*Name: Alu_new
  Inputs: none
  Returns: Alu_obj
  Does: Creates and initializes ALU 
*/
extern Alu_Obj Alu_new();
/*Name: Alu_free
  Inputs: Alu_obj *
  Returns: void
  Does: Frees ALU 
*/
extern void Alu_free(Alu_Obj *alu);

#endif