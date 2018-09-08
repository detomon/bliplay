#include "test.h"
#include "BKString.h"

int main (int argc, char const * argv [])
{
	BKString string = BK_STRING_INIT;

	assert (BKStrnlen(NULL, 12) == 0);
	assert (BKStrnlen("abcd", 12) == 4);
	assert (BKStrnlen("abcdefghij", 6) == 6);
	assert (BKStrnlen("abcdefghij", 10) == 10);

	return RESULT_PASS;
}
