// Copyright (c) 2014, ultramailman
// This file is licensed under the MIT License.

#ifndef RHTABLE_H
#define RHTABLE_H
#include <stdint.h>
#include <stddef.h>

/* this callback should return non zero for equality*/
typedef int (*rh_eq)(void const *, void const *);

/* this is a hash function callback*/
typedef uint32_t (*rh_hash)(void const *);

/* Generic hash table.*/
struct rhtable;

/* A "slot" is a piece of data containing a key and a value.
layout of a slot:
Since Key and Val aren't known at compile time,
pointer arithmetic and memcpy are needed
*/
struct slot_header{
	uint32_t dib;
	uint32_t hash;
};
#define SLOT(keyType, valType)\
	struct{\
		struct slot_header header;\
		keyType key;\
		valType val;\
	}

/* rhspec specifies the layout of the hash table and its slots.
There are too many integers parameters, so using a struct makes
it easier to pass in parameters.*/
struct rhspec{
	uint32_t slots;
	uint32_t slotSize;
	uint32_t keySize, keyOffset;
	uint32_t valSize, valOffset;
	rh_eq eqf;
	rh_hash hashf;
};
/* macro to initialize rhspec*/
#define RHSPEC_CREATE(keyType, valType, ...)\
	(struct rhspec){\
		.keySize = sizeof(keyType),\
		.keyOffset = offsetof(SLOT(keyType, valType), key),\
		.valSize = sizeof(valType),\
		.valOffset = offsetof(SLOT(keyType, valType), val),\
		.slotSize = sizeof(SLOT(keyType, valType)),\
		__VA_ARGS__\
		}

/* create a new hash table
returns null pointer if it fails.
Using the macro to create a new rhtable would be the best.
*/
struct rhtable * rhtable_create_(struct rhspec spec);
#define RHTABLE_CREATE(keyType, valType, ...)\
	rhtable_create_(RHSPEC_CREATE(keyType, valType, __VA_ARGS__))

void rhtable_destroy(struct rhtable * t);

/* get value
returns true if found, false if not found.
writes key and val into rkey and rval.
*/
int rhtable_get(struct rhtable const * t, void const * key, void * rkey, void * rval);

/* insert new pair or replace a value
returns true if successful, false if table is full
*/
int rhtable_set(struct rhtable * t, void const * key, void const * val);

/* deletes a pair if key is found*/
void rhtable_del(struct rhtable * t, void const * key);

/* get current number of pairs*/
uint32_t rhtable_count(struct rhtable const * t);

/* get total number of pairs the table can store*/
uint32_t rhtable_slots(struct rhtable const * t);

/* get the average distance to inital bucket*/
float rhtable_average_dib(struct rhtable const * t);

/* hash table iterator
in every iteration, key and val are set to the current values.
*/
struct rhiter{
	uint32_t i;
	void const * key;
	void * val;
};

/* make a new iterator*/
struct rhiter rhtable_begin(struct rhtable const * t);

/* get the next item*/
void rhtable_next(struct rhtable const * t, struct rhiter * iter);

/* convenient for loop macro to iterate*/
#define rh_for(ptr, iter)\
	for(struct rhiter iter = rhtable_begin(ptr); iter.key; rhtable_next((ptr), &iter))

/* resizes the table if possible.
returns a null if it fails.
returns a pointer to a table if succesful.
The previous pointer may become invalid if resize is successful.
*/
struct rhtable * rhtable_resize(struct rhtable * t, uint32_t slots);
#endif
