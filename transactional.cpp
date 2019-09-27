#include "transactional.h"

int main (void)
{
	const Database db ("test");

	for(const auto [name, age] : db.SELECT<std::string, int> ("* from ages; DROP table ages;")) 
	std::cout << name << ' ' << age << '\n';
}


