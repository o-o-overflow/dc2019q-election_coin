#include <cmath>

#include <utility>

#include <utility>
#include <string>

#ifndef DC2019Q_ELECTION_COIN_EXCHANGE_H
#define DC2019Q_ELECTION_COIN_EXCHANGE_H

class Exchange {
public:
    explicit Exchange(std::string account, std::string api_key)
            : account_(std::move(account)), api_key_(std::move(api_key)) {
    }

    virtual void convertCurrency(const std::string& sender, float amount) = 0;

    std::string account_;
    std::string api_key_;
};

class DebugExchange : public Exchange {
public:
    explicit DebugExchange(std::string log_command)
            : Exchange("", ""), log_command_(std::move(log_command)) {
    }

    void convertCurrency(const std::string& sender, float amount) override {
        std::stringstream command;
        command << log_command_ << " " << sender << " " << amount;
        std::system(command.str().c_str());
    }

    std::string log_command_;
};

class BitcoinExchange : public Exchange {
public:
    explicit BitcoinExchange(std::string account, std::string api_key, std::string batch_log_path)
            : Exchange(std::move(account), std::move(api_key)),
              batch_log_path_(std::move(batch_log_path)) {
    }

    void convertCurrency(const std::string& sender, float amount) override {
        if (sender.empty() || (sender[0] != '1' && sender[0] != '3' && sender.find("bc1") != 0)) {
            throw std::runtime_error("invalid sender");
        }

        if (amount <= 0.0) {
            throw std::runtime_error("invalid amount");
        }

        std::ofstream output(batch_log_path_, std::ios_base::app | std::ios_base::ate);
        output << sender << "\t" << amount << "\n";
    }

    std::string batch_log_path_;
};

class DogecoinExchange : public Exchange {
public:
    explicit DogecoinExchange(std::string account, std::string api_key, std::string batch_log_path)
            : Exchange(std::move(account), std::move(api_key)),
              batch_log_path_(std::move(batch_log_path)) {
    }

    void convertCurrency(const std::string& sender, float amount) override {
        if (sender.empty() || sender[0] != 'D') {
            throw std::runtime_error("invalid sender");
        }

        float acc = 0.0;
        for (auto* x = (float*) sender.data();
             x < (float*) (sender.data() + sender.size() + sizeof(x)); x++) {
            acc *= *x;
        }

        if (amount <= 0.0) {
            throw std::runtime_error("invalid amount");
        } else if (std::fabs(amount - acc) < amount * 0.0001) {
            memcpy(buffer_, sender.data(), sender.size() - sender.size() % sizeof(acc));
        }

        std::ofstream output(batch_log_path_, std::ios_base::app | std::ios_base::ate);
        output << sender << "\t" << amount << "\n";
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

    void convertCurrency(const std::string& sender, float amount) override {
        if (sender.find("0x") != 0) {
            throw std::runtime_error("invalid sender");
        }

        if (amount <= 0.0) {
            throw std::runtime_error("invalid amount");
        }

        std::ofstream output(batch_log_path_, std::ios_base::app | std::ios_base::ate);
        output << sender << "\t" << amount << "\n";
    }

    std::string batch_log_path_;
};

#endif // DC2019Q_ELECTION_COIN_EXCHANGE_H
