// Copyright (c) 2014, ultramailman
// This file is licensed under the MIT License.

#ifndef RHTABLE_SAFE_H
#define RHTABLE_SAFE_H

/* This file has convenience macros if you want more type safety.
It wraps all operations in type safe functions.
Hopefully your compiler can inline them so code size won't increase.
*/
#include "rhtable.h"

#define RHTABLE_DEFINE_SAFE(name, Key, Val, eqFunc, hashFunc)\
typedef struct{\
	struct rhtable * ptr;\
} name ;\
static int name##_eq(void const * a, void const * b)\
{\
	Key const * A = a;\
	Key const * B = b;\
	return eqFunc(A, B);\
}\
static uint32_t name##_hash(void const * a)\
{\
	Key const * A = a;\
	return hashFunc(A);\
}\
static inline name name##_create(uint32_t slots)\
{\
	struct rhtable * ptr = RHTABLE_CREATE(Key, Val,\
		.slots = slots,\
		.hashf = name##_hash,\
		.eqf = name##_eq\
	);\
	name t = {ptr};\
	return t;\
}\
static inline int name##_set(name * t, Key const * key, Val const * val)\
{\
	int good = rhtable_set(t->ptr, key, val);\
	if(!good  ) {\
		struct rhtable * new = rhtable_resize(t->ptr, rhtable_slots(t->ptr) * 2);\
		if(new != NULL) {\
			t->ptr = new;\
			return rhtable_set(t->ptr, key, val);\
		} else {\
			return 0;\
		}\
	} else {\
		return 1;\
	}\
}\
static inline int name##_get(name * t, Key const * key, Key * rkey, Val * rval)\
{	return rhtable_get(t->ptr, key, rkey, rval); }\
static inline void name##_del(name * t, Key const * key)\
{	rhtable_del(t->ptr, key); }\
static inline uint32_t name##_count(name * t)\
{	return rhtable_count(t->ptr); }\
static inline uint32_t name##_slots(name * t)\
{	return rhtable_slots(t->ptr); }\
static inline float name##_average_dib(name * t)\
{	return rhtable_average_dib(t->ptr); }\
typedef struct{\
	Key const * key;\
	Val * val;\
	struct rhiter unsafe;\
} name##iter;\
static inline name##iter name##_begin(name * t)\
{\
	struct rhiter begin = rhtable_begin(t->ptr);\
	name##iter iter = {\
		.key = begin.key,\
		.val = begin.val,\
		.unsafe = begin\
	};\
	return iter;\
}\
static inline void name##_next(name * t, name##iter * iter)\
{\
	rhtable_next(t->ptr, &iter->unsafe);\
	iter->key = iter->unsafe.key;\
	iter->val = iter->unsafe.val;\
}

#define rh_for_safe(name, table, iterator)\
for(name##iter iterator = name##_begin(table);\
	iterator.key;\
	name##_next(table, &iterator))

#endif
