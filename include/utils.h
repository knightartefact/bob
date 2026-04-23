#ifndef UTILS_H_
#define UTILS_H_

void sha2hex(const unsigned char *digest, char *out);
int sha_equal(const unsigned char *a, const unsigned char *b);

#endif /* !UTILS_H_ */
