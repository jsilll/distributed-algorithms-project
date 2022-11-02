#include "parser.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <algorithm>
#include <cctype>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cstring>

inline void LeftTrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    [](int ch)
                                    { return !std::isspace(ch); }));
}

inline void RightTrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](int ch)
                         { return !std::isspace(ch); })
                .base(),
            s.end());
}

inline void Trim(std::string &s)
{
    LeftTrim(s);
    RightTrim(s);
}

inline bool IsPositiveNumber(const std::string &s)
{
    return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c)
                                      { return !std::isdigit(c); }) == s.end();
}

Parser::Host::Host(size_t id, std::string &ip_or_hostname, unsigned short port)
    : id{id}, port{htons(port)}
{
    if (IsValidAddress(ip_or_hostname.c_str()))
    {
        ip = inet_addr(ip_or_hostname.c_str());
    }
    else
    {
        ip = IpLookup(ip_or_hostname.c_str());
    }
}

std::string Parser::Host::ip_readable() const
{
    in_addr tmp_ip{};
    tmp_ip.s_addr = ip;
    return {inet_ntoa(tmp_ip)};
}

unsigned short Parser::Host::port_readable() const
{
    return ntohs(port);
}

in_addr_t Parser::Host::IpLookup(const char *host)
{
    struct addrinfo hints
    {
    }, *res;
    char addrstr[128];
    void *ptr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    if (getaddrinfo(host, nullptr, &hints, &res) != 0)
    {
        throw std::runtime_error(
            "Could not resolve host `" + std::string(host) +
            "` to IP: " + std::string(std::strerror(errno)));
    }

    while (res)
    {
        inet_ntop(res->ai_family, res->ai_addr->sa_data, addrstr, 128);

        switch (res->ai_family)
        {
        case AF_INET:
            ptr =
                &(reinterpret_cast<struct sockaddr_in *>(res->ai_addr))->sin_addr;
            inet_ntop(res->ai_family, ptr, addrstr, 128);
            return inet_addr(addrstr);
            break;
        // case AF_INET6:
        //     ptr = &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr;
        //     break;
        default:
            break;
        }
        res = res->ai_next;
    }

    throw std::runtime_error("No host resolves to IPv4");
}

bool Parser::Host::IsValidAddress(const char *ipAddress)
{
    struct sockaddr_in sa
    {
    };
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}

Parser::Parser(const int argc, char const *const *argv, bool rc)
    : argc_{argc}, argv_{argv}, requires_config_{rc}, parsed_{false}
{
}

void Parser::Parse()
{
    if (!ParseInternal())
    {
        Help(argc_, argv_);
    }

    parsed_ = true;

    ParseHostsFile();

    if (requires_config_)
    {
        ParseConfigFile();
    }
}

bool Parser::requires_config() const noexcept
{
    return requires_config_;
}

std::string Parser::config_path() const
{
    CheckParsed();

    if (!requires_config_)
    {
        throw std::runtime_error("Parser is configured to ignore the config path");
    }

    return config_path_;
}

std::string Parser::hosts_path() const
{
    CheckParsed();
    return hosts_path_;
}

std::string Parser::output_path() const
{
    CheckParsed();
    return output_path_;
}

std::vector<Parser::Host> Parser::hosts() const
{
    CheckParsed();
    return hosts_;
}

unsigned long Parser::id() const
{
    CheckParsed();
    return id_;
}

unsigned long Parser::n_messages() const
{
    CheckParsed();
    return n_messages_;
}

unsigned long Parser::target_id() const
{
    CheckParsed();
    return receiver_id_;
}

Parser::ExecMode Parser::exec_mode() const noexcept
{
    return exec_mode_;
}

Parser::Host Parser::local_host() const
{
    if ((id_ - 1) >= hosts_.size())
    {
        throw std::invalid_argument("Id is outside list of hosts.");
    }

    return hosts_[id_ - 1];
}

Parser::Host Parser::target_host() const
{
    if ((receiver_id_ - 1) >= hosts_.size())
    {
        throw std::invalid_argument("Receiver id is outside list of hosts.");
    }

    return hosts_[receiver_id_ - 1];
}

void Parser::CheckParsed() const
{
    if (!parsed_)
    {
        throw std::runtime_error("Invoke Parse() first");
    }
}

void Parser::Help(const int, char const *const *argv) const
{
    std::string program_name(argv[0]);
    if (!requires_config_)
    {
        std::string msg = "Usage: " + program_name + " --id ID --hosts HOSTS --output OUTPUT";
        throw std::runtime_error(msg);
    }
    else
    {
        std::string msg = "Usage: " + program_name + " --id ID --hosts HOSTS --output OUTPUT CONFIG";
        throw std::runtime_error(msg);
    }
}

bool Parser::ParseInternal()
{
    if (!ParseId())
    {
        return false;
    }

    if (!ParseHostPath())
    {
        return false;
    }

    if (!ParseOutputPath())
    {
        return false;
    }

    if (!ParseConfigPath())
    {
        return false;
    }

    ParseMode();

    return true;
}

bool Parser::ParseId() noexcept
{
    if (argc_ < 3)
    {
        return false;
    }

    if (std::strcmp(argv_[1], "--id") == 0)
    {
        if (IsPositiveNumber(argv_[2]))
        {
            try
            {
                id_ = std::stoul(argv_[2]);
            }
            catch (std::invalid_argument const &e)
            {
                return false;
            }
            catch (std::out_of_range const &e)
            {
                return false;
            }

            return true;
        }
    }

    return false;
}

void Parser::ParseMode()
{
    if (argc_ < 10)
    {
        return;
    }

    if (std::strcmp(argv_[8], "--mode") == 0)
    {
        if (std::strcmp(argv_[9], "pl") == 0)
        {
            exec_mode_ = kPerfectLinks;
        }
        else if (std::strcmp(argv_[9], "fifo") == 0)
        {
            exec_mode_ = kFIFOBroadcast;
        }
        else
        {
            throw std::runtime_error("Invalid execution mode provided.");
        }
    }
}

bool Parser::ParseHostPath() noexcept
{
    if (argc_ < 5)
    {
        return false;
    }

    if (std::strcmp(argv_[3], "--hosts") == 0)
    {
        hosts_path_ = std::string(argv_[4]);
        return true;
    }

    return false;
}

bool Parser::ParseOutputPath() noexcept
{
    if (argc_ < 7)
    {
        return false;
    }

    if (std::strcmp(argv_[5], "--output") == 0)
    {
        output_path_ = std::string(argv_[6]);
        return true;
    }

    return false;
}

bool Parser::ParseConfigPath() noexcept
{
    if (!requires_config_)
    {
        return true;
    }

    if (argc_ < 8)
    {
        return false;
    }

    config_path_ = std::string(argv_[7]);
    return true;
}

void Parser::ParseHostsFile()
{
    std::ifstream hosts_file(hosts_path());

    if (!hosts_file.is_open())
    {
        std::ostringstream os;
        os << "`" << hosts_path() << "` does not exist.";
        throw std::invalid_argument(os.str());
    }

    int n_lines = 0;
    std::string line;
    while (std::getline(hosts_file, line))
    {
        ++n_lines;

        std::istringstream iss(line);

        Trim(line);
        if (line.empty())
        {
            continue;
        }

        std::string ip;
        unsigned long id;
        unsigned short port;
        if (!(iss >> id >> ip >> port))
        {
            std::ostringstream os;
            os << "Parsing for `" << hosts_path() << "` failed at line " << n_lines;
            throw std::invalid_argument(os.str());
        }

#ifdef DEBUG
        std::cout << "[DEBUG] Reading Host: " << id << " " << ip << " " << port << std::endl;
#endif
        hosts_.emplace_back(id, ip, port);
    }

    if (hosts_.size() < 2UL)
    {
        std::ostringstream os;
        os << "`" << hosts_path() << "` must contain at least two hosts";
        throw std::invalid_argument(os.str());
    }

    auto comp = [](const Host &x, const Host &y)
    { return x.id < y.id; };
    auto result = std::minmax_element(hosts_.begin(), hosts_.end(), comp);
    size_t minID = (*result.first).id;
    size_t maxID = (*result.second).id;
    if (minID != 1UL || maxID != static_cast<unsigned long>(hosts_.size()))
    {
        std::ostringstream os;
        os << "In `" << hosts_path()
           << "` IDs of processes have to start from 1 and be compact";
        throw std::invalid_argument(os.str());
    }

    std::sort(hosts_.begin(), hosts_.end(),
              [](const Host &a, const Host &b) -> bool
              { return a.id < b.id; });

#ifdef DEBUG
    std::cout << "[DEBUG] Hosts Vector" << std::endl;
    std::cout << "[DEBUG] ============" << std::endl;
    for (const auto &host : hosts_)
    {
        std::cout << "[DEBUG] " << host.id << " " << host.ip_readable() << " " << host.port_readable() << std::endl;
    }
#endif
}

void Parser::ParseConfigFile()
{
    std::ifstream config_file(config_path());

    if (!config_file.is_open())
    {
        std::ostringstream os;
        os << "`" << config_path() << "` does not exist.";
        throw std::invalid_argument(os.str());
    }

    std::string line;
    std::getline(config_file, line);
    std::istringstream iss(line);

    Trim(line);
    if (line.empty())
    {
        throw std::runtime_error("Config file is empty.");
    }

    switch (exec_mode_)
    {
    case kPerfectLinks:
        if (!(iss >> n_messages_ >> receiver_id_))
        {
            std::ostringstream os;
            os << "Parsing for `" << config_path() << "` failed at line 1";
            throw std::invalid_argument(os.str());
        }

        break;
    case kFIFOBroadcast:
        if (!(iss >> n_messages_))
        {
            std::ostringstream os;
            os << "Parsing for `" << config_path() << "` failed at line 1";
            throw std::invalid_argument(os.str());
        }
    break;

    default:
        throw std::runtime_error("Invalid execution mode.");
    }
}
