/**
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: Task.h
 * @author: Steven R. Emmerson
 *
 * This file declares a task that will be executed on an independent thread.
 */

#ifndef TASK_H_
#define TASK_H_

#include <pthread.h>
#include <signal.h>

class Task {
public:
                    Task(pthread_attr_t* attr = 0) :
                        attr(attr),
                        result(0),
                        stopped(false) {};
    virtual        ~Task() = 0;                 // makes `Task` an ABC
    virtual void*   start() = 0;                // subclasses must implement
    pthread_attr_t* getAttributes()             {return attr;};
    void            stop();                     // blocks
    bool            wasStopped()                {return stopped;}
    void            setResult(void* result)     {this->result = result;};
    void*           getResult()                 {return result;};

protected:
    virtual void    cancel() {}                 // blocks

private:
    pthread_attr_t*       attr;
    void*                 result;
    volatile sig_atomic_t stopped;
};

#endif /* TASK_H_ */
