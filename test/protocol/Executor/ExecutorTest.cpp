/*
 * This part of includes is fixed and provides standard CppUnit test
 * controllers and runners. Please remain unchanged.
 */
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TextOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TestResultCollector.h>

/*
 * Unit under test.
 */
#include "Executor.h"

#include <unistd.h>

using namespace std;

static void* myStartRoutine(void* arg)
{
    sleep(1);
}

/*
 * Always use the name of a class under test + Test as the test class's name.
 * And this class is partly fixed while we only need to modify the method
 * containing designed test cases. For each class to be tested, a simple copy
 * and paste from this template with modification upon test cases is enough.
 */
class ExecutorTest : public CppUnit::TestFixture {
    public:
        /*
         * This method will contain all the designed test cases against the
         * target class. Use CPPUNIT_ASSERT() to make a judgment if the runtime
         * result is exactly the same as it's expected to be. It's also
         * reasonable to create another test method for different member
         * functions, e.g. aTest(), bTest(). But do remember to add your new
         * method in to the CppUnit test suite below. And also update
         * printTestInfo() accordingly.
         */
        void runExecutorTest()
        {
            class MyTask : public Task {
            public:
                MyTask() : Task(myStartRoutine) {};
            }        task;
            Executor executor;

            executor.submit(task);

            Task& doneTask = executor.wait();
            CPPUNIT_ASSERT(&doneTask == &task);
        }

    private:
        CPPUNIT_TEST_SUITE(ExecutorTest);
        /*
         * If there is  more than one test method in this test class. Please
         * start a new line and pass the name of new test method to
         * CPPUNIT_TEST(), e.g. CPPUNIT_TEST(a), CPPUNIT_TEST(b).
         */
        CPPUNIT_TEST(runExecutorTest);
};

CPPUNIT_TEST_SUITE_REGISTRATION(ExecutorTest);

/*
 * @function name: printTestInfo
 * @description:
 * This is a function printing some test info to the stdout. A suggested way
 * is to write some output messages about what method has been tested by test
 * cases.
 * @input: None
 * @output: None
 * @return: None
 */
void printTestInfo()
{
    // cout << endl << "Testing list:" << endl;
}


/*
 * Do not change function main(). This is a fixed universal body of testing
 * procedure. It can be adapted to all test cases running by CppUnit. So just
 * re-write test cases for each class and remain the interfaces unchanged.
 */
int main(int argc, char* argv[])
{
    // Create the event manager and test controller
    CppUnit::TestResult controller;

    // Add a listener that collects test result
    CppUnit::TestResultCollector result;
    controller.addListener(&result);

    // Add a listener that print dots when test running.
    CppUnit::BriefTestProgressListener progress;
    controller.addListener(&progress);

    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry &registry =
            CppUnit::TestFactoryRegistry::getRegistry();
    runner.addTest(registry.makeTest());
    cout << endl << "Running Tests " << endl;
    runner.run(controller);

    // Print test in a compiler compatible format.
    cout << endl << "Test Results" << endl;
    CppUnit::CompilerOutputter outputter(&result, CppUnit::stdCOut());
    outputter.write();

    // print info of passed test cases here
    printTestInfo();

    return result.wasSuccessful() ? 0 : 1;
}
