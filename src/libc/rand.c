// newlib libc/stdlib/rand.c (newlib as shipped with GCC 2.9-arm-000512)

#define RAND_MAX 0x7FFFFFFF

// Only the member used here; _rand_next sits at 0x58 in this newlib's struct _reent.
struct _reent
{
    char filler[0x58];
    unsigned long _rand_next;
};

extern struct _reent *_impure_ptr;
#define _REENT _impure_ptr

void srand(unsigned int seed)
{
    _REENT->_rand_next = seed;
}

int rand(void)
{
    return ((_REENT->_rand_next = _REENT->_rand_next * 1103515245 + 12345) & RAND_MAX);
}
