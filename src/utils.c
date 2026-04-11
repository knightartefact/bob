#include "utils.h"

#include <stdio.h>
#include <string.h>

void sha2hex(const unsigned char *digest, char *out)
{
    for (int i = 0; i < 20; i++)
        sprintf(out + i * 2, "%02x", digest[i]);
    out[40] = '\0';
}

int sha_equal(const unsigned char *a, const unsigned char *b)
{
    return memcmp(a, b, 20) == 0;
}
