#include <ctype.h>
#include "compatible.h"
weak_decl int isspace(int c){return c==' '||(unsigned)c-'\t'<5;}
weak_decl int isupper(int c){return (unsigned)c-'A'<26;}
weak_decl int islower(int c){return (unsigned)c-'a'<26;}
weak_decl int isalpha(int c){return ((unsigned)c|32)-'a'<26;}
weak_decl int isdigit(int c){return (unsigned)c-'0'<10;}
weak_decl int isxdigit(int c){return isdigit(c)||((unsigned)c|32)-'a' < 6;}
weak_decl int isalnum(int c){return isalpha(c)||isdigit(c);}
weak_decl int iscntrl(int c){return (unsigned)c<0x20||c==0x7f;}
weak_decl int isgraph(int c){return (unsigned)c-0x21<0x5e;}
weak_decl int ispunct(int c){return isgraph(c)&&!isalnum(c);}
weak_decl int toupper(int c){return (islower(c))?(c&0x5f):c;}
weak_decl int tolower(int c){return (isupper(c))?(c|32):c;}
weak_decl int isprint(int c){return (unsigned)c-0x20<0x5f;}