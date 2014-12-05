// Copyright (c) 2014, ultramailman
// This file is licensed under the MIT License.

#ifndef RHTABLE_H
#define RHTABLE_H
#include <stdint.h>
/* Generic hash table.*/
struct rhtable;

/* this callback should return non zero for equality*/
typedef int (*rh_eq)(void const *, void const *);

/* this is a hash function callback*/
typedef uint32_t (*rh_hash)(void const *);

/* too many parameters for table creation, so pass this structure instead*/
struct rhspec{
	uint32_t slots, keySize, valSize;
	rh_eq eqf;
	rh_hash hashf;
};
/* create a new hash table
returns null pointer if it fails.
*/
struct rhtable * rhtable_create(struct rhspec spec);
#define RHTABLE_CREATE(...)\
	rhtable_create((struct rhspec){__VA_ARGS__})

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
struct rhiter rhtable_next(struct rhtable const * t, struct rhiter const * iter);

/* convenient for loop macro to iterate*/
#define rh_for(ptr, iter)\
	for(struct rhiter iter = rhtable_begin(ptr); iter.key; iter = rhtable_next((ptr), &iter))
#endif
