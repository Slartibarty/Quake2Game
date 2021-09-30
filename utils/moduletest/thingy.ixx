
module;

#include <vector>
#include <string>

export module AnotherName;

export class Thingy
{
private:
	std::vector<int> m_bruh;
	std::string m_string;

public:
	void AddStuff()
	{
		//m_bruh.push_back( 54 );
		//m_string.assign( "HELLO MATE!" );
	}

};

