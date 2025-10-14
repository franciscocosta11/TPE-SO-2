#include <stdio.h>

#include "CuTest.h"
#include "MemoryManagerTest.h"
#include "ProcessTest.h"

void RunAllTests(void) {
	CuString *output = CuStringNew();
	CuSuite *suite = CuSuiteNew();

	CuSuiteAddSuite(suite, getMemoryManagerTestSuite());
	CuSuiteAddSuite(suite, getProcessTestSuite());

	CuSuiteRun(suite);

	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);

	printf("%s\n", output->buffer);
}

int main(void) {
	RunAllTests();
	return 0;
}
