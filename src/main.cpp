#include "transactional.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <fstream>

void buildDatabases (Database& db)
{
	//todo: add default values and triggers or whatever..
	
	//start a transaction that we can rollback if anything throws
	auto t = db.transaction();

	//we aren't allowed to paramaterize this btw 
	//image identity database, holds hashes of all the images....
	db.CREATE("TABLE IF NOT EXISTS images (hash STRING NOT_NULL PRIMARY_KEY UNIQUE)");

	//identity skills 
	db.CREATE("TABLE IF NOT EXISTS idScore (hash STRING NOT_NULL PRIMARY_KEY UNIQUE REFERENCES images,"
											"mu REAL NOT_NULL, sigma REAL NOT_NULL)");
	//tag identity
	//table that holds all the tags
	//tag scores
	db.CREATE("TABLE IF NOT EXISTS tags (tag STRING NOT_NULL PRIMARY KEY UNIQUE)");
	db.CREATE("TABLE IF NOT EXISTS tagScore (tag STRING NOT_NULL PRIMARY_KEY UNIQUE REFERENCES tags,"
											"mu REAL NOT_NULL, sigma REAL NOT_NULL)");
	db.CREATE("TABLE IF NOT EXISTS image_tag_bridge (hash STRING NOT_NULL REFERENCES images,"
													"tag STRING NOT_NULL REFERENCES tags,"
													"PRIMARY KEY (hash, tag))");

	//artist identity 
	db.CREATE("TABLE IF NOT EXISTS artists (artist STRING NOT_NULL PRIMARY KEY UNIQUE)");
	db.CREATE("TABLE IF NOT EXISTS artistScore (artist STRING NOT_NULL PRIMARY_KEY UNIQUE REFERENCES artists,"
											"mu REAL NOT_NULL, sigma REAL NOT_NULL)");
	db.CREATE("TABLE IF NOT EXISTS image_artist_bridge (hash STRING NOT_NULL REFERENCES images,"
													"artist STRING NOT_NULL REFERENCES artists,"
													"PRIMARY KEY (hash, artist))");

	//only commit if we can make all the tables..
	t.commit();
}

std::vector< std::tuple<std::string, double, double>> 
readScoreCSV (const std::string& file )
{
	std::vector< std::tuple<std::string, double, double>> acc;
	std::fstream fs(file); 
	fs.seekg(0, std::ios::beg); 

	std::string line; 
	while(std::getline(fs, line))
	{
		std::istringstream is (line); 
		std::string t; 
		double sigma, mu;

		is >> std::quoted(t) >> mu >> sigma; 

		acc.push_back( std::make_tuple(
					t, mu, sigma));
	}

	return acc;
}

std::vector< std::tuple<std::string, std::vector<std::string>>>
readTagsCSV (const std::string& file)
{
	std::vector< std::tuple< std::string, std::vector<std::string>>> out;
	std::fstream fs(file);
	fs.seekg(0, std::ios::beg);

	std::string line;

	while ( std::getline(fs, line))
	{
		std::string head;
		std::string acc;
		//replace with tagset? 
		std::vector<std::string> tags;

		auto i = line.begin();

		if(line.size() < 3) continue; 
		for(; *i != ','; i++)
		{
			head += *i;
		}
		i++;

		bool inQuote = false;

		for(; i != line.end(); i++)
		{
			if(*i == ',')
			{
				tags.push_back( acc);
				acc = "";
				continue; 
			}

			acc.push_back(*i);
		}
		tags.push_back(acc);
		out.push_back( std::make_tuple(head, tags));
	}

	return out;
}

void insertTagScores (Database& db, const std::string& file)
{
	auto ii	  = db.INSERT("OR IGNORE INTO tags (tag) VALUES (?)");
	auto si	  = db.INSERT("OR IGNORE INTO tagScore (tag, mu, sigma) VALUES (?, ?, ?)");

	for(const auto [tag, mu, sigma] : readScoreCSV(file)) 

	{
		try
		{
		ii.push(tag);
		si.push(tag, mu, sigma);
		}
		catch(const sqliteError& e)
		{
			if(e.code == SQLITE_CONSTRAINT)
			{
				continue;
			}
			else 
			{
				throw (e);
			}
		}
	}
}

void insertTags (Database& db, const std::string& file)
{

	auto ti	    = db.INSERT("OR IGNORE INTO tags (tag) VALUES (?)");
	auto bridge = db.INSERT("OR IGNORE INTO image_tag_bridge (hash, tag) VALUES (?, ?)");

	//need to catch meta-tags like 'no_gelbooru' 
	for(const auto [image, tags] : readTagsCSV(file))
	{
		for(const auto t : tags)
		{
			ti.push(t);
			bridge.push(image, t);
		}
	}
}

void insertArtists (Database& db, const std::string& file)
{

	auto ti	    = db.INSERT("OR IGNORE INTO artists (artist) VALUES (?)");
	auto bridge = db.INSERT("OR IGNORE INTO image_artist_bridge (hash, artist) VALUES (?, ?)");

	//need to catch meta-tags like 'no_gelbooru' 
	for(const auto [image, tags] : readTagsCSV(file))
	{
		for(const auto t : tags)
		{
			ti.push(t);
			bridge.push(image, t);
		}
	}
}

void insertArtistScores (Database& db, const std::string& file)
{
	auto ii	  = db.INSERT("OR IGNORE INTO artists (artist) VALUES (?)");
	auto si	  = db.INSERT("OR IGNORE INTO artistScore (artist, mu, sigma) VALUES (?, ?, ?)");

	for(const auto [tag, mu, sigma] : readScoreCSV(file)) 

	{
		try
		{
		ii.push(tag);
		si.push(tag, mu, sigma);
		}
		catch(const sqliteError& e)
		{
			if(e.code == SQLITE_CONSTRAINT)
			{
				continue;
			}
			else 
			{
				throw (e);
			}
		}
	}
}

void cutestArtists (Database& db)
{
	const auto search = db.SELECT<std::string, double>("artist, (mu - sigma * 3) * min(10, COUNT(*)) as power" 
														" FROM artistScore JOIN image_artist_bridge USING (artist)" 
														" GROUP BY artist ORDER BY power");

	for(const auto& [artist, power] : search)
	{
		std::cout << artist << ' ' << power << std::endl;
	}
}

int main (void)
{
	//in memory database so its new everytime
	Database db ("test");
	buildDatabases(db);

	//batch into a transaction which is much much faster!
	auto t = db.transaction();
	insertTags(db, "../booru.csv");
	insertTagScores(db, "../booruScores.csv");
	insertArtists(db, "../artists.csv");
	insertArtistScores(db, "../artistScores.csv");

	t.commit();;

	cutestArtists(db);
}
