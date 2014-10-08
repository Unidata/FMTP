/**
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: Executor.cpp
 * @author: Steven R. Emmerson
 *
 * This file defines an executor of independent tasks that allows the caller to
 * obtain the tasks in the order in which they complete.
 */

#include "Executor.h"

#include <stdexcept>
#include <string>
#include <string.h>

Executor::~Executor() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    active.std::set<Job*,bool(*)(Job*,Job*)>::~set();
    completed.std::list<Job>::~list();
}

void Executor::submit(
        Task&           task,
        void* const     arg,
        pthread_attr_t* const attr)
{
    Job* job = new Job(*this, task, arg);

    if (int status = pthread_create(&job->thread, task.getAttributes(),
            Job::start, job))
        throw std::runtime_error(std::string("Couldn't create new thread: ") +
                strerror(status));

    addToActive(*job);
}

void* Executor::Job::start(
        void* arg)
{
    Job* job = (Job*)arg;
    void* result = job->task.getStartRoutine()(job->arg);
    job->task.setResult(result);
    return result;
}

Executor::Lock::Lock(pthread_mutex_t& mutex)
    : mutex(mutex)
{
    if (int status = pthread_mutex_lock(&mutex))
        throw std::runtime_error(std::string("Couldn't lock mutex: ") +
                strerror(status));
}

Executor::Lock::~Lock()
{
    if (int status = pthread_mutex_unlock(&mutex))
        throw std::runtime_error(std::string("Couldn't unlock mutex: ") +
                strerror(status));
}

void Executor::addToActive(
        Job& job)
{
    Lock lock(mutex);
    active.insert(&job);
}

Task& Executor::wait()
{
    Lock lock(mutex);
    while (completed.size() == 0)
        (void)pthread_cond_wait(&cond, &mutex);
    Job& job = completed.front();
    completed.pop_front();
    return job.task;
}

void Executor::moveToCompleted(
        Job& job)
{
    Lock lock(mutex);
    active.erase(&job);
    completed.push_back(job);
}

void Executor::stopAll()
{
    // TODO
}
