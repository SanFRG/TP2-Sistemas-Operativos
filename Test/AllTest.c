#include <stdio.h>

#include "CuTest.h"
#include "MemoryManagerTest.h"

int RunAllTests(void) {
	CuString *output = CuStringNew();
	CuSuite *suite = getMemoryManagerTestSuite();
	int failCount;

	CuSuiteRun(suite);

	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);

	printf("%s\n", output->buffer);

	failCount = suite->failCount;

	CuStringDelete(output);
	CuSuiteDelete(suite);

	return failCount;
}

int main(void) {
	/* Non-zero exit code when a test fails, so `make test` reports failure. */
	return RunAllTests() == 0 ? 0 : 1;
}
