// Copyright (c) 2014, ultramailman
// This file is licensed under the MIT License.

#include "rhtable.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>



/* The hash table is made of a header and a variable lengthed array
of slots.
layout of the hash table:
struct rhtable{
	struct rhtable t;
	                           <- base starts here
	struct slot tmp;
	struct slot slots[t.slots];
};
Since Key and Val aren't known at compile time, pointer arithmetic is
required to access the array of slots.
*/
struct rhtable{
	struct rhspec const spec;
	uint32_t count;
	
	char base [];
};

/* tmp is a temporary slot to store evicted pairs */
static
uint32_t tmpOffset(struct rhtable const * t)
{
	return 0;
}
static
struct slot_header * tmpGet(struct rhtable * t)
{
	return (struct slot_header *)(t->base + tmpOffset(t));
}

/* slots are the memory to store the contents of the hash table */
static
uint32_t slotOffset(struct rhtable const * t, uint32_t i)
{
	struct rhspec const s = t->spec;
	return s.slotSize + i * s.slotSize;
}
static
struct slot_header * slotGet(struct rhtable const * t, uint32_t i)
{
	return (struct slot_header *)(t->base + slotOffset(t, i));
}
////////////////////////////////////////////////////////////
/* access the key of a slot */
static
void * slotGetKey(struct slot_header const * slot, struct rhtable const * t)
{
	struct rhspec const s = t->spec;
	return ((char *)slot) + s.keyOffset;
}
static
void slotSetKey(struct slot_header * slot, struct rhtable * t, void const * key)
{
	struct rhspec const s = t->spec;
	memcpy(slotGetKey(slot, t), key, s.keySize);
}
/* access the value of a slot*/
static
void * slotGetVal(struct slot_header const * slot, struct rhtable const * t)
{
	struct rhspec const s = t->spec;
	return ((char *)slot) + s.valOffset;
}
static
void slotSetVal(struct slot_header * slot, struct rhtable * t, void const * val)
{
	struct rhspec const s = t->spec;
	memcpy(slotGetVal(slot, t), val, s.valSize);
}
/* flag to indicate the emptyness of the slot*/
static
int slotIsEmpty(struct slot_header const* slot)
{
	return 0 == (slot->dib + 1);
}
static
void slotSetEmpty(struct slot_header * slot)
{
	slot->dib = 0;
	slot->dib--;
}
/////////////////////////////////////////////////////////////
uint32_t rhtable_count(struct rhtable const * t)
{
	return t->count;
}

uint32_t rhtable_slots(struct rhtable const * t)
{
	return t->spec.slots;
}
float rhtable_average_dib(struct rhtable const * t)
{
	float dibs = 0;
	for(uint32_t i = 0; i < rhtable_slots(t); i++) {
		struct slot_header * slot = slotGet(t, i);
		if(!slotIsEmpty(slot)) {
			dibs += slot->dib;
		}
	}
	return dibs / t->count;
}
struct rhtable * rhtable_create_(struct rhspec const spec)
{
	if(spec.slots < 2) {
		return NULL;
	} else if(spec.keySize == 0) {
		return NULL;
	} else if(spec.hashf == NULL) {
		return NULL;
	} else if(spec.eqf == NULL) {
		return NULL;
	}
	
	uint32_t tableSize= 0;
	tableSize += sizeof(struct rhtable);
	tableSize += spec.slotSize;
	tableSize += spec.slots * spec.slotSize;
	
	struct rhtable * t = calloc(1, tableSize);
	if(t != NULL) {
		struct rhtable tmp = {.spec = spec};
		memcpy(t, &tmp, sizeof tmp);
		for(uint32_t i = 0; i < spec.slots; i++) {
			struct slot_header * slot = slotGet(t, i);
			slotSetEmpty(slot);
		}
	}
	return t;
}
void rhtable_destroy(struct rhtable * t)
{
	free(t);
}
int rhtable_get(struct rhtable const * t, void const * key, void * rkey, void * rval)
{
	struct rhspec const s = t->spec;
	uint32_t const hash = s.hashf(key);
	uint32_t dib = 0;
	uint32_t probe = (hash + dib) % s.slots;
	do{
		struct slot_header * slot = slotGet(t, probe);
		
		if(slotIsEmpty(slot) || dib > slot->dib) {
			return 0;
		} else if(hash == slot->hash && s.eqf(key, slotGetKey(slot, t))) {
			if(rkey != NULL) {
				memcpy(rkey, slotGetKey(slot, t), s.keySize);
			}
			if(s.valSize > 0 && rval != NULL) {
				memcpy(rval, slotGetVal(slot, t), s.valSize);
			}
			return 1;
		} else {
			dib++;
			probe++;
			if(probe == s.slots) {
				probe = 0;
			}
		}
	}while(dib < s.slots);
	return 0;
}
// find the next empty slot
static
uint32_t nextEmpty(struct rhtable * t, uint32_t probe)
{
	struct rhspec const s = t->spec;
	do{
		struct slot_header * slot = slotGet(t, probe);
		if(slotIsEmpty(slot)){
			return probe;
		}
		
		probe++;
		if(probe == s.slots) {
			probe = 0;
		}
	}while(1);
}
// shift array to the right until an empty slot is probed
static
void rightShift(struct rhtable * t, uint32_t probe)
{
	struct rhspec const s = t->spec;
	uint32_t dst = nextEmpty(t, probe);
	uint32_t src;
	
	
	do{
		src = dst - 1;
		if(src > dst) {
			src = s.slots - 1;
		}
		struct slot_header * dstSlot = slotGet(t, dst);
		struct slot_header * srcSlot = slotGet(t, src);
		
		srcSlot->dib++;
		memcpy(dstSlot, srcSlot, s.slotSize);
		
		dst = src;
	}while(src != probe);
}
// insert a new item or update an existing item
static
int rhtable_set_(struct rhtable * t)
{
	struct rhspec const s = t->spec;
	struct slot_header * const tmp = tmpGet(t);
	do{
		uint32_t probe = (tmp->hash + tmp->dib) % s.slots;
		struct slot_header * slot = slotGet(t, probe);
		
		if(slotIsEmpty(slot)) {
			memcpy(slot, tmp, s.slotSize);
			t->count++;
			return 1;
		} else if(tmp->hash == slot->hash && s.eqf(slotGetKey(tmp, t), slotGetKey(slot, t))) {
			slotSetVal(slot, t, slotGetVal(tmp, t));
			return 1;
		} else if(tmp->dib > slot->dib) {
			rightShift(t, probe);
			memcpy(slot, tmp, s.slotSize);
			t->count++;
			return 1;
		} else {
			tmp->dib++;
		}
	}while( tmp->dib < s.slots );
	assert(0);
	return 0;
}
int rhtable_set(struct rhtable * t, void const * key, void const * val)
{
	struct rhspec const s = t->spec;
	if(t->count == s.slots) {
		return 0;
	}
	
	struct slot_header * const tmp = tmpGet(t);
	
	tmp->dib = 0;
	tmp->hash = s.hashf(key);
	slotSetKey(tmp, t, key);
	slotSetVal(tmp, t, val);
	
	return rhtable_set_(t);
}
static
void rhtable_del_shift(struct rhtable * t, uint32_t probe)
{
	struct rhspec const s = t->spec;
	uint32_t next = (probe + 1) % s.slots;
	struct slot_header * thisSlot = slotGet(t, probe);
	struct slot_header * nextSlot = slotGet(t, next);
	while(!slotIsEmpty(nextSlot) && nextSlot->dib != 0) {
		memcpy(thisSlot, nextSlot, s.slotSize);
		thisSlot->dib--;
		assert(thisSlot->dib + 1 > 0);
		
		next++;
		if(next == s.slots) {
			next = 0;
		}
		thisSlot = nextSlot;
		nextSlot = slotGet(t, next);
	}
	slotSetEmpty(thisSlot);
}
void rhtable_del(struct rhtable * t, void const * key)
{
	struct rhspec const s = t->spec;
	uint32_t const hash = s.hashf(key);
	uint32_t dib = 0;
	uint32_t probe = (hash + dib) % s.slots;
	do{
		struct slot_header * slot = slotGet(t, probe);
		
		if(slotIsEmpty(slot) || dib > slot->dib) {
			return;
		} else if(hash == slot->hash && s.eqf(key, slotGetKey(slot, t))) {
			slotSetEmpty(slot);
			t->count--;
			rhtable_del_shift(t, probe);
			return;
		} else {
			dib++;
			probe++;
			if(probe == s.slots) {
				probe = 0;
			}
		}
	}while(dib < s.slots);
}

static
struct rhiter rhtable_next_(struct rhtable const * t, uint32_t i)
{
	struct rhspec const s = t->spec;
	for(; i < s.slots; i++) {
		struct slot_header * slot = slotGet(t, i);
		if(!slotIsEmpty(slot)) {
			return (struct rhiter) {
				.key = slotGetKey(slot, t),
				.val = slotGetVal(slot, t),
				.i = i
			};
		}
	}
	return (struct rhiter) {
		.key = NULL,
		.val = NULL,
		.i = s.slots
	};
}
void rhtable_next(struct rhtable const * t, struct rhiter * iter)
{
	*iter = rhtable_next_(t, iter->i + 1);
}

struct rhiter rhtable_begin(struct rhtable const * t)
{
	return rhtable_next_(t, 0);
}

struct rhtable * rhtable_resize(struct rhtable * t, uint32_t slots)
{
	struct rhspec s = t->spec;
	if(slots == 0) {
		return NULL;
	} else if(slots < t->count) {
		return NULL;
	} else if(slots == s.slots) {
		return t;
	}
	s.slots = slots;
	struct rhtable * new = rhtable_create_(s);
	if(new == NULL) {
		return NULL;
	}
	rh_for(t, iter) {
		int good = rhtable_set(new, iter.key, iter.val);
		assert(good);
	}
	rhtable_destroy(t);
	return new;
}
