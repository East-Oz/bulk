#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <time.h>
#include <chrono>
#include <ctime>
#include <iostream>
#include <fstream>

int gRowCount = 1;

class Observer
{
public:
	virtual void execute( std::vector<std::string>*, time_t* ) = 0;
	virtual ~Observer() = default;
};

class Executor
{
public:
	Executor(): m_fct(time(0)){};

	std::vector<std::string> m_commands;
	time_t m_fct; // first command time
private:
	std::vector<std::shared_ptr<Observer>> m_subscribers;

public:
	void subscribe( std::shared_ptr<Observer> ptrObs )
	{
		m_subscribers.push_back( ptrObs );
	}

	void set_commands( std::vector<std::string> commands, time_t fct )
	{
		m_commands = commands;
		m_fct = fct;
		execute();
	}

	void execute()
	{
		for( auto s : m_subscribers )
		{
			s->execute( &m_commands, &m_fct );
		}
	}

};

class FileObserver: public Observer
{
public:
	FileObserver( std::shared_ptr<Executor> ptrExecutor )
	{
		auto wptr = std::shared_ptr<FileObserver>( this, []( FileObserver* ) {} );
		ptrExecutor->subscribe( wptr );
	}

	void execute( std::vector<std::string>* commands, time_t* fct ) override
	{
		if( commands->size() > 0 )
		{
			struct tm  tstruct;
			char       buf[ 80 ];
			tstruct = *localtime( fct );
			strftime( buf, sizeof( buf ), "%OH%OM%OS", &tstruct );

			auto time_now = std::chrono::system_clock::now();
			auto ms = std::chrono::duration_cast< std::chrono::milliseconds >(time_now.time_since_epoch()) % 1000;

			std::string fn = "bulk";
			fn.append( buf );
			fn.append( std::to_string( ms.count() ) );
			fn.append( ".log" );

			std::ofstream myfile;
			myfile.open( fn );
			if( myfile.is_open() )
			{
				for( size_t i = 0; i < commands->size(); ++i )
				{
					myfile << commands->at( i );
					if( i != (commands->size() - 1) )
						myfile << ", ";
					else
						myfile << std::endl;
				}
				myfile.close();
			}
		}

	}
};

class ConsoleObserver: public Observer
{
public:
	ConsoleObserver( std::shared_ptr<Executor> ptrExecutor )
	{
		auto wptr = std::shared_ptr<ConsoleObserver>( this, []( ConsoleObserver* ) {} );
		ptrExecutor->subscribe( wptr );
	}

	void execute( std::vector<std::string>* commands, time_t* fct ) override
	{
		if( commands->size() > 0 )
		{
			std::cout << "bulk: ";
			for( size_t i = 0; i < commands->size(); ++i )
			{
				std::cout << commands->at( i );
				if( i < (commands->size() - 1) )
					std::cout << ", ";
				else
					std::cout << std::endl;
			}
		}
	}
};

class Parser
{
public:
	Parser( std::shared_ptr<Executor> ptrExec ): m_pExecutor( ptrExec ) {};
	void Start()
	{
		if( m_pExecutor == nullptr )
		{
			std::cout << "ERROR Executor is null! Return";
			return;
		}

		int open_braces = 0;
		bool is_ready_data = false;

		std::vector<std::string> vector_str;
		int count = 1;
		time_t fct = time( 0 );
		for( std::string line; std::getline( std::cin, line );)
		{
			if( count == 1 )
			{
				fct = time( 0 );
			}

			if( line.empty() ) // for exit sequence from console
			{
				is_ready_data = true;
			}
			else if( line.find( '{' ) != std::string::npos )
			{
				++open_braces;
				if( open_braces == 1 )
				{
					is_ready_data = true;
				}
			}
			else if( (line.find( '}' ) != std::string::npos) && (open_braces > 0) )
			{
				--open_braces;
				if( open_braces == 0 )
				{
					is_ready_data = true;
				}
			}
			else if( (count == gRowCount) && (open_braces == 0) )
			{
				vector_str.push_back( line );
				is_ready_data = true;
			}
			else
			{
				vector_str.push_back( line );
			}

			if( is_ready_data )
			{
				m_pExecutor->set_commands( vector_str, fct );
				vector_str.clear();
				is_ready_data = false;
				count = 0;
			}
			count++;
		}
		m_pExecutor->set_commands( vector_str, fct );
		vector_str.clear();
	}

private:
	std::shared_ptr<Executor> m_pExecutor;
};

int main( int argc, char *argv[] )
{
	if( argc >= 2 )
		gRowCount = std::stoi( argv[ 1 ] );

	std::shared_ptr<Executor> ptrExec( std::make_shared<Executor>( ) );

	FileObserver File( ptrExec );
	ConsoleObserver Console( ptrExec );

	Parser ParserWorker( ptrExec );
	ParserWorker.Start();

    return 0;
}

