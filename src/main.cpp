#include <iostream>
#include <memory>
#include <string>
#include <vector>

constexpr int UNKNOWN_OPT = 1;
constexpr int NOARGS_RET = 2;
constexpr int INTERNAL_ERR_RET = 166;

enum class Scenario { NoArgs, Help, Other };

class Command {
public:
	Command(const std::vector<std::string> &argv) : options_(argv) {}

	int handle_command() {
		Scenario scenario = Scenario::NoArgs;

		for (const std::string &s : options_) {
			if (s == "-h" || s == "--help") {
				scenario = Scenario::Help;
			} else {
				scenario = Scenario::Other;

				print_other(s);
				break;
			}
		}

		switch (scenario) {
			case Scenario::Help:
				print_help();
				return 0;
			case Scenario::NoArgs:
				print_one_line_hint();
				return NOARGS_RET;
			case Scenario::Other:
				return UNKNOWN_OPT;
		}

		/* This should never happen. Added for compiler. */
		return INTERNAL_ERR_RET;
	}

private:
	void print_help() const {
		std::cout << "Usage: pwlck [options]\n"
		             "Options:\n"
		             "  -h, --help    Show this help and exit\n";
	}

	void print_one_line_hint() const {
		std::cerr << "Usage: pwlck [options]. Try 'pwlck --help'\n";
	}

	void print_other(const std::string &s) const {
		std::cerr << "Unknown option: \"" << s << "\".\n";
	}

	std::vector<std::string> options_;
};

int main(int argc, char *argv[]) {
	std::vector<std::string> args(argv + 1, argv + argc);
	auto command_ptr = std::make_unique<Command>(args);

	return command_ptr->handle_command();
}
