#include <iostream>
#include "variant.h"

int main() {
	variant<double, char, int> t(5);
	bool f = t.valueless_by_exception();
	char* x;
	try
	{
		x = get_if<1>(&t);
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}