#include <stdlib.h>

weak_decl int abs(int a)
{
	return a>0 ? a : -a;
}
