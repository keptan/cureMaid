#include "sqlite3.h" 
#include <exception>
#include <string>
#include <variant>
#include <iostream>
#include <vector>
#include <tuple>
#include <assert.h>

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

template<typename F, typename T>
void sqlExpect (F f, T test)
{
	int rc = f(); 
	if(!test(rc)) throw sqliteError(rc); 
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
		sqlite3_errmsg(db);
		sqlite3_close(db);
		sqlite3_shutdown();
	}



	template<typename... types>
	std::vector<std::tuple<types...>> SELECT (const std::string& query) const
	{
		using Tuple = std::tuple<types...>;
		const std::string sQuery = "SELECT " + query; 
		
		sqlite3_stmt* stmt = nullptr;
		sqlExpect([&](void) -> int { return sqlite3_prepare_v2(db, sQuery.c_str(), sQuery.length(), &stmt, nullptr);}, SQLITE_OK);

		std::vector<Tuple> acc;
		const int size = std::tuple_size<Tuple>::value;
		assert(sqlite3_column_count(stmt) == size);

		while(sqlite3_step(stmt) == SQLITE_ROW)
		{
			Tuple out; 

			int i = 0;
			std::apply([&](auto&&... args) {((getColumn(args, i++, stmt)), ...);}, out);
			acc.push_back(out);
		}


		sqlite3_finalize(stmt);
		return acc;
		//assert that column matches row number, or just rly on throwing..
	}

	class BindingTransaction
	{
		sqlite3_stmt *stmt;

		public:

		BindingTransaction (const Database& db, const std::string& sql, const std::string& query)
		{
			const std::string sQuery = sql + " "  + query; 
			sqlExpect([&](void) -> int { return sqlite3_prepare_v2(db.db, sQuery.c_str(), sQuery.length(), &stmt, nullptr);}, SQLITE_OK);
		}

		~BindingTransaction (void)
		{
			sqlite3_finalize(stmt);
		}

		void bind (const int in, int pos = 1)
		{

			sqlExpect([&](void) -> int { return sqlite3_bind_int(stmt, pos, in);}, SQLITE_OK);
		}

		void bind (const std::string& in, int pos = 1)
		{

			sqlExpect([&](void) -> int { return sqlite3_bind_text(stmt, pos, in.c_str(), in.length(), nullptr);}, SQLITE_OK);
		}

		void bind (const double in, int pos = 1)
		{

			sqlExpect([&](void) -> int { return sqlite3_bind_double(stmt, pos, in);}, SQLITE_OK);
		}



		template<typename... types>
		void push (types... args)
		{
			int i = 1;
			(bind(args, i++) ,...);

			sqlExpect([&](void) -> int { return sqlite3_step(stmt);}, [](const auto a){return a == SQLITE_OK || a == SQLITE_ROW || a == SQLITE_DONE;});
			sqlExpect([&](void) -> int { return sqlite3_reset(stmt);}, SQLITE_OK);
		}
	};

	class Transaction
	{
		const Database& db;
		bool committed; 
		public:
		Transaction (const Database& db)
			:db(db),  committed(false)
		{
			sqlite3_stmt *stmt;
			sqlExpect([&](void) -> int { return sqlite3_prepare_v2(db.db, "BEGIN DEFERRED TRANSACTION",-1, &stmt, nullptr);}, SQLITE_OK);
			sqlExpect([&](void) -> int { return sqlite3_step(stmt);}, [](const auto a){return a == SQLITE_OK || a == SQLITE_DONE;});
			sqlite3_finalize(stmt);
		};

		void commit (void)
		{
			committed = true;
		}

		~Transaction (void)
		{
			if(!committed)
			{
				sqlite3_stmt *stmt;
				sqlExpect([&](void) -> int { return sqlite3_prepare_v2(db.db, "ROLLBACK TRANSACTION",-1, &stmt, nullptr);}, SQLITE_OK);
				sqlExpect([&](void) -> int { return sqlite3_step(stmt);}, [](const auto a){return a == SQLITE_OK || a == SQLITE_DONE || a == SQLITE_ROW;});
				sqlite3_finalize(stmt);
			}
			else 
			{
				sqlite3_stmt *stmt;

				sqlExpect([&](void) -> int { return sqlite3_prepare_v2(db.db, "COMMIT TRANSACTION",-1, &stmt, nullptr);}, SQLITE_OK);
				sqlExpect([&](void) -> int { return sqlite3_step(stmt);}, [](const auto a){return a == SQLITE_OK || a == SQLITE_DONE;});
				sqlite3_finalize(stmt);
				committed = true;
			}
		}
	};

	Transaction transaction (void)
	{
		return Transaction(*this);
	}

	BindingTransaction INSERT (const std::string& query)
	{
		return BindingTransaction (*this, "INSERT", query);
	}

	BindingTransaction UPDATE (const std::string& query)
	{
		return BindingTransaction(*this, "UPDATE", query);
	}

	BindingTransaction DELETE (const std::string& query)
	{
		return BindingTransaction(*this, "DELETE", query);
	}

	void CREATE (const std::string& q)
	{
		sqlite3_stmt* stmt; 

		std::string Qs = "CREATE " + q;
		sqlExpect([&](void) -> int { return sqlite3_prepare_v2(db, Qs.c_str(), Qs.length(), &stmt, nullptr);}, SQLITE_OK);
		sqlExpect([&](void) -> int { return sqlite3_step(stmt);}, SQLITE_DONE);
		sqlite3_finalize(stmt); 
	}
		


	private:

	void getColumn (int& i, int cidx , sqlite3_stmt* s) const
	{
		assert(sqlite3_column_type(s, cidx) == SQLITE_FLOAT || sqlite3_column_type(s, cidx) == SQLITE_INTEGER);
		i = sqlite3_column_int(s, cidx);
	}

	void getColumn (double& d, int cidx, sqlite3_stmt* s) const
	{
		assert(sqlite3_column_type(s, cidx) == SQLITE_FLOAT || sqlite3_column_type(s, cidx) == SQLITE_INTEGER);
		d = sqlite3_column_double(s, cidx);
	}

	void getColumn (std::string& str, int cidx, sqlite3_stmt* s) const
	{
		assert(sqlite3_column_type(s, cidx) == SQLITE_TEXT);
		str = (const char*) sqlite3_column_text(s, cidx);
	}
};

int test (void);
