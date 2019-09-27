#include "sqlite3.h" 
#include <exception>
#include <string>
#include <variant>
#include <iostream>
#include <vector>
#include <tuple>


class sqliteError : public std::exception
{
	const int code;

	public:
	explicit sqliteError (const int c)
		:code(c)
	{}

	virtual const char* what (void) const throw()
	{
		return sqlite3_errstr(code);
	}
};

template<typename F>
class Cleaner
{
	const F clean; 
	bool cancelled; 

	public:
	Cleaner (const F c)
		: clean(c), cancelled(false)
	{}
	Cleaner (Cleaner const&) = delete;

	~Cleaner (void)
	{
		if(!cancelled) clean();
	}

	void done (void)
	{
		cancelled = true;
	}
};

template<typename F>
void sqlExpect (F f, const int out)
{
	int rc = f(); 
	if(rc != out) throw sqliteError(rc); 
}


class Database 
{
	const std::string file;
	sqlite3* db;

	public:

	Database  (const std::string& f)
		:file(f)
	{
		sqlite3_initialize();
		Cleaner c([&](){std::cout << sqlite3_errmsg(db); sqlite3_close(db);});
		sqlExpect([&](void) -> int { return sqlite3_open_v2(file.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);}, SQLITE_OK);
		c.done();
	}
	
	~Database (void) //default behavior? use exit policy and also constrain with concepts! 
	{
		sqlite3_close(db);
		sqlite3_shutdown();
	}



	template<typename... types>
	std::vector<std::tuple<types...>> SELECT (const std::string& query)
	{
		sqlite3_stmt* stmt = nullptr;
		sqlExpect([&](void) -> int { return sqlite3_prepare_v2(db, query.c_str(), query.length(), &stmt, nullptr);}, SQLITE_OK);

		std::vector<std::tuple<types...>> acc;
		const int size = std::tuple_size<std::tuple<types...>>::value;

		while(sqlite3_step(stmt) == SQLITE_ROW)
		{
			std::tuple<types...> out; 

			int i = 0;
			std::apply([&](auto&&... args) {((getColumn(args, i++, stmt)), ...);}, out);
			acc.push_back(out);
		}

		return acc;
		//assert that column matches row number, or just rly on throwing..
	}

	private:

	void getColumn (int& i, int cidx , sqlite3_stmt* s)
	{
		i = sqlite3_column_int(s, cidx);
	}

	void getColumn (double& d, int cidx, sqlite3_stmt* s)
	{
		d = sqlite3_column_double(s, cidx);
	}

	void getColumn (std::string& str, int cidx, sqlite3_stmt* s)
	{
		str = (const char*) sqlite3_column_text(s, cidx);
	}


};
