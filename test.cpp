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

template<typename T>
struct my_struct
{
	int x;
	my_struct() : x(228) {}


	template<typename U, std::enable_if_t<std::is_same_v<my_struct<T>, std::decay_t<U>>, int> = 0, std::enable_if_t<std::is_same_v<T, int>, int> = 0>
	my_struct(U&& other) : 
		x(other.x) 
	{}

	my_struct(my_struct const&) = delete;

	
};

int main() {
	string a = "absss";
	const variant<int, char, double> t(2.28);
	cout << t.index() << endl;

	auto&& b = get<2>(move(t));

	double c(move(b));

	//variant<string, char, int> tt(t);
	//cout << std::is_trivially_copyable_v<string> << endl;
	return 0;
}