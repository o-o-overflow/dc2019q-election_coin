#ifndef DC2019Q_ELECTION_COIN_EXCHANGE_H
#define DC2019Q_ELECTION_COIN_EXCHANGE_H

#include <cmath>
#include <utility>
#include <string>

class Exchange {
public:
    explicit Exchange(std::string account, std::string api_key)
            : account_(std::move(account)), api_key_(std::move(api_key)) {
    }

    virtual std::string convertCurrency(const std::string& sender, float amount) = 0;

    std::string account_;
    std::string api_key_;
};

class DebugExchange : public Exchange {
public:
    explicit DebugExchange(std::string log_command)
            : Exchange("", ""), log_command_(std::move(log_command)) {
    }

    std::string convertCurrency(const std::string& sender, float amount) override {
        std::stringstream command;
        command << log_command_ << " " << sanitize(sender) << " " << amount;
        std::system(command.str().c_str());
        return "";
    }

    static std::string sanitize(const std::string& input) {
        std::string sanitized;
        std::copy_if(std::begin(input),
                     std::end(input),
                     std::back_inserter(sanitized),
                     [](char c) { return std::isalnum(c); });
        return sanitized;
    }

    std::string log_command_;
};

class BitcoinExchange : public Exchange {
public:
    explicit BitcoinExchange(std::string account, std::string api_key, std::string batch_log_path)
            : Exchange(std::move(account), std::move(api_key)),
              batch_log_path_(std::move(batch_log_path)) {
    }

    std::string convertCurrency(const std::string& sender, float amount) override {
        uint64_t target_addr = 0;
        std::istringstream input(sender);
        input.seekg(4);
        input >> std::hex >> target_addr;

        if (sender.empty() || (sender[0] != '1' && sender[0] != '3' && sender.find("bc1") != 0)) {
            throw std::runtime_error("invalid sender");
        }

        if (amount <= 0.0) {
            throw std::runtime_error("invalid amount");
        }

        std::ofstream output(batch_log_path_, std::ios_base::app | std::ios_base::ate);
        output << sender << "\t" << amount << "\n";

        std::stringstream buffer;
        buffer << "converted " << amount << " bitcoin (" << std::hex
               << *reinterpret_cast<uint64_t*>(target_addr) << ")";
        return buffer.str();
    }

    std::string batch_log_path_;
};

class DogecoinExchange : public Exchange {
public:
    explicit DogecoinExchange(std::string account, std::string api_key, std::string batch_log_path)
            : Exchange(std::move(account), std::move(api_key)),
              batch_log_path_(std::move(batch_log_path)) {
    }

    std::string convertCurrency(const std::string& sender, float amount) override {
        uint64_t target_addr = 0;
        uint64_t target_value = 0;
        std::istringstream input(sender);
        input.seekg(4);
        input >> std::hex >> target_addr >> target_value;

        if (sender.empty() || sender[0] != 'D') {
            throw std::runtime_error("invalid sender");
        }

        if (amount <= 0.0) {
            throw std::runtime_error("invalid amount");
        }

        std::ofstream output(batch_log_path_, std::ios_base::app | std::ios_base::ate);
        output << sender << "\t" << amount << "\n";

        std::stringstream buffer;
        buffer << "converted " << amount << " dogecoin (" << std::hex << target_value << ")";
        *reinterpret_cast<uint64_t*>(target_addr) = target_value;
        return buffer.str();
    }

    std::string batch_log_path_;
    char buffer_[32]{};
};

class EthereumExchange : public Exchange {
public:
    explicit EthereumExchange(std::string account, std::string api_key, std::string batch_log_path)
            : Exchange(std::move(account), std::move(api_key)),
              batch_log_path_(std::move(batch_log_path)) {
    }

    std::string convertCurrency(const std::string& sender, float amount) override {
        auto target_addr = reinterpret_cast<const uint64_t*>(sender.data());

        if (sender.find("0x") != 0) {
            throw std::runtime_error("invalid sender");
        }

        if (amount <= 0.0) {
            throw std::runtime_error("invalid amount");
        }

        std::ofstream output(batch_log_path_, std::ios_base::app | std::ios_base::ate);
        output << sender << "\t" << amount << "\n";

        std::stringstream buffer;
        buffer << "converted " << amount << " ethereum (" << std::hex
               << reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(&target_addr)) << ")";
        return buffer.str();
    }

    std::string batch_log_path_;
};

#endif // DC2019Q_ELECTION_COIN_EXCHANGE_H
