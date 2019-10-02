#include "transactional.h"

int test (void)
{
	Database db ("../test");
	for(const auto [name, age] : db.SELECT<std::string, double> ("* from ages; DROP table ages;")) 
	std::cout << name << ' ' << age << '\n';

	auto transaction = db.transaction();
	auto stmt = db.INSERT("INTO ages (name, age) VALUES (?, ?)");
	auto updt = db.UPDATE("ages SET age = 99 WHERE name = ?");
	updt.push("dogg");

	if(db.SELECT<std::string, double>("* from ages where name = 'dogg'").size() == 0)
	{
		std::cout << "inserting DOG" << std::endl;
		stmt.push("dogg", 10.5);
	}

	db.CREATE("TABLE IF NOT EXISTS parts ("
			"name STRING NOT_NULL PRIMARY_KEY,"
			"price NUMBER NOT_NULL)");


	transaction.commit();
	//transaction.commit();
	//will be rolledback automatically on scope close
	//
}


