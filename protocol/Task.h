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

class Task {
public:
                    Task(void* (*startRoutine)(void* arg),
                            pthread_attr_t* attr = 0) :
                        startRoutine(startRoutine),
                        attr(attr),
                        result(0) {};
    virtual        ~Task();
    void*         (*getStartRoutine())(void*)      {return startRoutine;};
    pthread_attr_t* getAttributes()                {return attr;};
    virtual void    stop() {}                      // blocks
    void            setResult(void* result)        {this->result = result;};
    void*           getResult()                    {return result;};

private:
    void*         (*startRoutine)(void*);
    pthread_attr_t* attr;
    void*           result;
};

#endif /* TASK_H_ */
