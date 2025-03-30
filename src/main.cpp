#include <server.h>
#include <memory>
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdlib>

bool is_number(const std::string &s)
{
	return !s.empty() && all_of(s.begin(), s.end(), [](unsigned char c) { return isdigit(c); });
}

void print_help()
{
	std::cout << "Usage: ./program [OPTIONS]\n"
			  << "Options:\n"
			  << "  --help         Show this help message\n"
			  << "  --port=PORT    Set server port number\n";
}

int main(int argc, char **argv)
{
	int bind_port = 5000;
	bool help_requested = false;

	for(int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];

		if(arg == "--help")
		{
			help_requested = true;
		}
		else if(arg.starts_with("--port="))
		{
			size_t pos = arg.find('=');
			if(pos != std::string::npos && pos + 1 < arg.size())
			{
				std::string port_str = arg.substr(pos + 1);
				if(is_number(port_str))
				{
					bind_port = std::stoi(port_str);
				}
				else
				{
					std::cerr << "Error: Invalid port number\n";
					print_help();
					return EXIT_FAILURE;
				}
			}
		}
		else
		{
			std::cerr << "Error: Unknown argument '" << arg << std::endl;
			print_help();
			return EXIT_FAILURE;
		}
	}

	if(help_requested)
	{
		print_help();
		return EXIT_SUCCESS;
	}

	auto server = std::make_unique<Server>(bind_port);
	server->Start();

	return EXIT_SUCCESS;
}