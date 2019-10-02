#include "transactional.h"

void continueTransaction (Database::Transaction&& t, Database& db)
{

	//tags to image bridge 
	db.CREATE("TABLE IF NOT EXISTS image_tag_bridge (hash STRING NOT_NULL REFERENCES images,"
													"tag STRING NOT_NULL REFERENCES tags)");
	//only commit if we can make all the tables..
	t.commit();
}

void buildDatabases (Database& db)
{
	auto t = db.transaction();
	//image identity database, holds hashes of all the images....
	db.CREATE("TABLE IF NOT EXISTS images (hash STRING NOT_NULL PRIMARY_KEY)");

	//identity skills 
	db.CREATE("TABLE IF NOT EXISTS idScore (hash STRING NOT_NULL PRIMARY_KEY REFERENCES identity(hash),"
											"mu REAL NOT_NULL, sigma REAL NOT_NULL)");
	//tag identity
	//table that holds all the tags
	db.CREATE("TABLE IF NOT EXISTS tags (tag STRING NOT_NULL PRIMARY KEY)");

	continueTransaction(std::move(t), db);
}




	
int main (void)
{
	Database db ("../test");
	buildDatabases(db);
}
