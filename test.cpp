#include <iostream>
#include "variant.h"
#include <variant>
#include "vld.h"
using std::string;
using std::cin;
using std::cout;
using std::endl;
using std::in_place_index_t;
using std::in_place_type_t;

struct my_struct
{
	int x;
	my_struct() : x(228) {}

	my_struct(my_struct const&) = delete;
};

int main() {
	variant<string, char, int> t(5);
	variant<string, char, int> tt(t);
	//variant<double, int, char> a(5);
	//variant<double, int, char> b(char(2));
	//cout << t.index() << endl;
	//cout << std::is_trivially_destructible_v<decltype(t)> << endl;
	//cout << holds_alternative<string>(t) << endl;

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