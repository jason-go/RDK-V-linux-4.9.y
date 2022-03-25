#include "kernelcalls.h"
#include "hash.h"

struct mem_point {
	unsigned char *p;
	int size;
	int line;
	const char *file;
};

struct file_record {
	int size;
	int alloc_count;
	int free_count;
	int line;
	const char *file;
};

struct mem_node {
	union {
		struct mem_point  alloc;
		struct file_record record;
		struct mem_node *next;
	};
};

struct memory_debug {
	gx_mutex_t lock;
	int node_alloc_size;
	GxHash *alloc_hash;
	GxHash *file_hash;
	struct mem_node head;
};

#define MAGIC_NUM 0x50505050
void gxav_memory_show_debug(int nofree_only);
void gxav_memory_check_debug(void);

static struct memory_debug *memory_init(int size)
{
	struct memory_debug *debug;
	debug = (struct memory_debug*)_malloc(sizeof(struct memory_debug));

	debug->head.next = NULL;
	debug->node_alloc_size = size;
	debug->alloc_hash = GxHashNew(size >> 2, NULL);
	debug->file_hash = GxHashNew(size >> 4, NULL);
	gx_mutex_init(&debug->lock);

	return debug;
}

static struct mem_node *memory_increase(struct memory_debug *debug)
{
	int i;
	struct mem_node *nodes = (struct mem_node*)_malloc(debug->node_alloc_size * sizeof(struct mem_node));

	for (i=0; i < debug->node_alloc_size; i++) {
		nodes[i].next = debug->head.next;
		debug->head.next = &nodes[i];
	}

	return debug->head.next;
}

static struct mem_node *memory_alloc(struct memory_debug *debug)
{
	struct mem_node *node = debug->head.next;

	if (node == NULL)
		node = memory_increase(debug);

	if (node != NULL) {
		debug->head.next = debug->head.next->next;
		memset(node, 0, sizeof(struct mem_node));
	}
	gxav_memory_check_debug();

	return node;
}

static struct mem_point *memory_alloc_find(struct memory_debug *debug, void *p)
{
	char key[10];

	gx_snprintf(key, 10, "%p", p);
	return GxHashGet(debug->alloc_hash, key);
}

static int memory_free(struct memory_debug *debug, void *p)
{
	char key[10];

	gx_snprintf(key, 10, "%p", p);
	return GxHashDrop(debug->alloc_hash, key);
}

static struct file_record *memory_update_file_record(struct memory_debug *debug, struct mem_point *node)
{
	char key[256];
	struct file_record *record;

	gx_snprintf(key, 255, "%s:%d:%d", node->file, node->line, node->size);
	record = GxHashGet(debug->file_hash, key);

	if (record == NULL) {
		record = (struct file_record*)memory_alloc(debug);
		record->file = node->file;
		record->line = node->line;
		record->size = node->size;

		GxHashAdd(debug->file_hash, key, record);
	}

	return record;
}

static void memory_add(struct memory_debug *debug, struct mem_point *p)
{
	struct mem_point *node;

	gx_mutex_lock(&debug->lock);

	node = (struct mem_point*)memory_alloc(debug);
	if (node) {
		struct file_record *record;
		char key[10];

		gx_snprintf(key, 10, "%p", p->p);
		memcpy(node, p, sizeof(struct mem_point));
		GxHashAdd(debug->alloc_hash, key, node);

		record = memory_update_file_record(debug, node);

		if (record)
			record->alloc_count++;
	}
	gx_mutex_unlock(&debug->lock);
}

static void memory_del(struct memory_debug *debug, void *p, const char *file, int line)
{
	struct mem_point *node;

	gx_mutex_lock(&debug->lock);

	node = memory_alloc_find(debug, p);
	if (node) {
		struct file_record *record;
		int magic = 0;
#if 1
		magic = (magic << 8) + node->p[node->size + 0];
		magic = (magic << 8) + node->p[node->size + 1];
		magic = (magic << 8) + node->p[node->size + 2];
		magic = (magic << 8) + node->p[node->size + 3];
#endif

		if (magic != MAGIC_NUM)
			gx_printf("memory out of bounds %p, size: %d [%s:%d]: %x\nn", node->p, node->size, node->file, node->line, magic);

		record = memory_update_file_record(debug, node);
		if (record)
			record->free_count++;

		((struct mem_node*)node)->next = debug->head.next;
		debug->head.next = (struct mem_node*)node;

		memory_free(debug, p);
	}
	else
		gx_printf("                         [%s +%d] %p\n", file, line, p);
	gx_mutex_unlock(&debug->lock);
}

#define S 32
static struct memory_debug *_debug = NULL;

void gxav_MemoryAdd(const char *file, int line, char *p, int size)
{
	if (_debug == NULL)
		_debug = memory_init(S);

	if (_debug) {
		struct mem_point mp;

		p[size + 0] = 0x50;
		p[size + 1] = 0x50;
		p[size + 2] = 0x50;
		p[size + 3] = 0x50;

		mp.p = (unsigned char*)p;
		mp.size = size;
		mp.file = file;
		mp.line = line;

		memory_add(_debug, &mp);
	}
}

void gxav_MemoryDel(void *p, const char *file, int line)
{
	if (_debug != NULL && p != NULL)
		memory_del(_debug, p, file, line);
}

#ifndef INT_MAX
#    define INT_MAX 2147483647
#endif

void gxav_memory_show_debug(int nofree_only)
{
	struct file_record *record = NULL;
	int max_size = 0, min_size= INT_MAX;
	GxHashIterator *iter;
	int record_count = 0;

	if (_debug == NULL)
		return;

	iter = GxHashIterNew(_debug->file_hash);

	while (GxHashIterNext(iter) != NULL) {
		record = (struct file_record*)GxHashIteratorGetData(iter);
		if (record)
			record_count++;
	}
	GxHashIterRelease(iter);

	if (record_count == 0)
		return;

	iter = GxHashIterNew(_debug->file_hash);

	while (GxHashIterNext(iter) != NULL) {
		record = (struct file_record*)GxHashIteratorGetData(iter);

		if (record) {
#if 1
			if (nofree_only == 1 || record->alloc_count - record->free_count > 0)
				gx_printf("Size: %8d byte, Alloc Count: %8d, Free Count: %8d [%s:%d]\n",
						record->size, record->alloc_count, record->free_count,
						record->file, record->line);
#endif

			if (record->size > max_size)
				max_size = record->size;

			if (record->size < min_size)
				min_size = record->size;
		}
	}
	GxHashIterRelease(iter);

	gx_printf("\nMax alloc size: %dbyte\nMin alloc size: %dbyte\n\n", max_size, min_size);

	gxav_memory_check_debug();
}

void gxav_memory_check_debug(void)
{
	struct mem_point *node;
	GxHashIterator *iter;

	if (_debug == NULL)
		return;

	iter = GxHashIterNew(_debug->alloc_hash);

	while (GxHashIterNext(iter) != NULL) {
		node = (struct mem_point*)GxHashIteratorGetData(iter);

		if (node != NULL) {
			int magic = 0;
#if 1
			magic = (magic << 8) + node->p[node->size + 0];
			magic = (magic << 8) + node->p[node->size + 1];
			magic = (magic << 8) + node->p[node->size + 2];
			magic = (magic << 8) + node->p[node->size + 3];
#endif

			if (magic != MAGIC_NUM)
				gx_printf("memory out of bounds %p, size: %d [%s +%d]:%x\n",
						node->p, node->size, node->file, node->line, magic);
		}
	}
	GxHashIterRelease(iter);
}

void *gxav_malloc_debug(const char *file, int line, size_t size)
{
	char *p = _malloc(size + sizeof(int));
	if(p == NULL) {
		gx_printf("~~~malloc null~~~%s %d~~~~\n", file, line);
		return NULL;
	}

	gxav_MemoryAdd(file, line, p, size);

	return p;
}

void *gxav_mallocz_debug(const char *file, int line, size_t size)
{
	void *p = gxav_malloc_debug(file, line, size);
	if (p)
		memset(p, 0, size);

	return p;
}

void gxav_free_debug(const char *file, int line, void *ptr)
{
	gxav_MemoryDel(ptr, file, line);
	if (ptr)
		_free(ptr);
}

void *gxav_malloc_release(size_t size)
{
	return _malloc(size);
}

void *gxav_mallocz_release(size_t size)
{
	void *p = _malloc(size);
	if (p)
		memset(p, 0, size);

	return p;
}

void gxav_free_release(void *ptr)
{
	if (ptr)
		_free(ptr);
}

EXPORT_SYMBOL(gxav_malloc_release);
EXPORT_SYMBOL(gxav_mallocz_release);
EXPORT_SYMBOL(gxav_free_release);

