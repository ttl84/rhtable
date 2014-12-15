// Copyright (c) 2014, ultramailman
// This file is licensed under the MIT License.

#include <assert.h>
#include <stdio.h>
#include <time.h>

#include "rhtable.h"
#include "rhtable_safe.h"




typedef struct{
	uint32_t n;
} Key;
typedef struct{
	uint64_t index;
} Val;

int KeyEq(Key const * a, Key const * b)
{
	return a->n == b->n;
}
uint32_t KeyHash(Key const * a)
{
	return a->n * 65537;
}
RHTABLE_DEFINE_SAFE(hashtable, Key, Val, KeyEq, KeyHash)

#define LENGTH 30000

// random data
static inline unsigned random(unsigned x)
{
	return (x * 16807) % ((2 << 31) - 1);
}
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
void testRemoval(hashtable * t, unsigned begin, unsigned end)
{
	for(unsigned i = begin; i < end; i++) {
		int found = hashtable_get(t, data + i, 0, 0);
		if(membership[i]) {
			assert(found);
			hashtable_del(t, data + i);
			
			found = hashtable_get(t, data + i, 0, 0);
			assert(!found);
			membership[i] = 0;
		}
	}
}
static 
void testFind(hashtable * t)
{
	for(int i = 0; i < LENGTH; i++)
	{
		Key rkey;
		Val rval;
		int found = hashtable_get(t, data + i, &rkey, &rval);
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
void testInsert(hashtable * t, unsigned begin, unsigned end)
{
	for(int i = 0; i < LENGTH; i++)
	{
		Key key = data[i];
		Val val = {i};
		int good = hashtable_set(t, &key, &val);
		if(good)
		{
			Key rkey;
			Val rval;
			int found = hashtable_get(t, data + i, &rkey, &rval);
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
	printf("items in: %d\n", hashtable_count(t));
	printf("total number of items: %d\n", LENGTH);
	printf("table capacity: %d\n", hashtable_slots(t));

	float count = hashtable_count(t);
	float slots = hashtable_slots(t);
	float load = count / slots;
	float averageDib = hashtable_average_dib(t);
	fprintf(stdout, "load: %f\n", load);
	fprintf(stdout, "average dib: %f\n\n", averageDib);
}
static 
void testIterator(hashtable * t)
{
	rh_for_safe(hashtable, t, iter) {
		Key rkey;
		Val rval;
		assert(hashtable_get(t, iter.key, &rkey, &rval));
		assert(rkey.n == data[rval.index].n);
		assert(membership[rval.index]);
	}
}
int main(int argc, char ** argv)
{
	init();
	unsigned size = 45001;
	if(argc == 2) {
		sscanf(argv[1], "%ud", &size);
	}
	hashtable t = hashtable_create(size);
	
	clock_t t1 = clock();
	testInsert(&t, 0, LENGTH);
	testFind(&t);
	testIterator(&t);
	testRemoval(&t, 0, LENGTH);
	clock_t t2 = clock();
	
	printf("time: %lu\n", t2 - t1);
}

