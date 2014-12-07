rhtable
=======

rhtable is an implementation of open addressing hash table.

It uses linear probing and Robin-hood heuristic to decide the order of
inserted items.

It uses void* to achieve generic-ness, so make sure you are inserting
into the table with the types you initiated the table with.

Alternatively, you can use the typesafe macros in rhtable_safe.h header
to generate typesafe versions of the table. It's like c++ templates in c.

The code uses c99 features.

Operations:
create(Key, Val, slots, hash, eq)
	initialize a table that uses Key and Val types.
	slots is the maximum number of mappings this table can hold.
	hash and eq are callback functions pointers.

get(key, rkey, rval)
	find the value indexed by the key,
	and write both the key and value into rkey and rvalue

set(key, value)
	writes the value to the slot indexed by the key.

del(key)
	deletes the value at the slot indexed by the key.
