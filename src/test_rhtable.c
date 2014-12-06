// Copyright (c) 2014, ultramailman
// This file is licensed under the MIT License.

#include "rhtable.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>
#define LENGTH 30000

typedef struct{
	uint32_t n;
} Key;
typedef struct{
	uint64_t index;
} Val;
static inline unsigned random(unsigned x)
{
	return (x * 16807) % ((2 << 31) - 1);
}

static int inteqf(void const * aptr, void const * bptr)
{
	Key const * a = aptr;
	Key const * b = bptr;
	return a->n == b->n;
}


static uint32_t inthashf(void const * ptr)
{
	Key const * a = ptr;
	return a->n * 65537;
}

// random data
Key data [LENGTH];
unsigned membership[LENGTH];
void init(void)
{
	unsigned seed = 10;
	for(int i = 0; i < LENGTH; i++)
	{
		seed = random(seed);
		data[i].n = seed;
		membership[i] = 0;
	}
}
static 
void testRemoval(struct rhtable * t, unsigned begin, unsigned end)
{
	for(unsigned i = begin; i < end; i++) {
		int found = rhtable_get(t, data + i, 0, 0);
		if(membership[i]) {
			assert(found);
			rhtable_del(t, data + i);
			
			found = rhtable_get(t, data + i, 0, 0);
			assert(!found);
			membership[i] = 0;
		}
	}
}
static 
void testFind(struct rhtable * t)
{
	for(int i = 0; i < LENGTH; i++)
	{
		Key rkey;
		Val rval;
		int found = rhtable_get(t, data + i, &rkey, &rval);
		if(membership[i]) {
			assert(found);
			assert(data[rval.index].n == rkey.n);
			assert(data[rval.index].n == data[i].n);
		} else {
			assert(!found);
		}
	}
}

static
void testInsert(struct rhtable * t, unsigned begin, unsigned end)
{
	for(int i = 0; i < LENGTH; i++)
	{
		Key key = data[i];
		Val val = {i};
		int good = rhtable_set(t, &key, &val);
		if(good)
		{
			Key rkey;
			Val rval;
			int found = rhtable_get(t, data + i, &rkey, &rval);
			assert(found);
			assert(rkey.n == key.n);
			assert(rval.index == val.index);
			assert(data[rval.index].n == rkey.n);
			assert(data[rval.index].n == data[i].n);
			membership[i] = 1;
		}
		else
		{
			printf("table full %d\n", i);
		}
	}
	printf("items in: %d\n", rhtable_count(t));
	printf("total number of items: %d\n", LENGTH);
	printf("table capacity: %d\n", rhtable_slots(t));

	float count = rhtable_count(t);
	float slots = rhtable_slots(t);
	float load = count / slots;
	float averageDib = rhtable_average_dib(t);
	fprintf(stdout, "load: %f\n", load);
	fprintf(stdout, "average dib: %f\n\n", averageDib);
}
static 
void testIterator(struct rhtable * t)
{
	rh_for(t, iter) {
		Key rkey;
		Val rval;
		assert(rhtable_get(t, iter.key, &rkey, &rval));
		assert(rkey.n == data[rval.index].n);
		assert(membership[rval.index]);
	}
}
int main(void)
{
	init();
	struct rhtable * t = RHTABLE_CREATE(Key, Val, 
		.slots = LENGTH,
		.hashf = inthashf,
		.eqf = inteqf
	);
	
	clock_t t1 = clock();
	testInsert(t, 0, LENGTH);
	testFind(t);
	testIterator(t);
	testRemoval(t, 0, LENGTH);
	clock_t t2 = clock();
	
	printf("time: %lu\n", t2 - t1);
}
