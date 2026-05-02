CC = gcc
CFLAGS = -I./stubs -I../SOURCE -I../SOURCE/KERNEL/32RTOSKRNL -w -O0 -g -DTEST_HOST -fno-builtin

TEST_BINS = test_string.out test_math.out test_mem.out test_bitmap.out test_arghand.out

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

test_string.out: test_string.c ../SOURCE/STD/STRING.c stubs/os_stubs.c
	$(CC) $(CFLAGS) $^ -o $@.out

test_math.out: test_math.c ../SOURCE/STD/MATH.c stubs/os_stubs.c
	$(CC) $(CFLAGS) $^ -o $@.out

test_mem.out: test_mem.c ../SOURCE/STD/MEM.c stubs/mem_test_stubs.c
	$(CC) $(CFLAGS) $^ -o $@.out

test_bitmap.out: test_bitmap.c ../SOURCE/STD/BITMAP.c stubs/os_stubs.c
	$(CC) $(CFLAGS) $^ -o $@.out

test_arghand.out: test_arghand.c ../SOURCE/LIBRARIES/ARGHAND/ARGHAND.c ../SOURCE/STD/STRING.c stubs/os_stubs.c
	$(CC) $(CFLAGS) $^ -o $@.out

clean:
	rm -f $(TEST_BINS)

.PHONY: all clean
