#include <iostream>
#include "variant.h"
#include <variant>
#include "vld.h"
using std::string;

struct my_struct
{
	int x;
	my_struct() : x(228) {}
};

int main() {
	variant<my_struct, char, int> t;
	//variant<double, int, char> a(5);
	//variant<double, int, char> b(char(2));


	//bool ff = a < b;
	//bool f = t.valueless_by_exception();
	//int* x;
	//try
	//{
	//	x = get_if<int>(&t);
	//}
	//catch (const std::exception& e)
	//{
	//	std::cout << e.what() << std::endl;
	//}

	//destroyable_storage_t<int, double, string> q(std::integral_constant<size_t, 2>{}, 8, 'a');
	//bool f = std::is_trivially_destructible_v<decltype(q)>;

	//string s = raw_get<2>(q.get_storage());

	//destroyable_storage_t<int, double, char> qq(std::integral_constant<size_t, 1>{}, 2.28);
	//bool ff = std::is_trivially_destructible_v<decltype(qq)>;
	return 0;
}