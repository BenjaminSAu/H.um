/* um.c 
James Cameron and Benjamin Auerbach
Comp40 HW6
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>
#include <bitpack.h>
#include <assert.h>
#include <um-dis.h>
#include "alu.h"
#include "memory.h"

#define WORD_WIDTH 8
#define REG_WIDTH 3
#define INS_WIDTH 4
#define VAL_WIDTH 25

#define NUM_BYTES 4
#define HALT_RET -1

#define INS_LSB 28
#define REG_A_LSB 6
#define REG_B_LSB 3
#define REG_C_LSB 0

#define LOAV_A_LSB 25
#define LOAV_VAL_LSB 0
#define LOAV_INS 13


/*
* Function: decode
* Input: 3 ints and uint32_t
* Returns: int
* Does: Takes in a single word and breaks it down into the instruction and the
*       three registers contained within. 
*/
int decode(uint32_t word, int *rA, int *rB, int *rC);
/*
* Function: load_ume
* Input: int and FILE *
* Returns: Mem_Obj
* Does: Reads the initial UM instructions from the input file, returning a 
*       memory with num_instructions insturctions loaded in seg 0. 
*/
Mem_Obj load_um(FILE *um_file, int num_instructions);


int main(int argc, char const *argv[])
{
    if(argc != 2) {
        fprintf(stderr, "UM must be run with a single .um file\n");
        return EXIT_FAILURE;
    }

/* Retrieve number of instructions from given file */
    struct stat file_info;
    stat(argv[1], &file_info);
    int num_instructions = file_info.st_size / NUM_BYTES;

    FILE *um_file = fopen(argv[1], "r");
    assert(um_file != NULL);

    Mem_Obj mem = load_um(um_file, num_instructions);

    fclose(um_file);

    Alu_Obj my_alu = Alu_new();

    int rA = 0;
    int rB = 0;
    int rC = 0;
    uint32_t program_counter = 0;
    int execute_val = 0;
    int ins = 0;

/* Run each instruction from segment zero of memory */
    while (execute_val != HALT_RET)
    {
        ins = decode(Memory_get(mem, 0, (uint32_t)program_counter), 
            &rA, &rB, &rC);
        execute_val = execute(my_alu, mem, ins, rA, rB, rC);
        if (execute_val == HALT_RET) {
            break;
        }
        else if (execute_val >= 0) {
            program_counter = (uint32_t)execute_val;
        }
        else {
            program_counter++;
        }
    }

    Alu_free(&my_alu);
    Memory_free(&mem);
    return EXIT_SUCCESS;
}

int decode(uint32_t word, int *rA, int *rB, int *rC)
{
    uint64_t ins;

    ins = Bitpack_getu((uint64_t)word, INS_WIDTH, INS_LSB);

/* Handle irregular format of Load Value instruction: */
    if(ins == LOAV_INS)
    {
        *rA = (int)Bitpack_getu((uint64_t)word, REG_WIDTH, LOAV_A_LSB);
/* rB used to store val for convenience, not a literal register */
        *rB = (int)Bitpack_getu((uint64_t)word, VAL_WIDTH, LOAV_VAL_LSB);
    }
/* Default register storage format: */
    else
    {
        *rA = (int)Bitpack_getu((uint64_t)word, REG_WIDTH, REG_A_LSB);
        *rB = (int)Bitpack_getu((uint64_t)word, REG_WIDTH, REG_B_LSB);
        *rC = (int)Bitpack_getu((uint64_t)word, REG_WIDTH, REG_C_LSB);
    }
    return ins;
}

Mem_Obj load_um(FILE *um_file, int num_instructions)
{
    Mem_Obj mem = Memory_new();
    Memory_map(mem, num_instructions);

    uint64_t ins_word = 0;
    int char_con = 0;

/* Grabs 32 bit instruction one byte at a time */
    for(int i = 0; i < num_instructions; i++)
    {
        char_con = fgetc(um_file);
        ins_word = Bitpack_newu(ins_word, WORD_WIDTH, 24, char_con);
        char_con = fgetc(um_file);
        ins_word = Bitpack_newu(ins_word, WORD_WIDTH, 16, char_con);
        char_con = fgetc(um_file);
        ins_word = Bitpack_newu(ins_word, WORD_WIDTH, 8, char_con);
        char_con = fgetc(um_file);
        ins_word = Bitpack_newu(ins_word, WORD_WIDTH, 0, char_con);
        Memory_put(mem, 0, i, (uint32_t)ins_word);
    }
    return mem;
}