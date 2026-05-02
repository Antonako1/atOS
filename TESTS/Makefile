CC = gcc
CFLAGS = -I./stubs -I../SOURCE -I../SOURCE/KERNEL/32RTOSKRNL -w -O0 -g -DTEST_HOST -fno-builtin

TEST_BINS = test_string test_math test_mem test_bitmap test_arghand

all: $(TEST_BINS)
	@failed=0; \
	for t in $(TEST_BINS); do \
		echo ""; \
		echo "--- Running $$t ---"; \
		./$$t || failed=$$((failed + 1)); \
	done; \
	echo ""; \
	if [ $$failed -eq 0 ]; then \
		echo "All tests passed."; \
	else \
		echo "$$failed test binary(s) had failures."; \
		exit 1; \
	fi

test_string: test_string.c ../SOURCE/STD/STRING.c stubs/os_stubs.c
	$(CC) $(CFLAGS) $^ -o $@

test_math: test_math.c ../SOURCE/STD/MATH.c stubs/os_stubs.c
	$(CC) $(CFLAGS) $^ -o $@

test_mem: test_mem.c ../SOURCE/STD/MEM.c stubs/mem_test_stubs.c
	$(CC) $(CFLAGS) $^ -o $@

test_bitmap: test_bitmap.c ../SOURCE/STD/BITMAP.c stubs/os_stubs.c
	$(CC) $(CFLAGS) $^ -o $@

test_arghand: test_arghand.c ../SOURCE/LIBRARIES/ARGHAND/ARGHAND.c ../SOURCE/STD/STRING.c stubs/os_stubs.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(TEST_BINS)

.PHONY: all clean
