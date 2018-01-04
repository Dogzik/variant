#include <iostream>
#include "variant.h"


int main() {
	variant<double, bool, int> t;
	bool f = t.valueless_by_exception();
	return 0;
}