// Copyright (c) 2014, ultramailman
// This file is licensed under the MIT License.

#include "rhtable.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* A "slot" is a piece of data containing a key and a value.
layout of a slot:
struct slot{
	struct slot_header header;
	Key key;
	Val val;
};
Since Key and Val aren't known at compile time, pointer arithmetic is
required to access key and val.
*/
struct slot_header{
	uint32_t dib;
	uint32_t hash;
};

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
	uint32_t slots;
	uint32_t count;
	uint32_t keySize, valSize;
	rh_hash hashf;
	rh_eq eqf;
};

/* memswap swaps two pieces of non overlapping memory.*/
static inline
void memswap_(char * restrict a, char * restrict b, unsigned bytes)
{
	for(unsigned i = 0; i < bytes; i++) {
		char tmp = a[i];
		a[i] = b[i];
		b[i] = tmp;
	}
}
static
void memswap(void * restrict a, void * restrict b, unsigned bytes)
{
	memswap_(a, b, bytes);
}


static
uint32_t slotSize(struct rhtable const * t)
{
	return sizeof(struct slot_header) + t->keySize + t->valSize;
}


static
char * base(struct rhtable const * t)
{
	return ((char*)t) + sizeof *t;
}

/* tmp is a temporary slot to store evicted pairs */
static
unsigned tmpOffset(struct rhtable const * t)
{
	return 0;
}
static
struct slot_header * tmpGet(struct rhtable * t)
{
	return (struct slot_header *)(base(t) + tmpOffset(t));
}

/* slots are the memory to store the contents of the hash table */
static
unsigned slotOffset(struct rhtable const * t, uint32_t i)
{
	return slotSize(t) + i * slotSize(t);
}
static
struct slot_header * slotGet(struct rhtable const * t, uint32_t i)
{
	return (struct slot_header *)(base(t) + slotOffset(t, i));
}
////////////////////////////////////////////////////////////
/* access the key of a slot */
static
void * slotGetKey(struct slot_header const * slot, struct rhtable const * t)
{
	return ((char *)slot) + sizeof(struct slot_header);
}
static
void slotSetKey(struct slot_header * slot, struct rhtable * t, void const * key)
{
	memcpy(slotGetKey(slot, t), key, t->keySize);
}
/* access the value of a slot*/
static
void * slotGetVal(struct slot_header const * slot, struct rhtable const * t)
{
	return ((char *)slot) + sizeof(struct slot_header) + t->keySize;
}
static
void slotSetVal(struct slot_header * slot, struct rhtable * t, void const * val)
{
	memcpy(slotGetVal(slot, t), val, t->valSize);
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
	return t->slots;
}
float rhtable_average_dib(struct rhtable const * t)
{
	float dibs;
	for(uint32_t i = 0; i < t->slots; i++) {
		struct slot_header * slot = slotGet(t, i);
		if(!slotIsEmpty(slot)) {
			dibs += slot->dib;
		}
	}
	return dibs / t->count;
}
struct rhtable * rhtable_create(struct rhspec const spec)
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
	struct rhtable * t = calloc(1, sizeof *t + (1 + spec.slots) * (sizeof(struct slot_header) + 
		spec.keySize + 
		spec.valSize));
	if(t != NULL) {
		t->slots = spec.slots;
		t->count = 0;
		t->keySize = spec.keySize;
		t->valSize = spec.valSize;
		t->hashf = spec.hashf;
		t->eqf = spec.eqf;
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
	uint32_t const hash = t->hashf(key);
	uint32_t dib = 0;
	uint32_t probe = (hash + dib) % t->slots;
	do{
		struct slot_header * slot = slotGet(t, probe);
		
		if(slotIsEmpty(slot) || dib > slot->dib) {
			return 0;
		} else if(hash == slot->hash &&
		t->eqf(key, slotGetKey(slot, t))) {
			if(rkey != NULL) {
				memcpy(rkey, slotGetKey(slot, t), t->keySize);
			}
			if(t->valSize > 0 && rval != NULL) {
				memcpy(rval, slotGetVal(slot, t), t->valSize);
			}
			return 1;
		} else {
			dib++;
			probe++;
			if(probe == t->slots) {
				probe = 0;
			}
		}
	}while(dib < t->slots);
	return 0;
}
static
int rhtable_set_(struct rhtable * t)
{
	struct slot_header * const tmp = tmpGet(t);
	do{
		uint32_t probe = (tmp->hash + tmp->dib) % t->slots;
		struct slot_header * slot = slotGet(t, probe);
		
		if(slotIsEmpty(slot)) {
			memcpy(slot, tmp, slotSize(t));
			t->count++;
			return 1;
		} else if(tmp->hash == slot->hash && 
		t->eqf(slotGetKey(tmp, t), slotGetKey(slot, t))) {
			slotSetVal(slot, t, slotGetVal(tmp, t));
			return 1;
		} else if(tmp->dib > slot->dib) {
			memswap(tmp, slot, slotSize(t));
			tmp->dib++;
		} else {
			tmp->dib++;
		}
	}while( tmp->dib < t->slots );
	assert(0);
	return 0;
}
int rhtable_set(struct rhtable * t, void const * key, void const * val)
{

	if(t->count == t->slots) {
		return 0;
	}
	
	struct slot_header * const tmp = tmpGet(t);
	
	tmp->dib = 0;
	tmp->hash = t->hashf(key);
	slotSetKey(tmp, t, key);
	slotSetVal(tmp, t, val);
	
	return rhtable_set_(t);
}
static
void rhtable_del_shift(struct rhtable * t, uint32_t probe)
{
	uint32_t next = (probe + 1) % t->slots;
	struct slot_header * thisSlot = slotGet(t, probe);
	struct slot_header * nextSlot = slotGet(t, next);
	while(!slotIsEmpty(nextSlot) && nextSlot->dib != 0) {
		memswap(thisSlot, nextSlot, slotSize(t));
		thisSlot->dib--;
		assert(thisSlot->dib + 1 > 0);
		
		next++;
		if(next == t->slots) {
			next = 0;
		}
		thisSlot = nextSlot;
		nextSlot = slotGet(t, next);
	}
}
void rhtable_del(struct rhtable * t, void const * key)
{
	uint32_t const hash = t->hashf(key);
	uint32_t dib = 0;
	uint32_t probe = (hash + dib) % t->slots;
	do{
		struct slot_header * slot = slotGet(t, probe);
		
		if(slotIsEmpty(slot) || dib > slot->dib) {
			return;
		} else if(hash == slot->hash &&
			t->eqf(key, slotGetKey(slot, t))) {
			slotSetEmpty(slot);
			t->count--;
			rhtable_del_shift(t, probe);
			return;
		} else {
			dib++;
			probe++;
			if(probe == t->slots) {
				probe = 0;
			}
		}
	}while(dib < t->slots);
}

static
struct rhiter rhtable_next_(struct rhtable const * t, uint32_t i)
{
	for(; i < t->slots; i++) {
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
		.i = t->slots
	};
}
struct rhiter rhtable_next(struct rhtable const * t, struct rhiter const * iter)
{
	return rhtable_next_(t, iter->i + 1);
}

struct rhiter rhtable_begin(struct rhtable const * t)
{
	return rhtable_next_(t, 0);
}
