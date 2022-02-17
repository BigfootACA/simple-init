#include <ctype.h>
#undef isupper
#undef islower
#undef isalpha
#undef isdigit
#undef isalnum
#undef isxdigit
int comp_isspace(int c){return c==' '||(unsigned)c-'\t'<5;}
int comp_isupper(int c){return (unsigned)c-'A'<26;}
int comp_islower(int c){return (unsigned)c-'a'<26;}
int comp_isalpha(int c){return ((unsigned)c|32)-'a'<26;}
int comp_isdigit(int c){return (unsigned)c-'0'<10;}
int comp_isxdigit(int c){return comp_isdigit(c)||((unsigned)c|32)-'a' < 6;}
int comp_isalnum(int c){return comp_isalpha(c)||comp_isdigit(c);}
int comp_toupper(int c){return (comp_islower(c))?(c&0x5f):c;}
int comp_tolower(int c){return (comp_isupper(c))?(c|32):c;}
