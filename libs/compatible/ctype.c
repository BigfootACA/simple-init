#include <ctype.h>
#undef isupper
#undef islower
#undef isalpha
#undef isdigit
#undef isalnum
#undef isxdigit
int isupper(int c){return (unsigned)c-'A'<26;}
int islower(int c){return (unsigned)c-'a'<26;}
int isalpha(int c){return ((unsigned)c|32)-'a'<26;}
int isdigit(int c){return (unsigned)c-'0'<10;}
int isxdigit(int c){return isdigit(c)||((unsigned)c|32)-'a' < 6;}
int isalnum(int c){return isalpha(c)||isdigit(c);}
int toupper(int c){return (islower(c))?(c&0x5f):c;}
int tolower(int c){return (isupper(c))?(c|32):c;}
