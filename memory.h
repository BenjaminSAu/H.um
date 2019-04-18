/* memory.h
James Cameron and Benjamin Auerbach
Comp40 HW6
*/

#include <uarray.h>
#include <seq.h>
#include <stdint.h>

#ifndef MEMORY_H
#define MEMORY_H

typedef struct Mem_Obj
{
        UArray_T segments;
        Seq_T unmapped;
        uint32_t *seg_zero;
        unsigned counter;
} *Mem_Obj;

/*Name: Memory_new
Inputs: none
Returns: Mem_Obj
Does: Creates and initializes a memory */
extern Mem_Obj Memory_new();

/* Name: Memory_free
Inputs: Mem_Obj pointer
Returns: void
Does: Frees memory */
extern void Memory_free(Mem_Obj *mem);

/*Name: Memory_get
Inputs: 2 ints, Mem_Obj
Returns: uint32_t
Does: Goes to the location specified by the offset and
seg_id and fetches the word specified */
extern uint32_t Memory_get(Mem_Obj mem, int seg_id, int offset);

/* Name: Memory_put
Input: 2 ints, 1 uint32_t, Mem_Obj
Returns: void
Does: Goest to location specified by the offset and seg_id
and puts the specified word in that location. */
extern void Memory_put(Mem_Obj mem, int seg_id, int offset, uint32_t word);

/*Name: Memory_map
Inputs: int, Mem_Obj
Returns: void
Does: Creates a new memory segment of specified size,
placing it at the index returned. */
extern uint32_t Memory_map(Mem_Obj mem, int length);

/*Name: Memory_unmap
Inputs: int, Mem_Obj
Returns: void
Does: Deletes specified memory segment from memory */
extern void Memory_unmap(Mem_Obj mem, int seg_id);

/*Name: Memory_load
Inputs: int, Mem_Obj
Returns: void
Does: Replaces seg 0 in mem with segment specified by seg_id*/
extern void Memory_load(Mem_Obj mem, int seg_id);

#endif
