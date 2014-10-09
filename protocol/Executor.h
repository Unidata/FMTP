/**
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: Executor.h
 * @author: Steven R. Emmerson
 *
 * This file declares an executor of independent tasks that allows the caller to
 * obtain the tasks in the order in which they complete.
 */

#ifndef EXECUTOR_H_
#define EXECUTOR_H_

#include "Task.h"

#include <pthread.h>
#include <list>
#include <set>

class Executor {
public:
             Executor() :
                active(Job::compare),
                completed() {}
            ~Executor();
    /**
     * Submits a task for execution on an independent thread.
     *
     * @param[in] task                Task to execute on independent thread.
     * @throws    std::runtime_error  If task was already submitted.
     * @throws    std::runtime_error  If new thread couldn't be created.
     */
    void     submit(Task& task);
    Task&    wait();    // blocks
    void     stopAll(); // blocks

private:
    class Job {
    public:
                    Job(Executor& executor, Task& task) :
                           executor(executor),
                           task(task),
                           thread(0) {};
       static void* start(void* job);           // called by `pthread_create()`
       void         stop() {task.stop();};      // doesn't block
       static bool  compare(Job* job1, Job* job2)
               {return &job1->task < &job2->task;}

       Executor& executor;
       Task&     task;
       pthread_t thread;
    };

    class Lock {
    public:
         Lock(pthread_mutex_t& mutex);
        ~Lock();
    private:
        pthread_mutex_t& mutex;
    };

    /**
     * Adds a job to the set of active jobs.
     *
     * @param[in] job                 Job to add.
     * @throws    std::runtime_error  If a system error occurs.
     */
    void     addToActive(Job* job);
    void     removeFromActive(Job* job);
    void     moveToCompleted(Job* job);

    pthread_mutex_t                   mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t                    cond = PTHREAD_COND_INITIALIZER;
    std::set<Job*,bool(*)(Job*,Job*)> active;
    std::list<Job*>                   completed;
};

#endif /* EXECUTOR_H_ */
