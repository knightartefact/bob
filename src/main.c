#include <stdio.h>
#include <openssl/sha.h>
#include <string.h>

int main(int ac, char **av)
{
    unsigned char hash[20] = {0};
    SHA1(av[1], strlen(av[1]), hash);
    for (int i = 0; i < 20; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
    return 0;
}
