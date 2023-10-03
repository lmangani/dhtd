
#ifndef _EXT_ANNOUNCES_H_
#define _EXT_ANNOUNCES_H_

#include <sys/time.h>
#include <stdio.h>

/*
* Announce a value id / port pair in regular
* intervals until the lifetime expires.
*/

struct announcement_t {
	struct announcement_t *next;
	uint8_t id[SHA1_BIN_LENGTH];
	int port;
	time_t lifetime; // Keep entry refreshed until the lifetime expires
	time_t refresh; // Next time the entry need to be refreshed
};

void announces_setup(void);
void announces_free(void);

struct announcement_t* announces_get(void);
struct announcement_t* announces_find(const uint8_t id[]);

int announces_count();

// List all entries
void announces_debug(FILE *fp);

// Add a value id / port that will be announced until lifetime is exceeded
//struct announcement_t *announces_add(const char query[], int port, time_t lifetime);
struct announcement_t *announces_add(uint8_t id[], int port, time_t lifetime);


#endif // _EXT_ANNOUNCES_H_
