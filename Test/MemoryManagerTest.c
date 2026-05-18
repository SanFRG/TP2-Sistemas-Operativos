#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "CuTest.h"
#include "MemoryManagerTest.h"
#include "memoryManager.h"

#define MANAGED_MEMORY_SIZE 65536
#define ALLOCATION_SIZE 1024
#define BIG_ALLOCATION_SIZE (ALLOCATION_SIZE * 8)
#define WRITTEN_VALUE 'a'
#define MAX_EXHAUST_BLOCKS 256
#define PATTERN_BLOCKS 8

void testInitialStatus(CuTest *const cuTest);
void testAllocMemory(CuTest *const cuTest);
void testTwoAllocations(CuTest *const cuTest);
void testWriteMemory(CuTest *const cuTest);
void testFreeUpdatesStatus(CuTest *const cuTest);
void testAllocAfterFree(CuTest *const cuTest);
void testAllocZeroFails(CuTest *const cuTest);
void testOversizedAllocFails(CuTest *const cuTest);
void testFreeNullIsIgnored(CuTest *const cuTest);
void testFreeInvalidPointerIsIgnored(CuTest *const cuTest);
void testDoubleFreeIsIgnored(CuTest *const cuTest);
void testFreedBlockIsReused(CuTest *const cuTest);
void testHeapCanBeExhaustedAndReused(CuTest *const cuTest);
void testCoalescingReclaimsContiguousSpace(CuTest *const cuTest);
void testAllocationsDoNotCorruptEachOther(CuTest *const cuTest);

static const size_t TestQuantity = 15;
static const Test MemoryManagerTests[] = {
	testInitialStatus,    testAllocMemory,           testTwoAllocations,    testWriteMemory,
	testFreeUpdatesStatus, testAllocAfterFree,       testAllocZeroFails,    testOversizedAllocFails,
	testFreeNullIsIgnored, testFreeInvalidPointerIsIgnored, testDoubleFreeIsIgnored,
	testFreedBlockIsReused, testHeapCanBeExhaustedAndReused,
	testCoalescingReclaimsContiguousSpace, testAllocationsDoNotCorruptEachOther};

/* given */
static inline void givenAMemoryManager(void);
static inline void givenAMemoryAmount(void);
static inline void givenAnAllocation(void);
static inline void givenThreeAllocations(void);
static inline void givenPatternBlocks(void);

/* when */
static inline void whenMemoryIsAllocated(void);
static inline void whenMemoryIsWritten(void);
static inline void whenTheAllocationIsFreed(void);
static inline void whenTheAllocationIsFreedTwice(void);
static inline void whenNullIsFreed(void);
static inline void whenAnInvalidPointerIsFreed(void);
static inline void whenTheMiddleBlockIsFreed(void);
static inline void whenTheHeapIsExhausted(void);
static inline void whenTheExhaustedHeapIsFreed(void);
static inline void whenABigBlockIsAllocated(void);
static inline void whenEachBlockIsFilledWithItsOwnPattern(void);

/* then */
static inline void thenSomeMemoryIsReturned(CuTest *const cuTest);
static inline void thenNoMemoryIsReturned(CuTest *const cuTest);
static inline void thenTheTwoAdressesAreDifferent(CuTest *const cuTest);
static inline void thenBothDoNotOverlap(CuTest *const cuTest);
static inline void thenMemorySuccessfullyWritten(CuTest *const cuTest);
static inline void thenStatusIsConsistentAfterInit(CuTest *const cuTest);
static inline void thenStatusReflectsTheFree(CuTest *const cuTest);
static inline void thenAFailedAllocationIsCounted(CuTest *const cuTest);
static inline void thenNoFreeIsCounted(CuTest *const cuTest);
static inline void thenOnlyOneFreeIsCounted(CuTest *const cuTest);
static inline void thenTheFreedBlockIsReused(CuTest *const cuTest);
static inline void thenAtLeastOneAllocationSucceeded(CuTest *const cuTest);
static inline void thenNoBlockWasCorrupted(CuTest *const cuTest);

/* 8-byte aligned backing store for the managed heap. */
static uint64_t managedMemory[MANAGED_MEMORY_SIZE / sizeof(uint64_t)];

/* A variable that lives outside the managed heap (on the stack/BSS). */
static int outsideHeapVariable;

static size_t memoryToAllocate;

static void *allocatedMemory = NULL;
static void *anAllocation = NULL;

static void *blockA = NULL;
static void *blockB = NULL;
static void *blockC = NULL;

static void *exhaustPointers[MAX_EXHAUST_BLOCKS];
static size_t exhaustCount = 0;

static void *patternPointers[PATTERN_BLOCKS];

CuSuite *getMemoryManagerTestSuite(void) {
	CuSuite *const suite = CuSuiteNew();

	for (size_t i = 0; i < TestQuantity; i++)
		SUITE_ADD_TEST(suite, MemoryManagerTests[i]);

	return suite;
}

/*-------------------------------------------------------------------------*
 * Tests
 *-------------------------------------------------------------------------*/

void testInitialStatus(CuTest *const cuTest) {
	givenAMemoryManager();

	thenStatusIsConsistentAfterInit(cuTest);
}

void testAllocMemory(CuTest *const cuTest) {
	givenAMemoryManager();
	givenAMemoryAmount();

	whenMemoryIsAllocated();

	thenSomeMemoryIsReturned(cuTest);
}

void testTwoAllocations(CuTest *const cuTest) {
	givenAMemoryManager();
	givenAMemoryAmount();
	givenAnAllocation();

	whenMemoryIsAllocated();

	thenSomeMemoryIsReturned(cuTest);
	thenTheTwoAdressesAreDifferent(cuTest);
	thenBothDoNotOverlap(cuTest);
}

void testWriteMemory(CuTest *const cuTest) {
	givenAMemoryManager();
	givenAMemoryAmount();
	givenAnAllocation();

	whenMemoryIsWritten();

	thenMemorySuccessfullyWritten(cuTest);
}

void testFreeUpdatesStatus(CuTest *const cuTest) {
	givenAMemoryManager();
	givenAMemoryAmount();
	givenAnAllocation();

	whenTheAllocationIsFreed();

	thenStatusReflectsTheFree(cuTest);
}

void testAllocAfterFree(CuTest *const cuTest) {
	givenAMemoryManager();
	givenAMemoryAmount();
	givenAnAllocation();

	whenTheAllocationIsFreed();
	whenMemoryIsAllocated();

	thenSomeMemoryIsReturned(cuTest);
}

void testAllocZeroFails(CuTest *const cuTest) {
	givenAMemoryManager();

	memoryToAllocate = 0;
	whenMemoryIsAllocated();

	thenNoMemoryIsReturned(cuTest);
	thenAFailedAllocationIsCounted(cuTest);
}

void testOversizedAllocFails(CuTest *const cuTest) {
	givenAMemoryManager();

	memoryToAllocate = (size_t) MANAGED_MEMORY_SIZE * 2;
	whenMemoryIsAllocated();

	thenNoMemoryIsReturned(cuTest);
	thenAFailedAllocationIsCounted(cuTest);
}

void testFreeNullIsIgnored(CuTest *const cuTest) {
	givenAMemoryManager();

	whenNullIsFreed();

	thenNoFreeIsCounted(cuTest);
}

void testFreeInvalidPointerIsIgnored(CuTest *const cuTest) {
	givenAMemoryManager();

	whenAnInvalidPointerIsFreed();

	thenNoFreeIsCounted(cuTest);
}

void testDoubleFreeIsIgnored(CuTest *const cuTest) {
	givenAMemoryManager();
	givenAMemoryAmount();
	givenAnAllocation();

	whenTheAllocationIsFreedTwice();

	thenOnlyOneFreeIsCounted(cuTest);
}

void testFreedBlockIsReused(CuTest *const cuTest) {
	givenAMemoryManager();
	givenAMemoryAmount();
	givenThreeAllocations();

	whenTheMiddleBlockIsFreed();
	whenMemoryIsAllocated();

	thenSomeMemoryIsReturned(cuTest);
	thenTheFreedBlockIsReused(cuTest);
}

void testHeapCanBeExhaustedAndReused(CuTest *const cuTest) {
	givenAMemoryManager();
	givenAMemoryAmount();

	whenTheHeapIsExhausted();

	thenAtLeastOneAllocationSucceeded(cuTest);
	thenAFailedAllocationIsCounted(cuTest);

	whenTheExhaustedHeapIsFreed();
	whenMemoryIsAllocated();

	thenSomeMemoryIsReturned(cuTest);
}

void testCoalescingReclaimsContiguousSpace(CuTest *const cuTest) {
	givenAMemoryManager();
	givenAMemoryAmount();

	/* Fill the heap with small blocks, then free them all. A big request
	 * only fits afterwards if the freed blocks were coalesced back. */
	whenTheHeapIsExhausted();
	whenTheExhaustedHeapIsFreed();
	whenABigBlockIsAllocated();

	thenSomeMemoryIsReturned(cuTest);
}

void testAllocationsDoNotCorruptEachOther(CuTest *const cuTest) {
	givenAMemoryManager();
	givenAMemoryAmount();
	givenPatternBlocks();

	whenEachBlockIsFilledWithItsOwnPattern();

	thenNoBlockWasCorrupted(cuTest);
}

/*-------------------------------------------------------------------------*
 * given
 *-------------------------------------------------------------------------*/

inline void givenAMemoryManager(void) {
	allocatedMemory = NULL;
	anAllocation = NULL;
	blockA = NULL;
	blockB = NULL;
	blockC = NULL;
	exhaustCount = 0;
	memoryToAllocate = 0;
	for (size_t i = 0; i < PATTERN_BLOCKS; i++)
		patternPointers[i] = NULL;
	mm_init(managedMemory, MANAGED_MEMORY_SIZE);
}

inline void givenAMemoryAmount(void) {
	memoryToAllocate = ALLOCATION_SIZE;
}

inline void givenAnAllocation(void) {
	anAllocation = mm_alloc(memoryToAllocate);
}

inline void givenThreeAllocations(void) {
	blockA = mm_alloc(memoryToAllocate);
	blockB = mm_alloc(memoryToAllocate);
	blockC = mm_alloc(memoryToAllocate);
}

inline void givenPatternBlocks(void) {
	for (size_t i = 0; i < PATTERN_BLOCKS; i++)
		patternPointers[i] = mm_alloc(memoryToAllocate);
}

/*-------------------------------------------------------------------------*
 * when
 *-------------------------------------------------------------------------*/

inline void whenMemoryIsAllocated(void) {
	allocatedMemory = mm_alloc(memoryToAllocate);
}

inline void whenMemoryIsWritten(void) {
	for (size_t i = 0; i < ALLOCATION_SIZE; i++)
		((char *) anAllocation)[i] = WRITTEN_VALUE;
}

inline void whenTheAllocationIsFreed(void) {
	mm_free(anAllocation);
}

inline void whenTheAllocationIsFreedTwice(void) {
	mm_free(anAllocation);
	mm_free(anAllocation);
}

inline void whenNullIsFreed(void) {
	mm_free(NULL);
}

inline void whenAnInvalidPointerIsFreed(void) {
	mm_free(&outsideHeapVariable);
}

inline void whenTheMiddleBlockIsFreed(void) {
	mm_free(blockB);
}

inline void whenTheHeapIsExhausted(void) {
	void *block;

	while (exhaustCount < MAX_EXHAUST_BLOCKS && (block = mm_alloc(memoryToAllocate)) != NULL)
		exhaustPointers[exhaustCount++] = block;
}

inline void whenTheExhaustedHeapIsFreed(void) {
	for (size_t i = 0; i < exhaustCount; i++)
		mm_free(exhaustPointers[i]);
}

inline void whenABigBlockIsAllocated(void) {
	allocatedMemory = mm_alloc(BIG_ALLOCATION_SIZE);
}

inline void whenEachBlockIsFilledWithItsOwnPattern(void) {
	for (size_t i = 0; i < PATTERN_BLOCKS; i++)
		if (patternPointers[i] != NULL)
			memset(patternPointers[i], (int) (i + 1), ALLOCATION_SIZE);
}

/*-------------------------------------------------------------------------*
 * then
 *-------------------------------------------------------------------------*/

inline void thenSomeMemoryIsReturned(CuTest *const cuTest) {
	CuAssertPtrNotNull(cuTest, allocatedMemory);
}

inline void thenNoMemoryIsReturned(CuTest *const cuTest) {
	CuAssertPtrEquals(cuTest, NULL, allocatedMemory);
}

inline void thenTheTwoAdressesAreDifferent(CuTest *const cuTest) {
	CuAssertTrue(cuTest, anAllocation != allocatedMemory);
}

inline void thenBothDoNotOverlap(CuTest *const cuTest) {
	long distance = (long) ((char *) anAllocation - (char *) allocatedMemory);
	if (distance < 0)
		distance = -distance;

	CuAssertTrue(cuTest, distance >= ALLOCATION_SIZE);
}

inline void thenMemorySuccessfullyWritten(CuTest *const cuTest) {
	uint8_t writtenValue = WRITTEN_VALUE;

	for (size_t i = 0; i < ALLOCATION_SIZE; i++) {
		uint8_t readValue = ((uint8_t *) anAllocation)[i];
		CuAssertIntEquals(cuTest, writtenValue, readValue);
	}
}

inline void thenStatusIsConsistentAfterInit(CuTest *const cuTest) {
	mm_status_t status;
	mm_get_status(&status);

	CuAssertTrue(cuTest, status.total_bytes > 0);
	CuAssertTrue(cuTest, status.used_bytes == 0);
	CuAssertTrue(cuTest, status.free_bytes > 0);
	CuAssertTrue(cuTest, status.free_bytes <= status.total_bytes);
	CuAssertTrue(cuTest, status.successful_allocations == 0);
	CuAssertTrue(cuTest, status.successful_frees == 0);
}

inline void thenStatusReflectsTheFree(CuTest *const cuTest) {
	mm_status_t status;
	mm_get_status(&status);

	CuAssertTrue(cuTest, status.successful_allocations == 1);
	CuAssertTrue(cuTest, status.successful_frees == 1);
	CuAssertTrue(cuTest, status.used_bytes == 0);
}

inline void thenAFailedAllocationIsCounted(CuTest *const cuTest) {
	mm_status_t status;
	mm_get_status(&status);

	CuAssertTrue(cuTest, status.failed_allocations >= 1);
}

inline void thenNoFreeIsCounted(CuTest *const cuTest) {
	mm_status_t status;
	mm_get_status(&status);

	CuAssertTrue(cuTest, status.successful_frees == 0);
}

inline void thenOnlyOneFreeIsCounted(CuTest *const cuTest) {
	mm_status_t status;
	mm_get_status(&status);

	CuAssertTrue(cuTest, status.successful_frees == 1);
}

inline void thenTheFreedBlockIsReused(CuTest *const cuTest) {
	CuAssertPtrEquals(cuTest, blockB, allocatedMemory);
}

inline void thenAtLeastOneAllocationSucceeded(CuTest *const cuTest) {
	CuAssertTrue(cuTest, exhaustCount > 0);
}

inline void thenNoBlockWasCorrupted(CuTest *const cuTest) {
	for (size_t i = 0; i < PATTERN_BLOCKS; i++) {
		uint8_t expected = (uint8_t) (i + 1);
		uint8_t *block;

		CuAssertPtrNotNull(cuTest, patternPointers[i]);

		block = (uint8_t *) patternPointers[i];
		for (size_t j = 0; j < ALLOCATION_SIZE; j++)
			CuAssertIntEquals(cuTest, expected, block[j]);
	}
}
