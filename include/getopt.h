#ifndef _GETOPT_H
#define _GETOPT_H
struct option{
	const char*name;
	int has_arg;
	int*flag;
	int val;
};
extern char *b_optarg;
extern int b_optind;
extern int b_opterr;
extern int b_optopt;
extern int b_optreset;
extern int b_getopt(
	int argc,
	char*const*argv,
	const char*optstring
);
extern int b_getlopt(
	int argc,
	char*const*argv,
	const char*optstring,
	const struct option*longopts,
	int*idx
);
extern int b_getlopt_only(
	int argc,
	char*const*argv,
	const char*optstring,
	const struct option*longopts,
	int*idx
);
#define no_argument 0
#define required_argument 1
#define optional_argument 2
#endif
