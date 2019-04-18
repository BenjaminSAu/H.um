/* alu.c
James Cameron and Benjamin Auerbach
Comp40 HW6
*/
#include "alu.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define TWOTO32 4294967296
#define CONTINUE -2

static void load_value(Alu_Obj alu, int rA, int rB);
static void output(Alu_Obj alu, int rC);
static void input(Alu_Obj alu, int rC);
static void add(Alu_Obj alu, int rA, int rB, int rC);
static void multiply(Alu_Obj alu, int rA, int rB, int rC);
static void divide(Alu_Obj alu, int rA, int rB, int rC);
static void nand(Alu_Obj alu, int rA, int rB, int rC);
static void cond_move(Alu_Obj alu, int rA, int rB, int rC);
static void seg_load(Alu_Obj alu, Mem_Obj mem, int rA, int rB, int rC);
static void seg_store(Alu_Obj alu, Mem_Obj mem, int rA, int rB, int rC);
static void map_seg(Alu_Obj alu, Mem_Obj mem, int rB, int rC);
static void unmap_seg(Alu_Obj alu, Mem_Obj mem, int rC);
static void load_prog(Alu_Obj alu, Mem_Obj mem, int rB);

enum Instructions {CMOVE, SEGL, SEGS, ADD, MLT, DIV, NAND, HALT, MSEG, USEG,
    OUT, IN, LOAP, LOAV};

    Alu_Obj Alu_new()
    {
        Alu_Obj my_Alu = malloc(sizeof(*my_Alu));
        assert(my_Alu != NULL);
        for(int i = 0; i < 8; i++)
        {
                my_Alu->registers[i] = 0;
        }
        return my_Alu;
}

void Alu_free(Alu_Obj *alu)
{
        free(*alu);
        return;
}

int execute(Alu_Obj alu, Mem_Obj mem, int ins, int rA, int rB, int rC)
{
        enum Instructions input_ins;
        input_ins = ins;
        switch(input_ins)
        {
                case LOAV:
                load_value(alu, rA, rB);
                break;
                case CMOVE:
                cond_move(alu, rA, rB, rC);
                break;
                case SEGL:
                seg_load(alu, mem, rA, rB, rC);
                break;
                case SEGS:
                seg_store(alu, mem, rA, rB, rC);
                break;
                case ADD:
                add(alu, rA, rB, rC);
                break;
                case MLT:
                multiply(alu, rA, rB, rC);
                break;
                case DIV:
                divide(alu, rA, rB, rC);
                break;
                case NAND:
                nand(alu, rA, rB, rC);
                break;
                case HALT:
                return -1;
                break;
                case MSEG:
                map_seg(alu, mem, rB, rC);
                break;
                case USEG:
                unmap_seg(alu, mem, rC);
                break;
                case OUT:
                output(alu, rC);
                break;
                case IN:
                input(alu, rC);
                break;
                case LOAP:
                load_prog(alu, mem, rB);
                return alu->registers[rC];
                break;
        }
        return CONTINUE;
}

/* Function: load_value
* Inputs: Alu_Obj, 2 ints
* Returns: None
* Does: Loads a value into a register
*/
static inline void load_value(Alu_Obj alu, int rA, int rB)
{
        alu->registers[rA] = rB;
        return;
}
/* Function: output
* Inputs: Alu_Obj, 1 int
* Returns: None
* Does: Prints a register as a character
*/
static inline void output(Alu_Obj alu, int rC)
{
        assert(alu->registers[rC] <= 255);
        fprintf(stdout, "%c", (char)alu->registers[rC]);
        return;
}
/* Function: input
* Inputs: Alu_Obj, 1 int
* Returns: None
* Does: Saves a character's ASCII value into a register
*/
static inline void input(Alu_Obj alu, int rC)
{
        int in = fgetc(stdin);
        assert(in <= 255);
        if (in == EOF) {
                alu->registers[rC] = ~0U;
        }
        else {
                alu->registers[rC] = in;
        }
        return;
}
/* Function: add
* Inputs: Alu_Obj, 3 ints
* Returns: None
* Does: Adds the values of 2 registers, and stores the result in a 3rd
*/
static inline void add(Alu_Obj alu, int rA, int rB, int rC)
{
        alu->registers[rA] = (alu->registers[rB] + alu->registers[rC])
                                                            % TWOTO32;
        return;
}
/* Function: multiply
* Inputs: Alu_Obj, 3 ints
* Returns: None
* Does: Multipliess the values of 2 registers, and stores the result in a 3rd
*/
static inline void multiply(Alu_Obj alu, int rA, int rB, int rC)
{
        alu->registers[rA] = (alu->registers[rB] * alu->registers[rC])
                                                        % TWOTO32;
        return;
}
/* Function: divide
* Inputs: Alu_Obj, 3 ints
* Returns: None
* Does: Divides the values of 2 registers, and stores the result in a 3rd
*/
static inline void divide(Alu_Obj alu, int rA, int rB, int rC)
{
        alu->registers[rA] = alu->registers[rB] / alu->registers[rC];
        return;
}
/* Function: nand
* Inputs: Alu_Obj, 3 ints
* Returns: None
* Does: Bitwise nand's the values of 2 registers, stores the result in a 3rd
*/
static inline void nand(Alu_Obj alu, int rA, int rB, int rC)
{
        alu->registers[rA] = ~(alu->registers[rB] & alu->registers[rC]);
        return;
}
/* Function: cond_move
* Inputs: Alu_Obj, 3 ints        alu->registers[rA] = alu->registers[rB] / alu->registers[rC];
* Returns: None
* Does: Moves value of rB into rA when rC is not 0
*/
static inline void cond_move(Alu_Obj alu, int rA, int rB, int rC)
{
        if (alu->registers[rC] != 0) {
                alu->registers[rA] = alu->registers[rB];
        }
        return;
}
/* Function: seg_load
* Inputs: Alu_Obj, Mem_Obj, 3 ints
* Returns: None
* Does: Grabs a piece of memory identified by 2 registers, and stores the
        result in a 3rd
*/
static inline void seg_load(Alu_Obj alu, Mem_Obj mem, int rA, int rB, int rC)
{
        alu->registers[rA] = Memory_get(mem, alu->registers[rB],
                                            alu->registers[rC]);
        return;
}
/* Function: seg_store
* Inputs: Alu_Obj, Mem_Obj, 3 ints
* Returns: None
* Does: Stores a register value at memory location identified by 2 registers,
         and stores the result in a 3rd
*/
static inline void seg_store(Alu_Obj alu, Mem_Obj mem, int rA, int rB, int rC)
{
        Memory_put(mem, alu->registers[rA], alu->registers[rB],
                                           alu->registers[rC]);
        return;
}
/* Function: map_seg
* Inputs: Alu_Obj, Mem_Obj, 2 ints
* Returns: None
* Does: Creates a new memory segment (length defined in rC) and stores ID
*       in rB
*/
static inline void map_seg(Alu_Obj alu, Mem_Obj mem, int rB, int rC)
{
        alu->registers[rB] = (uint32_t)Memory_map(mem, alu->registers[rC]);
        return;
}
/* Function: unmap_seg
* Inputs: Alu_Obj, Mem_Obj, 1 int
* Returns: None
* Does: Removes the memory segment specified in rC
*/
static inline void unmap_seg(Alu_Obj alu, Mem_Obj mem, int rC)
{
        Memory_unmap(mem, alu->registers[rC]);
        return;
}
/* Function: load_prog
* Inputs: Alu_Obj, Mem_Obj, 2 ints
* Returns: None
* Does: Loads a segment specified by rB into seg 0
*/
static inline void load_prog(Alu_Obj alu, Mem_Obj mem, int rB)
{
        if(alu->registers[rB] != 0)
        {
                Memory_load(mem, alu->registers[rB]);
        }
        return;
}
