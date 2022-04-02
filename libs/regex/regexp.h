#ifndef regexp_h
#define regexp_h

typedef struct Reprog Reprog;
typedef struct Resub Resub;

Reprog *regexp_comp(const char *pattern, int cflags, const char **errorp);
int regexp_exec(Reprog *prog, const char *string, Resub *sub, int eflags);
void regexp_free(Reprog *prog);

enum {
	/* regexp_comp flags */
	REG_ICASE = 1,
	REG_NEWLINE = 2,

	/* regexp_exec flags */
	REG_NOTBOL = 4,

	/* limits */
	REG_MAXSUB = 10
};

struct Resub {
	unsigned int nsub;
	struct {
		const char *sp;
		const char *ep;
	} sub[REG_MAXSUB];
};

#endif
