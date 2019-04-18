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
#include "alu.h"

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
        ins = decode(mem->seg_zero[program_counter], &rA, &rB, &rC);
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
        mem->segments = UArray_new(4, sizeof(UArray_T));
        mem->unmapped = Seq_new(32);
        mem->counter = 0;
        return mem;
}

void Memory_free(Mem_Obj *mem)
{
        int length = (*mem)->counter;
        for (int i = 0; i < length; i++)
        {
                UArray_T *segment = (UArray_T *)UArray_at((*mem)->segments, i);
                if (*segment != NULL)
                {
                        UArray_free(&(*segment));
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
        uint32_t *return_word = NULL;
        if (seg_id != 0) {
          UArray_T *segment = (UArray_T *)UArray_at(mem->segments, seg_id);
          return_word = (uint32_t *)(uintptr_t)UArray_at(*segment,
                                                                    offset);
        }
        else {
          return mem->seg_zero[offset];
        }

        return *return_word;
}

void Memory_put(Mem_Obj mem, int seg_id, int offset, uint32_t word)
{
        assert(mem != NULL);
        if (seg_id != 0) {
            UArray_T *segment = (UArray_T *)UArray_at(mem->segments, seg_id);
            uint32_t *located = (uint32_t *)(uintptr_t)UArray_at(*segment, offset);
            *located = word;
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

        UArray_T *new_seg = (UArray_T *)UArray_at(mem->segments, seg_id);
        *new_seg = UArray_new(length, sizeof(uint32_t));
        return (uint32_t)seg_id;
}

void Memory_unmap(Mem_Obj mem, int seg_id)
{
        UArray_T *segment = (UArray_T *)UArray_at(mem->segments, seg_id);
        UArray_free(&(*segment));
        *segment = NULL;
        int *insert = malloc(sizeof(int));
        assert(insert != NULL);
        *insert = seg_id;
        Seq_addhi(mem->unmapped, insert);
        return;
}

void Memory_load(Mem_Obj mem, int seg_id)
{
        UArray_T *load_seg;
        load_seg = (UArray_T *)UArray_at(mem->segments, seg_id);
        free(mem->seg_zero);
        mem->seg_zero = NULL;
        int seg_length = UArray_length(*load_seg);
        mem->seg_zero = malloc(sizeof(uint32_t) * seg_length);
        assert(mem->seg_zero != NULL);

        for (int i = 0; i < seg_length; i++)
        {

            mem->seg_zero[i] = *((uint32_t *)(uintptr_t)UArray_at(*load_seg, i));
        }
        return;
}
