#include "transactional.h"

int test (void)
{
	Database db ("test");
	for(const auto [name, age] : db.SELECT<std::string, double> ("* from ages; DROP table ages;")) 
	std::cout << name << ' ' << age << '\n';

	auto transaction = db.transaction();
	auto stmt = db.INSERT("INTO ages (name, age) VALUES (?, ?)");
	stmt.push("promotion", 10.5);
	//will be rolledback automatically on scope close
	//
}


