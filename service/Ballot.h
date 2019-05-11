#include <string>
#include <unordered_map>

#include "Vote.h"
#include "Exchange.h"

#ifndef DC2019Q_ELECTION_COIN_BALLOT_H
#define DC2019Q_ELECTION_COIN_BALLOT_H

class Ballot {
public:
    void convertVotes(std::vector<std::shared_ptr<Exchange>>& exchanges) {
        for (auto& v_it : votes_) {
            auto e_it = std::find_if(std::begin(exchanges),
                                     std::end(exchanges),
                                     [&](std::shared_ptr<Exchange>& x) {
                                         return (v_it.second.currency_ == "bitcoin" &&
                                                 std::dynamic_pointer_cast<BitcoinExchange>(x))
                                                || (v_it.second.currency_ == "dogecoin" &&
                                                    std::dynamic_pointer_cast<DogecoinExchange>(x))
                                                || (v_it.second.currency_ == "ethereum" &&
                                                    std::dynamic_pointer_cast<EthereumExchange>(x));
                                     });
            if (e_it != std::end(exchanges)) {
                (*e_it)->convertCurrency(voter_, v_it.second.amount_);
            }
        }
    }

    std::string voter_;
    std::string election_;
    std::unordered_map<std::string, Vote> votes_;
};

void to_json(json& value, const Ballot& ballot) {
    value = json{
            {"voter",    ballot.voter_},
            {"election", ballot.election_},
            {"votes",    ballot.votes_}
    };
}

void from_json(const json& value, Ballot& ballot) {
    value.at("voter").get_to(ballot.voter_);
    value.at("election").get_to(ballot.election_);
    value.at("votes").get_to(ballot.votes_);
}

#endif // DC2019Q_ELECTION_COIN_BALLOT_H
