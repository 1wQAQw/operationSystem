#include "threadPool.h"
#include <stdio.h>
#include <stdlib.h>

#define DEFAUL_MIN_THREADS 5
#define DEFAUL_MAX_THREADS 10

enum STATUS
{
    ON_SUCCESS,
    NULL_PTR,
    MALLOC_ERROR,
    ACCESS_INVAILD,
    UNKOWNN_ERROR,
};

void * threadHander(void *arg)
{
    pthead_exit(NULL);
}


int threadPoolInit(threadpool_t *pool, int minThreads, int maxThreads)
{
    if (pool == NULL)
    {
        return NULL_PTR;
    }

    do
    {
        if (minThreads < 0 || maxThreads < 0 || minThreads >= maxThreads)
        {
            minThreads = DEFAUL_MIN_THREADS;
            maxThreads = DEFAUL_MAX_THREADS;
        }
        /* 更新线程池属性 */
        pool->minThreads = minThreads;
        pool->maxThreads = maxThreads;

        pool->threadIds = (pthread_t *)malloc(sizeof(pthread_t) * maxThreads);
        if (pool->threadIds == NULL)
        {
            perror("malloc error");
            exit(-1);
        }
        /* 清除脏数据 */
        memset(pool->threadIds, 0 , sizeof(pthread_t) * pool->maxThreads);

        int ret = 0;
        /* 创建线程 */
        for (int idx = 0; idx < pool->minThreads; idx++)
        {
            /* 如果线程Id号为0 这个位置可以用 */
            ret = pthread_create(&(pool->threadIds[idx]), NULL, threadHander, NULL);
            if (ret != 0)
            {
                perror("thread create error");
                break;
            }
        }
    return ON_SUCCESS;
    } 
    while (0);
  

    /* 回收线程资源 */
    for (int idx = 0; idx < pool->minThreads; idx++)
    {
        if (pool->threadIds != 0)
        {
            pthread_join(pool->threadIds[idx]);
        }
    }

    if (pool->threadIds != NULL)
    {
        free(pool->threadIds);
        pool->threadIds = NULL;
    }
    return UNKOWNN_ERROR;
}

int threadPoolDestroy(threadpool_t *pool)
{
    return 0;
}
