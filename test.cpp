#include <iostream>
#include "variant.h"
#include <variant>
#include <vector>
#include "vld.h"
using std::vector;
using std::string;
using std::cin;
using std::cout;
using std::endl;
using std::in_place_index_t;
using std::in_place_type_t;
using std::move;

struct my_struct
{
	int x;
	my_struct() : x(228) {}

	//my_struct(my_struct const&) = delete;

	my_struct(my_struct&&) noexcept(0) {}
};


int main() {
	//string a = "absss";
	//string b = "lslff";
	variant<int, char> t(5);
	variant<char, double> tt(3.22);

	//cout << variant_size_v<decltype(t)>;
	visit([](auto a, auto b) { cout << a << " " << b << endl; }, t, tt);
	return 0;
}