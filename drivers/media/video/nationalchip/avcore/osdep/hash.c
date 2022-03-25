#include "kernelcalls.h"
#include "hash.h"

#ifndef TEE_OS
extern int strcmp(const char *s1, const char *s2);
extern char *strcpy(char *dest, const char *src);
extern size_t strlen(const char *s);
#endif

/* private types */
typedef struct _hashentry_t hashentry_t;

struct _hashentry_t {
	char *key;
	void *value;
	hashentry_t *next;
};

struct _GxHash {
	unsigned int ref;
	GxHashFreeFunc freefunc;
	int length;
	int num_keys;
	hashentry_t **entries;
};

struct _GxHashIterator {
	unsigned int ref;
	GxHash *table;
	hashentry_t *entry;
	int index;
};

/** allocate and initialize a new hash table */
GxHash *GxHashNew(int size, GxHashFreeFunc freefunc)
{
	GxHash *result = NULL;

	result = (GxHash *) _malloc(sizeof(GxHash));
	if (result != NULL) {
		result->entries = (hashentry_t **) _malloc(size * sizeof(hashentry_t *));
		if (result->entries == NULL) {
			_free(result);
			return NULL;
		}
		memset(result->entries, 0, size * sizeof(hashentry_t *));

		result->length = size;
		result->freefunc = freefunc;
		result->num_keys = 0;
		/* give the caller a reference */
		result->ref = 1;
	}

	return result;
}

/** obtain a new reference to an existing hash table */
GxHash *GxHashClone(GxHash * table)
{
	table->ref++;
	return table;
}

/** release a hash table that is no longer needed */
void GxHashRelease(GxHash * table)
{
	hashentry_t *entry, *next;
	int i;

	if (table->ref > 1)
		table->ref--;
	else {
		for (i = 0; i < table->length; i++) {
			entry = table->entries[i];
			while (entry != NULL) {
				next = entry->next;
				_free(entry->key);
				if (table->freefunc)
					table->freefunc(entry->value);
				_free(entry);
				entry = next;
			}
		}
		_free(table->entries);
		_free(table);
	}
}

/** hash a key for our table lookup */
static int _GxHashKey(GxHash * table, const char *key)
{
	int hash = 0;
	int shift = 0;
	const char *c = key;

	if (table->length == 1)
		return 0;
	while (*c != '\0') {
		/* assume 32 bit ints */
		hash ^= ((int)*c++ << shift);
		shift += 8;
		if (shift > 24)
			shift = 0;
	}

	return hash % table->length;
}

/** add a key, value pair to a hash table.
 *  each key can appear only once; the value of any
 *  identical key will be replaced
 */
int GxHashAdd(GxHash * table, const char *key, void *data)
{
	hashentry_t *entry = NULL;
	int index = _GxHashKey(table, key);

	entry = table->entries[index];
	while (entry != NULL) {
		/* traverse the linked list looking for the key */
		if (strcmp(key, entry->key) == 0) {
			/* match */
			if (table->freefunc && entry->value)
				table->freefunc(entry->value);
			entry->value = data;
			return 0;
		}
		entry = entry->next;
	}

	/* allocate and fill a new entry */
	entry = (hashentry_t *) _malloc(sizeof(hashentry_t));
	if (!entry)
		return -1;

	entry->key = (char *)_malloc(strlen(key) + 1);
	if (!entry->key) {
		_free(entry);
		return -1;
	}
	gx_strcpy(entry->key, key);

	entry->value = data;
	/* insert ourselves in the linked list */
	/* TODO: this leaks duplicate keys */
	entry->next = table->entries[index];
	table->entries[index] = entry;
	table->num_keys++;

	return 0;
}

/** look up a key in a hash table */
void *GxHashGet(GxHash * table, const char *key)
{
	hashentry_t *entry;
	int index = _GxHashKey(table, key);
	void *result = NULL;

	/* look up the hash entry */
	entry = table->entries[index];
	while (entry != NULL) {
		/* traverse the linked list looking for the key */
		if (!strcmp(key, entry->key)) {
			/* match */
			result = entry->value;
			return result;
		}
		entry = entry->next;
	}
	/* no match */
	return result;
}

/** delete a key from a hash table */
int GxHashDrop(GxHash * table, const char *key)
{
	hashentry_t *entry, *prev;
	int index = _GxHashKey(table, key);

	/* look up the hash entry */
	entry = table->entries[index];
	prev = NULL;
	while (entry != NULL) {
		/* traverse the linked list looking for the key */
		if (!strcmp(key, entry->key)) {
			/* match, remove the entry */
			_free(entry->key);
			if (table->freefunc)
				table->freefunc(entry->value);
			if (prev == NULL)
				table->entries[index] = entry->next;
			else
				prev->next = entry->next;
			_free(entry);
			table->num_keys--;
			return 0;
		}
		prev = entry;
		entry = entry->next;
	}
	/* no match */
	return -1;
}

int GxHashNumKeys(GxHash * table)
{
	return table->num_keys;
}

/** allocate and initialize a new iterator */
GxHashIterator *GxHashIterNew(GxHash * table)
{
	GxHashIterator *iter;

	iter = (GxHashIterator *) _malloc(sizeof(*iter));
	if (iter != NULL) {
		iter->ref = 1;
		iter->table = GxHashClone(table);
		iter->entry = NULL;
		iter->index = 0;
	}

	return iter;
}

/** release an iterator that is no longer needed */
void GxHashIterRelease(GxHashIterator * iter)
{
	iter->ref--;

	if (iter->ref <= 0) {
		GxHashRelease(iter->table);
		_free(iter);
	}
}

/** return the next hash table key from the iterator.
    the returned key should not be freed */
const char *GxHashIterNext(GxHashIterator * iter)
{
	GxHash *table = iter->table;
	hashentry_t *entry = iter->entry;

	/* advance until we find the next entry */
	if (entry != NULL)
		entry = entry->next;

	if (entry == NULL) {
		/* we're off the end of list, search for a new entry */
		while (iter->index < iter->table->length) {
			entry = table->entries[iter->index];
			if (entry != NULL) {
				iter->index++;
				break;
			}
			iter->index++;
		}
	}

	if ((entry == NULL) || (iter->index > table->length)) {
		/* no more keys! */
		return NULL;
	}

	/* remember our current match */
	iter->entry = entry;
	return entry->key;
}

void *GxHashIteratorGetData(GxHashIterator *iter)
{
	if (iter == NULL)
		return NULL;
	if (iter->entry == NULL)
		return NULL;
	return iter->entry->value;
}

int GxHashIterSetData(GxHashIterator * iter, void *data)
{
	if (iter == NULL)
		return -1;
	if (iter->entry == NULL)
		return -1;
	iter->entry->value = data;

	return 0;
}
