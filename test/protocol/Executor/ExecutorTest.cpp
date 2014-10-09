/**
 * Copyright 2014 University Corporation for Atmospheric Research. All rights
 * reserved. See the the file COPYRIGHT in the top-level source-directory for
 * licensing conditions.
 *
 *   @file: ExecutorTest.cpp
 * @author: Steven R. Emmerson
 *
 * This file tests the Executor class.
 */

#include "Executor.h"
#include <gtest/gtest.h>
#include <pthread.h>
#include <string>
#include <signal.h>
#include <stdexcept>

namespace {

// The fixture for testing class Executor.
class ExecutorTest : public ::testing::Test {
  protected:
      ExecutorTest()
          : one(1),
            two(2),
            terminatingTask1(&one),
            terminatingTask2(&two),
            indefiniteTask1(&one) {
      }

      class TerminatingTask : public Task {
      public:
          TerminatingTask(void* arg) : arg(arg) {};

          void* start()
          {
              sleep(1);
              return arg;
          }

      private:
          void* arg;
      };

      class IndefiniteTask : public Task {
      public:
          IndefiniteTask(void* arg)
              : arg(arg),
                done(0)
          {
                pthread_mutex_init(&mutex, 0);
                pthread_cond_init(&cond, 0);
          };

          void* start()
          {
              (void)pthread_mutex_lock(&mutex);
              while(!done)
                  (void)pthread_cond_wait(&cond, &mutex);
              (void)pthread_mutex_unlock(&mutex);
              return arg;
          }

          void cancel() {
              (void)pthread_mutex_lock(&mutex);
              done = 1;
              (void)pthread_cond_signal(&cond);
              (void)pthread_mutex_unlock(&mutex);
          }

      private:
          void*                 arg;
          volatile sig_atomic_t done;
          pthread_mutex_t       mutex;
          pthread_cond_t        cond;
      };

      int             one;
      int             two;
      Executor        executor;
      TerminatingTask terminatingTask1;
      TerminatingTask terminatingTask2;
      IndefiniteTask  indefiniteTask1;
};

// Tests that the Executor correctly executes a single, self-terminating task.
TEST_F(ExecutorTest, OneSelfTerminatingTask) {
    executor.submit(terminatingTask1);

    Task& doneTask = executor.wait();

    EXPECT_TRUE(&terminatingTask1 == &doneTask);
    EXPECT_TRUE(&one == doneTask.getResult());
}

// Tests that the Executor correctly executes two, self-terminating tasks.
TEST_F(ExecutorTest, TwoSelfTerminatingTasks) {

    executor.submit(terminatingTask1);
    executor.submit(terminatingTask2);

    Task& doneTaskA = executor.wait();
    Task& doneTaskB = executor.wait();

    EXPECT_TRUE(&one == doneTaskA.getResult() || &one == doneTaskB.getResult());
    EXPECT_TRUE(&two == doneTaskA.getResult() || &two == doneTaskB.getResult());
    EXPECT_TRUE(doneTaskA.getResult() != doneTaskB.getResult());
}

// Tests that the Executor correctly stops an indefinite task.
TEST_F(ExecutorTest, IndefiniteTask) {

    executor.submit(indefiniteTask1);

    sleep(1);
    indefiniteTask1.stop();
    Task& doneTask = executor.wait();

    EXPECT_TRUE(&indefiniteTask1 == &doneTask);
    EXPECT_EQ(true, doneTask.wasStopped());
}

// Tests that the Executor won't accept the same indefinite task twice.
TEST_F(ExecutorTest, SameIndefiniteTask) {

    executor.submit(indefiniteTask1);
    try {
        executor.submit(indefiniteTask1);
        ASSERT_TRUE(false);
    }
    catch (std::exception& e) {
    }

    sleep(1);
    indefiniteTask1.stop();
    Task& doneTask = executor.wait();

    EXPECT_TRUE(&indefiniteTask1 == &doneTask);
    EXPECT_EQ(true, doneTask.wasStopped());
}

}  // namespace

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
