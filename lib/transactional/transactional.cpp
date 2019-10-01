#include "transactional.h"

int test (void)
{
	Database db ("test");
	for(const auto [name, age] : db.SELECT<std::string, double> ("* from ages; DROP table ages;")) 
	std::cout << name << ' ' << age << '\n';

	auto transaction = db.transaction();
	auto stmt = db.INSERT("INTO ages (name, age) VALUES (?, ?)");

	if(db.SELECT<std::string, double>("* from ages where name = 'promotion'").size() == 0)
	{
		std::cout << "inserting PROMOTION" << std::endl;
		stmt.push("promotion", 10.5);
		transaction.commit();
	}
	//will be rolledback automatically on scope close
	//
}


