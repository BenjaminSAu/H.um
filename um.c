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
#include <seq.h>
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

#define INS_MASK 0xF0000000
#define LOAV_REG_MASK 0x0E000000
#define LOAV_VAL_MASK 0x01ffffff
#define REG_C_MASK 0x0000007
#define REG_B_MASK 0x0000038
#define REG_A_MASK 0x00001c0

#define TWOTO32 4294967296
#define CONTINUE -2

static void load_value(int rA, int rB);
static void output(int rC);
static void input(int rC);
static void add(int rA, int rB, int rC);
static void multiply(int rA, int rB, int rC);
static void divide(int rA, int rB, int rC);
static void nand(int rA, int rB, int rC);
static void cond_move(int rA, int rB, int rC);
static void seg_load(Mem_Obj mem, int rA, int rB, int rC);
static void seg_store(Mem_Obj mem, int rA, int rB, int rC);
static void map_seg(Mem_Obj mem, int rB, int rC);
static void unmap_seg(Mem_Obj mem, int rC);
static void load_prog(Mem_Obj mem, int rB);

uint32_t registers[8];

enum Instructions {CMOVE, SEGL, SEGS, ADD, MLT, DIV, NAND, HALT, MSEG, USEG,
    OUT, IN, LOAP, LOAV};
int execute(Mem_Obj mem, int ins, int rA, int rB, int rC);
/*Name: Alu_new
  Inputs: none
  Returns: Alu_obj
  Does: Creates and initializes ALU
*/


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

    int rA = 0;
    int rB = 0;
    int rC = 0;
    uint32_t program_counter = 0;
    int execute_val = 0;
    int ins = 0;

/* Run each instruction from segment zero of memory */
    while (execute_val != HALT_RET)
    {
      // fprintf(stderr, "Zero_seg at 0: %u\n", mem->seg_zero[0]);
      // fprintf(stderr, "prg counter: %u\n", program_counter);
        ins = decode(mem->seg_zero[program_counter], &rA, &rB, &rC);
        execute_val = execute(mem, ins, rA, rB, rC);
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

    Memory_free(&mem);
    return EXIT_SUCCESS;
}

int decode(uint32_t word, int *rA, int *rB, int *rC)
{
    uint32_t ins;
    ins = (word & INS_MASK) >> INS_LSB;
/* Handle irregular format of Load Value instruction: */
    if(ins == LOAV_INS)
    {
        *rA = (word & LOAV_REG_MASK) >> LOAV_A_LSB;
/* rB used to store val for convenience, not a literal register */
        *rB = word & LOAV_VAL_MASK;
    }
/* Default register storage format: */
    else
    {
        *rA = (word & REG_A_MASK) >> REG_A_LSB;
        *rB = (word & REG_B_MASK) >> REG_B_LSB;
        *rC = word & REG_C_MASK;
    }
    return ins;
}

Mem_Obj load_um(FILE *um_file, int num_instructions)
{
    Mem_Obj mem = Memory_new();
    mem->seg_zero = malloc(sizeof(uint32_t) * num_instructions);
    assert(mem->seg_zero != NULL);
    Memory_map(mem, 0);

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
        mem->seg_zero[i] = (uint32_t)ins_word;
    }
    return mem;
}

Mem_Obj Memory_new()
{
        Mem_Obj mem = malloc(sizeof(*mem));
        assert(mem != NULL);
        mem->segments = UArray_new(4, sizeof(uint32_t *));
        mem->unmapped = Seq_new(32);
        mem->counter = 0;
        return mem;
}

void Memory_free(Mem_Obj *mem)
{
        free((*mem)->seg_zero);
        int length = (*mem)->counter;
        uint32_t **segment = segment = (uint32_t **)UArray_at((*mem)->segments, 0);
        free(*segment);

        for (int i = 1; i < length; i++)
        {
                segment = (uint32_t **)UArray_at((*mem)->segments, i);
                (*segment)--;
                if (*segment != NULL)
                {
                        free(*segment);
                }
        }
        UArray_T segments = (*mem)->segments;
        UArray_free(&segments);
/* Free remaining members of unmapped sequence */
        while(Seq_length((*mem)->unmapped) != 0)
        {
                void *to_free;
                to_free = Seq_remhi((*mem)->unmapped);
                free(to_free);
        }
        Seq_free(&((*mem)->unmapped));
        free(*mem);
        return;
}

uint32_t Memory_get(Mem_Obj mem, int seg_id, int offset)
{
        assert(mem != NULL);
        if (seg_id != 0) {
          uint32_t **segment = (uint32_t **)UArray_at(mem->segments, seg_id);
          return (*segment)[offset];
        }
        else {
          return mem->seg_zero[offset];
        }
}

void Memory_put(Mem_Obj mem, int seg_id, int offset, uint32_t word)
{
        assert(mem != NULL);
        if (seg_id != 0) {
            uint32_t **segment = (uint32_t **)UArray_at(mem->segments, seg_id);
            (*segment)[offset] = word;
            return;
        }
        mem->seg_zero[offset] = word;
        return;
}

uint32_t Memory_map(Mem_Obj mem, int length)
{
        int seg_id = 0;

        if(Seq_length(mem->unmapped) == 0)
        {
                if((unsigned)UArray_length(mem->segments) <= mem->counter)
                {
                        UArray_resize(mem->segments,
                        (UArray_length(mem->segments) * 2));
                }
                seg_id = mem->counter++;
        }
        else
        {
                int *seg_ptr;
                seg_ptr = (int *)Seq_remhi(mem->unmapped);
                seg_id = *seg_ptr;
                free(seg_ptr);
        }
        uint32_t **new_seg = (uint32_t **)UArray_at(mem->segments, seg_id);
        *new_seg = calloc(length + 1, sizeof(uint32_t));
        assert(*new_seg != NULL);

        if (length != 0) {
          **new_seg = (uint32_t)length;
          (*new_seg)++;
        }

        return (uint32_t)seg_id;
}

void Memory_unmap(Mem_Obj mem, int seg_id)
{
        uint32_t **segment = (uint32_t **)UArray_at(mem->segments, seg_id);
        (*segment)--;
        free(*segment);
        *segment = NULL;
        (*segment)++;
        int *insert = malloc(sizeof(int));
        assert(insert != NULL);
        *insert = seg_id;
        Seq_addhi(mem->unmapped, insert);
        return;
}

void Memory_load(Mem_Obj mem, int seg_id)
{
        uint32_t **load_seg = (uint32_t **)UArray_at(mem->segments, seg_id);
        free(mem->seg_zero);
        mem->seg_zero = NULL;
        (*load_seg)--;
        uint32_t seg_length = **load_seg;
        (*load_seg)++;
        // uint32_t seg_length = *((*load_seg) - 4);
        fprintf(stderr, "seg_id %d seg_length %u\n", seg_id, seg_length);
        mem->seg_zero = malloc(sizeof(uint32_t) * seg_length);
        assert(mem->seg_zero != NULL);

        for (unsigned i = 0; i < seg_length; i++)
        {
            mem->seg_zero[i] = (*load_seg)[i];
            // fprintf(stderr, "setting %u\n", mem->seg_zero[i-1]);
        }
        return;
}

int execute(Mem_Obj mem, int ins, int rA, int rB, int rC)
{
        enum Instructions input_ins;
        input_ins = ins;
        switch(input_ins)
        {
                case LOAV:
                load_value(rA, rB);
                break;
                case CMOVE:
                cond_move(rA, rB, rC);
                break;
                case SEGL:
                seg_load(mem, rA, rB, rC);
                break;
                case SEGS:
                seg_store(mem, rA, rB, rC);
                break;
                case ADD:
                add(rA, rB, rC);
                break;
                case MLT:
                multiply(rA, rB, rC);
                break;
                case DIV:
                divide(rA, rB, rC);
                break;
                case NAND:
                nand(rA, rB, rC);
                break;
                case HALT:
                return -1;
                break;
                case MSEG:
                map_seg(mem, rB, rC);
                break;
                case USEG:
                unmap_seg(mem, rC);
                break;
                case OUT:
                output(rC);
                break;
                case IN:
                input(rC);
                break;
                case LOAP:
                load_prog(mem, rB);
                return registers[rC];
                break;
        }
        return CONTINUE;
}

/* Function: load_value
* Inputs: Alu_Obj, 2 ints
* Returns: None
* Does: Loads a value into a register
*/
static inline void load_value(int rA, int rB)
{
        registers[rA] = rB;
        return;
}
/* Function: output
* Inputs: Alu_Obj, 1 int
* Returns: None
* Does: Prints a register as a character
*/
static inline void output(int rC)
{
        assert(registers[rC] <= 255);
        fprintf(stdout, "%c", (char)registers[rC]);
        return;
}
/* Function: input
* Inputs: Alu_Obj, 1 int
* Returns: None
* Does: Saves a character's ASCII value into a register
*/
static inline void input(int rC)
{
        int in = fgetc(stdin);
        assert(in <= 255);
        if (in == EOF) {
                registers[rC] = ~0U;
        }
        else {
                registers[rC] = in;
        }
        return;
}
/* Function: add
* Inputs: Alu_Obj, 3 ints
* Returns: None
* Does: Adds the values of 2 registers, and stores the result in a 3rd
*/
static inline void add(int rA, int rB, int rC)
{
        registers[rA] = (registers[rB] + registers[rC])
                                                            % TWOTO32;
        return;
}
/* Function: multiply
* Inputs: Alu_Obj, 3 ints
* Returns: None
* Does: Multipliess the values of 2 registers, and stores the result in a 3rd
*/
static inline void multiply(int rA, int rB, int rC)
{
        registers[rA] = (registers[rB] * registers[rC])
                                                        % TWOTO32;
        return;
}
/* Function: divide
* Inputs: Alu_Obj, 3 ints
* Returns: None
* Does: Divides the values of 2 registers, and stores the result in a 3rd
*/
static inline void divide(int rA, int rB, int rC)
{
        registers[rA] = registers[rB] / registers[rC];
        return;
}
/* Function: nand
* Inputs: Alu_Obj, 3 ints
* Returns: None
* Does: Bitwise nand's the values of 2 registers, stores the result in a 3rd
*/
static inline void nand(int rA, int rB, int rC)
{
        registers[rA] = ~(registers[rB] & registers[rC]);
        return;
}
/* Function: cond_move
* Inputs: Alu_Obj, 3 ints        registers[rA] = registers[rB] / registers[rC];
* Returns: None
* Does: Moves value of rB into rA when rC is not 0
*/
static inline void cond_move(int rA, int rB, int rC)
{
        if (registers[rC] != 0) {
                registers[rA] = registers[rB];
        }
        return;
}
/* Function: seg_load
* Inputs: Alu_Obj, Mem_Obj, 3 ints
* Returns: None
* Does: Grabs a piece of memory identified by 2 registers, and stores the
        result in a 3rd
*/
static inline void seg_load(Mem_Obj mem, int rA, int rB, int rC)
{
        registers[rA] = Memory_get(mem, registers[rB],
                                            registers[rC]);
        return;
}
/* Function: seg_store
* Inputs: Alu_Obj, Mem_Obj, 3 ints
* Returns: None
* Does: Stores a register value at memory location identified by 2 registers,
         and stores the result in a 3rd
*/
static inline void seg_store(Mem_Obj mem, int rA, int rB, int rC)
{
        Memory_put(mem, registers[rA], registers[rB],
                                           registers[rC]);
        return;
}
/* Function: map_seg
* Inputs: Alu_Obj, Mem_Obj, 2 ints
* Returns: None
* Does: Creates a new memory segment (length defined in rC) and stores ID
*       in rB
*/
static inline void map_seg(Mem_Obj mem, int rB, int rC)
{
        registers[rB] = (uint32_t)Memory_map(mem, registers[rC]);
        return;
}
/* Function: unmap_seg
* Inputs: Alu_Obj, Mem_Obj, 1 int
* Returns: None
* Does: Removes the memory segment specified in rC
*/
static inline void unmap_seg(Mem_Obj mem, int rC)
{
        Memory_unmap(mem, registers[rC]);
        return;
}
/* Function: load_prog
* Inputs: Alu_Obj, Mem_Obj, 2 ints
* Returns: None
* Does: Loads a segment specified by rB into seg 0
*/
static inline void load_prog(Mem_Obj mem, int rB)
{
        if(registers[rB] != 0)
        {
                Memory_load(mem, registers[rB]);
        }
        return;
}
