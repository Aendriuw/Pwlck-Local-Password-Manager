#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

constexpr int UNKNOWN_OPT = 1;
constexpr int NOARGS_RET = 2;
constexpr int INTERNAL_ERR_RET = 166;

enum class Scenario { NoArgs, Help, Other };

class Command {
public:
	Command(const std::vector<std::string> &argv) : options_(argv) {}

	/**
	 * Handles a given command.
	 */
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
	/**
	 * Prints help message for `pwlck -h` or `pwlck --help`.
	 */
	void print_help() const {
		std::cout << "Usage: pwlck [options]\n"
		             "Options:\n"
		             "  -h, --help    Show this help and exit\n";
	}

	/**
	 * Prints one_line hint for `pwlck`.
	 */
	void print_one_line_hint() const {
		std::cerr << "Usage: pwlck [options]. Try 'pwlck --help'\n";
	}

	/**
	 * Prints error message for unknown options.
	 */
	void print_other(const std::string &s) const {
		std::cerr << "Unknown option: \"" << s << "\".\n";
	}

	std::vector<std::string> options_;
};

enum class AddRet {
	Ok,
	NoDomain,
	NoUsername,
	NoPassword,
	Duplicate,
	WrongUsernameFmt,
	WrongDomainFmt
};

class EntryList {
public:
	struct Entry {
		std::string username, password;
	};

	/**
	 * Adds an entry to the Entry List.
	 * If the entry was added succesfully it returns AddRet::Ok.
	 * If the entry has an empty field it will return AddRet::NoDomain,
	 * AddRet::NoUsername or AddRet::NoPassword accordingly.
	 *
	 * A domain can only have one entry with a certain name. If in the current's
	 * entry domain there already is one with the same username, it will return
	 * AddRet::Duplicate.
	 *
	 * If a domain has a wrong format (contains white spaces or control
	 * characters) it will return AddRet::WrongDomainFmt. If a username has a
	 * wrong format (starts or ends with white spaces or contains control
	 * characters) it will return AddRet::WrongUsernameFmt.
	 */
	AddRet add_entry(const std::string &domain, const std::string &username,
	                 const std::string &password) {
		if (domain.empty()) {
			return AddRet::NoDomain;
		}

		if (username.empty()) {
			return AddRet::NoUsername;
		}

		if (password.empty()) {
			return AddRet::NoPassword;
		}

		if (!check_username_format(username)) {
			return AddRet::WrongUsernameFmt;
		}

		if (!check_domain_format(domain)) {
			return AddRet::WrongDomainFmt;
		}

		std::string normal_domain = normalise_string(domain);
		Entry aux_entry = {username, password};

		if (accounts_.find(normal_domain) == accounts_.end()) {
			accounts_[normal_domain].push_back(aux_entry);
		} else {
			auto &curr_entries = accounts_[normal_domain];
			auto pos =
			    std::lower_bound(curr_entries.begin(), curr_entries.end(),
			                     aux_entry, [](const Entry &a, const Entry &b) {
				                     return a.username < b.username;
			                     });

			if (pos != curr_entries.end() && pos->username == username) {
				return AddRet::Duplicate;
			}

			curr_entries.insert(pos, aux_entry);
		}

		return AddRet::Ok;
	}

	/**
	 * Prints all current entries' username, grouped by domain.
	 * If there are no entries prints a "No entries in the vault." message.
	 */
	void list_entries() const {
		if (accounts_.empty()) {
			std::cout << "No entries in the vault.\n";
			return;
		}

		for (const auto &[curr_domain, curr_entries] : accounts_) {
			std::cout << "Domain: \"" << curr_domain
			          << "\"\n"
			             "{\n";

			for (const auto &entry : curr_entries) {
				std::cout << std::string(4, ' ') << "username: \""
				          << entry.username << "\"\n";
			}

			std::cout << "}\n";
		}

		std::cout << "\n";
	}

	/**
	 * Returns an array of entries for a domain if it exists, or std::nullopt
	 * otherwise.
	 */
	std::optional<std::vector<Entry>>
	get_entries(const std::string &domain) const {
		std::string normal_domain = normalise_string(domain);
		auto it = accounts_.find(normal_domain);

		if (it == accounts_.end()) {
			return std::nullopt;
		}

		return it->second;
	}

	/**
	 * Returns an entry with a given domain and username if it exists, or
	 * std::nullopt otherwise.
	 */
	std::optional<Entry> get_entry(const std::string &domain,
	                               const std::string &username) const {
		std::optional<std::vector<Entry>> curr_entries = get_entries(domain);

		if (curr_entries == std::nullopt) {
			return std::nullopt;
		}

		for (const auto &entry : *curr_entries) {
			if (entry.username == username) {
				return entry;
			}
		}

		return std::nullopt;
	}

	/**
	 * Attempts to remove an entry specified by domain and username from the
	 * Entry List. Returns true if succesfully removed, or false otherwise.
	 */
	bool remove_entry(const std::string &domain, const std::string &username) {
		std::string normal_domain = normalise_string(domain);
		auto it = accounts_.find(normal_domain);

		if (it == accounts_.end()) {
			return false;
		}

		auto &curr_entries = it->second;

		std::size_t idx = 0;
		for (const auto &entry : curr_entries) {
			if (entry.username == username) {
				curr_entries.erase(curr_entries.begin() + idx);

				if (curr_entries.empty()) {
					accounts_.erase(it);
				}

				return true;
			}

			idx++;
		}

		return false;
	}

private:
	/**
	 * Returns the normalised domain.
	 */
	std::string normalise_string(const std::string &domain) const {
		std::string normal_domain = domain;
		std::transform(normal_domain.begin(), normal_domain.end(),
		               normal_domain.begin(),
		               [](unsigned char c) { return std::tolower(c); });

		return normal_domain;
	}

	/**
	 * Returns true if the given username has a valid format, or false
	 * otherwise.
	 */
	bool check_username_format(const std::string &username) const {
		if (std::isspace(static_cast<unsigned char>(username[0])) ||
		    std::isspace(
		        static_cast<unsigned char>(username[username.size() - 1]))) {
			return false;
		}

		for (unsigned char c : username) {
			if (std::iscntrl(static_cast<unsigned char>(c))) {
				return false;
			}
		}

		return true;
	}

	/**
	 * Returns true if the given domain has a valid format, or false otherwise.
	 */
	bool check_domain_format(const std::string &domain) const {
		for (unsigned char c : domain) {
			if (std::isspace(static_cast<unsigned char>(c)) ||
			    std::iscntrl(static_cast<unsigned char>(c))) {
				return false;
			}
		}

		return true;
	}

	std::map<std::string, std::vector<Entry>> accounts_;
};

int main(int argc, char *argv[]) {
	std::vector<std::string> args(argv + 1, argv + argc);
	auto command_ptr = std::make_unique<Command>(args);

	return command_ptr->handle_command();
}
