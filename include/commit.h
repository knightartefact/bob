#ifndef COMMIT_H_
#define COMMIT_H_

// Creates a commit object from a tree hash, optional parent hash, and message.
// tree_hex: 40-char hex SHA1 of the tree object.
// parent_hex: 40-char hex SHA1 of parent commit, or NULL for root commit.
// message: commit message string.
// Returns 20-byte binary SHA1 digest (caller must free), or NULL on error.
char *commit_create(const char *tree_hex, const char *parent_hex, const char *message);

#endif /* !COMMIT_H_ */
