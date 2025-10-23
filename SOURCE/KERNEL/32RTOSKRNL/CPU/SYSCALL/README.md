Arguments in syscall

If an argument expects pointers or strings, they need to be allocated in heap

Every process and kernel shares the heap so everyone can access the allocation with the same pointer

If a stack variable is dereferenced and that is used as a pointer, it is not quaranteed to work? (I quess. haven't even tested:)