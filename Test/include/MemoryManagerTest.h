#ifndef MEMORY_MANAGER_TEST_H
#define MEMORY_MANAGER_TEST_H

#include "CuTest.h"

typedef void (*Test)(CuTest *const cuTest);

CuSuite *getMemoryManagerTestSuite(void);

#endif
