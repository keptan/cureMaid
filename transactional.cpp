#include "transactional.h"

int main (void)
{
	Database db ("test");

	for(const auto [name, age] : db.SELECT<std::string, int> ("* from ages; DROP table ages;")) 
	std::cout << name << ' ' << age << '\n';

	auto stmt = db.INSERT("INTO ages (name, age) VALUES (?, ?)");
	stmt.push("nameTest", 99);
}


