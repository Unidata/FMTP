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

using namespace std;

Executor::~Executor() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    active.set<Job*,bool(*)(Job*,Job*)>::~set();
    completed.list<Job*>::~list();
}

void Executor::submit(
        Task&           task)
{
    Job* job = new Job(*this, task);

    try {
        job->executor.addToActive(job);

        try {
            if (int status = pthread_create(&job->thread, task.getAttributes(),
                    Job::start, job))
                throw runtime_error(string("Couldn't create new thread: ") +
                        strerror(status));
        }
        catch (exception& e) {
            job->executor.removeFromActive(job);
            throw;
        }
    }
    catch (exception& e) {
        delete job;
        throw;
    }
}

void* Executor::Job::start(
        void* arg)
{
    Job* job = (Job*)arg;
    void* result = job->task.start();
    job->task.setResult(result);
    job->executor.moveToCompleted(job);
    return result;
}

Executor::Lock::Lock(pthread_mutex_t& mutex)
    : mutex(mutex)
{
    if (int status = pthread_mutex_lock(&mutex))
        throw runtime_error(string("Couldn't lock mutex: ") + strerror(status));
}

Executor::Lock::~Lock()
{
    if (int status = pthread_mutex_unlock(&mutex))
        throw runtime_error(string("Couldn't unlock mutex: ") +
                strerror(status));
}

void Executor::addToActive(
        Job* job)
{
    Lock                                              lock(mutex);
    pair<set<Job*,bool(*)(Job*,Job*)>::iterator,bool> pair = active.insert(job);

    if (!pair.second)
        throw runtime_error(string("Task already in active set"));
}

void Executor::removeFromActive(
        Job* job)
{
    Lock lock(mutex);
    (void)active.erase(job);    // don't care if it's not there
}

Task& Executor::wait()
{
    Lock lock(mutex);
    while (completed.size() == 0)
        (void)pthread_cond_wait(&cond, &mutex);
    Job* job = completed.front();
    completed.pop_front();
    void* ptr;
    (void)pthread_join(job->thread, &ptr);
    Task& task = job->task;
    delete job;
    return task;
}

void Executor::moveToCompleted(
        Job* job)
{
    Lock lock(mutex);
    (void)active.erase(job);    // don't care if it's not there
    completed.push_back(job);
    (void)pthread_cond_signal(&cond);
}

void Executor::stopAll()
{
    // TODO
}
