#include "stdbool.h"
#include "stddef.h"
#include "stdlib.h"
#include "ctype.h"
#include "errno.h"
#include "limits.h"
static int
Digit2Val( int c)
{
	if(isalpha(c)) {  /* If c is one of [A-Za-z]... */
		c = toupper(c) - 7;   // Adjust so 'A' is ('9' + 1)
	}
	return c - '0';   // Value returned is between 0 and 35, inclusive.
}
long
strtol(const char * __restrict nptr, char ** __restrict endptr, int base)
{
	const char *pEnd;
	long        Result = 0;
	long        Previous;
	int         temp;
	bool        Negative = false;

	pEnd = nptr;

	if((base < 0) || (base == 1) || (base > 36)) {
		if(endptr != NULL) {
			*endptr = NULL;
		}
		return 0;
	}
	// Skip leading spaces.
	while(isspace(*nptr))   ++nptr;

	// Process Subject sequence: optional sign followed by digits.
	if(*nptr == '+') {
		Negative = false;
		++nptr;
	}
	else if(*nptr == '-') {
		Negative = true;
		++nptr;
	}

	if(*nptr == '0') {  /* Might be Octal or Hex */
		if(toupper(nptr[1]) == 'X') {   /* Looks like Hex */
			if((base == 0) || (base == 16)) {
				nptr += 2;  /* Skip the "0X"      */
				base = 16;  /* In case base was 0 */
			}
		}
		else {    /* Looks like Octal */
			if((base == 0) || (base == 8)) {
				++nptr;     /* Skip the leading "0" */
				base = 8;   /* In case base was 0   */
			}
		}
	}
	if(base == 0) {   /* If still zero then must be decimal */
		base = 10;
	}
	if(*nptr  == '0') {
		for( ; *nptr == '0'; ++nptr);  /* Skip any remaining leading zeros */
		pEnd = nptr;
	}

	while( isalnum(*nptr) && ((temp = Digit2Val(*nptr)) < base)) {
		Previous = Result;
		Result = (Result * base) + (long int)temp;
		if( Result <= Previous) {   // Detect Overflow
			if(Negative) {
				Result = LONG_MIN;
			}
			else {
				Result = LONG_MAX;
			}
			Negative = false;
			errno = ERANGE;
			break;
		}
		pEnd = ++nptr;
	}
	if(Negative) {
		Result = -Result;
	}

	// Save pointer to final sequence
	if(endptr != NULL) {
		*endptr = (char *)pEnd;
	}
	return Result;
}
long long
strtoll(const char * __restrict nptr, char ** __restrict endptr, int base)
{
	const char *pEnd;
	long long   Result = 0;
	long long   Previous;
	int         temp;
	bool     Negative = false;

	pEnd = nptr;

	if((base < 0) || (base == 1) || (base > 36)) {
		if(endptr != NULL) {
			*endptr = NULL;
		}
		return 0;
	}
	// Skip leading spaces.
	while(isspace(*nptr))   ++nptr;

	// Process Subject sequence: optional sign followed by digits.
	if(*nptr == '+') {
		Negative = false;
		++nptr;
	}
	else if(*nptr == '-') {
		Negative = true;
		++nptr;
	}

	if(*nptr == '0') {  /* Might be Octal or Hex */
		if(toupper(nptr[1]) == 'X') {   /* Looks like Hex */
			if((base == 0) || (base == 16)) {
				nptr += 2;  /* Skip the "0X"      */
				base = 16;  /* In case base was 0 */
			}
		}
		else {    /* Looks like Octal */
			if((base == 0) || (base == 8)) {
				++nptr;     /* Skip the leading "0" */
				base = 8;   /* In case base was 0   */
			}
		}
	}
	if(base == 0) {   /* If still zero then must be decimal */
		base = 10;
	}
	if(*nptr  == '0') {
		for( ; *nptr == '0'; ++nptr);  /* Skip any remaining leading zeros */
		pEnd = nptr;
	}

	while( isalnum(*nptr) && ((temp = Digit2Val(*nptr)) < base)) {
		Previous = Result;
		Result = (Result * base) + (long long int)temp;
		if( Result <= Previous) {   // Detect Overflow
			if(Negative) {
				Result = LLONG_MIN;
			}
			else {
				Result = LLONG_MAX;
			}
			Negative = false;
			errno = ERANGE;
			break;
		}
		pEnd = ++nptr;
	}
	if(Negative) {
		Result = -Result;
	}

	// Save pointer to final sequence
	if(endptr != NULL) {
		*endptr = (char *)pEnd;
	}
	return Result;
}
