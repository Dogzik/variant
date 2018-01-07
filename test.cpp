#include <iostream>
#include <variant>
#include <vector>
#include <string>
#include "variant.h"
#include "vld.h"
using std::vector;
using std::string;
using std::cin;
using std::cout;
using std::endl;
using std::in_place_index_t;
using std::in_place_type_t;
using std::move;

int main() {
	string a = "absss";
	variant<int, double, string> t(a);
	variant<int, double, string> tt(3.22);

	visit([](auto&& a, auto&& b) { cout << a << " " << b <<  endl; }, t, tt);

	auto w = visit([](auto&& arg)->variant<int, double, string> { return arg + arg; }, t);
	auto ww = visit([](auto&& arg)->variant<int, double, string> { return arg + arg; }, tt);

	visit([](auto&& a, auto&& b) { cout << a << " " << b << endl; }, w, ww);
	return 0;
}