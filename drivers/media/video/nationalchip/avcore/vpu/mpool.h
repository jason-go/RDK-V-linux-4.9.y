#ifndef __min_pool_h__
#define __min_pool_h__

/**
 * @brief create a memory pool
 * @param [in] blk_size  size of memory block
 *        [in] blk_num   number of blocks
 * @return a pointer to the pool created
 *         on error return NULL
 * @remark
 */
void* mpool_create(unsigned blk_size, unsigned blk_num);

/**
 * @brief deatroy a memory pool
 * @param [in] pool  pointer to the pool to destroy
 * @return retun no value
 * @remark
 */
void  mpool_destroy(void *pool);

/**
 * @brief alloc a memory block
 * @param [in] pool     the pool for alloc
 *        [in] blk_size the size of block to alloc
 * @return a pointer to the block alloced
 *         on error return NULL
 * @remark
 */
void* mpool_alloc(void *pool, unsigned blk_size);

/**
 * @brief free a block
 * @param [in] pool the pool for alloc from
 *        [in] p    the pointer of block
 * @return return no value
 * @remark
 */
void  mpool_free(void *pool, void *p);

/**
 * @brief print the state of pool
 * @param [in] the pool to print
 * @return return no value
 * @remark pool debug
 */
void mpool_print(void *pool);

#endif

