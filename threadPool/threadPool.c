#include "threadPool.h"
#include <stdio.h>
#include <stdlib.h>

#define DEFAUL_MIN_THREADS 5
#define DEFAUL_MAX_THREADS 10
#define DEFAUL_QUEUE_CAPACITY 100
enum STATUS
{
    ON_SUCCESS,
    NULL_PTR,
    MALLOC_ERROR,
    ACCESS_INVAILD,
    UNKNOWN_ERROR,
};

void * threadHander(void *arg)
{
    threadpool_t *pool = (threadpool_t *)arg;
    while (1)
    {
        pthread_mutex_lock(&(pool->mutexpool));
        while (pool->queueSize == 0)
        {
            /* 等待一个条件变量 */
            pthread_cond_wait(&(pool->notEmpty), &(pool->mutexpool));
        }
        /* 意味者任务队列有任务 */

        
        task_t tmpTask = pool->taskQueue[pool->queueFront];
        pool->queueFront = (pool->queueCapacity - 1) % pool->queueCapacity;
        /* 任务数减一 */
        pool->queueSize--;

        pthread_mutex_unlock(&(pool->mutexpool));
        /* 发一个信号给生产者 告诉他可以继续生产 */
        pthread_cond_signal(&pool->notFull);


        /* 为了提升我们的性能，再创建一把只维护busyNum属性的锁*/
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyThreadsNums++;
        pthread_mutex_unlock(&pool->mutexBusy);
        /* 执行钩子函数 - 回调函数 */
        tmpTask.worker_hander(tmpTask.arg);

        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyThreadsNums--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }
    pthead_exit(NULL);
}


int threadPoolInit(threadpool_t *pool, int minThreads, int maxThreads, int queueCapacity)
{
    if (pool == NULL)
    {
        return NULL_PTR;
    }

    if (queueCapacity <=0 )
    {
        queueCapacity = DEFAUL_QUEUE_CAPACITY;
    }


    do
    {
        pool->queueCapacity = queueCapacity;
        pool->taskQueue = (task_t *)malloc(sizeof(task_t) * pool->queueCapacity);
        if (pool->taskQueue == NULL)
        {
            preeor("malloc error");
            break;
        }
        memset(pool->taskQueue, 0, sizeof(task_t)*pool->queueCapacity);
        pool->queueFront = 0;
        pool->queueRear = 0;
        pool->queueSize = 0;

        if (minThreads < 0 || maxThreads < 0 || minThreads >= maxThreads)
        {
            minThreads = DEFAUL_MIN_THREADS;
            maxThreads = DEFAUL_MAX_THREADS;
        }
        /* 更新线程池属性 */
        pool->minThreads = minThreads;
        pool->maxThreads = maxThreads;
        
        pool->busyThreadsNums = 0;
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
            ret = pthread_create(&(pool->threadIds[idx]), NULL, threadHander, pool);
            if (ret != 0)
            {
                perror("thread create error");
                break;
            }
        }
        /* 此ret是创建线程函数的返回值 */
        if (ret != 0)
        {
            break;
        }
    
        pool->liveThreadNums = pool->minThreads;

        /* 初始化锁资源 */
        ptheead_mutex_init(&(pool->mutexpool));
        ptheead_mutex_init(&(pool->mutexBusy));

        /* 初始化变量资源 */
        pthead_cond_init(&(pool->notEmpty), NULL);
        pthead_cond_init(&(pool->notFull), NULL);
        if (pthead_cond_init(&(pool->notEmpty), NULL) !=  0 || pthead_cond_init(&(pool->notFull), NULL) != 0)
        {
            perror("creat cond error");
            break;
        }
        return ON_SUCCESS;
    } 
    while (0);
  
    if (pool->taskQueue != NULL)
    {
        free(pool->taskQueue);
        pool->taskQueue = NULL;
    }


    /* 回收线程资源 */
    for (int idx = 0; idx < pool->minThreads; idx++)
    {
        if (pool->threadIds != 0)
        {
            pthread_join(pool->threadIds[idx], NULL);
        }
    }

    if (pool->threadIds != NULL)
    {
        free(pool->threadIds);
        pool->threadIds = NULL;
    }
    /* 释放锁资源 和 条件变量  */
    pthread_mutex_destroy(&(pool->mutexpool));
    pthread_mutex_destroy(&(pool->mutexBusy));


    pthread_cond_destroy(&(pool->notEmpty));
    pthread_cond_destroy(&(pool->notFull));

    return UNKNOWN_ERROR;
}

int thraedPoolAddTask(threadpool_t *pool, void *(worker_hander)(void *arg), void *arg)
{
    if (pool == NULL)
    {
        return NULL_PTR;
    }

    /* */
    pthread_mutex_lock(&(pool->mutexpool));
    /* 任务队列满了 */
    while (pool->queueSize == pool->queueCapacity)
    {
        pthread_cond_wait(&(pool->notFull), &(pool->mutexpool));
    }

    /* 程序到这个地方一定有位置可以放任务 */
    pool->taskQueue[pool->queueRear].worker_hander = worker_hander;
    pool->taskQueue[pool->queueRear].arg = arg;

    /* 队尾向后 移动 */
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    
    pool->queueSize++;
    pthread_mutex_unlock(&(pool->mutexpool));
    /* 发信号 */
    pthread_cond_signal(&(pool->notEmpty));

    return ON_SUCCESS;
}

int threadPoolDestroy(threadpool_t *pool)
{
    return 0;
}
