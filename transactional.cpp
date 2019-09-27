#include "transactional.h"

int main (void)
{
	Database db ("test");

	for(const auto [name, age] : db.SELECT<std::string, int> ("SELECT * from ages")) 
	std::cout << name << ' ' << age << '\n';
}


