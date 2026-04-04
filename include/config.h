#ifndef CONFIG_H_
#define CONFIG_H_

typedef struct bob_config_s {
    char name[256];
    char email[256];
} bob_config_t;

int config_read(bob_config_t *cfg);

#endif /* !CONFIG_H_ */
