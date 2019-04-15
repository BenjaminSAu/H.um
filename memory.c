/* memory.c
James Cameron and Benjamin Auerbach
Comp40 HW6
*/
#include <uarray.h>
#include <seq.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "memory.h"

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
        UArray_T *segment = (UArray_T *)UArray_at(mem->segments, seg_id);
        uint32_t *return_word = (uint32_t *)(uintptr_t)UArray_at(*segment, 
                                                                  offset);
        return *return_word;
}

void Memory_put(Mem_Obj mem, int seg_id, int offset, uint32_t word)
{
        assert(mem != NULL);
        UArray_T *segment = (UArray_T *)UArray_at(mem->segments, seg_id);
        uint32_t *located = (uint32_t *)(uintptr_t)UArray_at(*segment, offset);
        *located = word;
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
        UArray_T *zero_seg;
        UArray_T *load_seg;
        UArray_T *copy_seg;
        zero_seg = (UArray_T *)UArray_at(mem->segments, 0);
        load_seg = (UArray_T *)UArray_at(mem->segments, seg_id);
        UArray_T copy_seg_holder = UArray_copy(*load_seg, 
                                UArray_length(*load_seg));
        copy_seg = &copy_seg_holder;
        UArray_free(&(*zero_seg));
        *zero_seg = *copy_seg;
        return;
}