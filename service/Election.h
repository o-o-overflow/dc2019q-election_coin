#ifndef DC2019Q_ELECTION_COIN_ELECTION_H
#define DC2019Q_ELECTION_COIN_ELECTION_H

#include <chrono>
#include <string>
#include <utility>

#include "Vote.h"
#include "Ballot.h"
#include "Candidate.h"

class Election {
public:
    Election redactTallies() {
        auto redacted = *this;
        for (auto& r_it : redacted.races_) {
            std::vector<Candidate> candidates;
            for (auto& c_it : r_it.second) {
                candidates.emplace_back(c_it.redactTallies());
            }

            r_it.second = candidates;
        }

        return redacted;
    }

    void postBallot(const Ballot& ballot) {
        for (const auto& v_it : ballot.votes_) {
            auto r_it = races_.find(v_it.first);
            if (r_it == std::end(races_)) {
                throw std::runtime_error("unknown race");
            }

            auto& race = r_it->second;
            auto c_it = std::find_if(std::begin(race),
                                     std::end(race),
                                     [&](Candidate& c) {
                                         return v_it.second.candidate_ == c.name_;
                                     });
            if (c_it == std::end(race)) {
                throw std::runtime_error("unknown candidate");
            }

            c_it->postVote(v_it.second);
        }
    }

    std::string name_;
    std::unordered_map<std::string, std::vector<Candidate>> races_;
    std::chrono::system_clock::time_point start_timestamp_;
    std::chrono::hours duration_;
};

void to_json(json& value, const Election& election) {
    value = json{
            {"name",            election.name_},
            {"races",           election.races_},
            {"start_timestamp", std::chrono::duration_cast<std::chrono::seconds>(election.start_timestamp_
                                                                                         .time_since_epoch())
                                        .count()},
            {"duration",        election.duration_.count()},
    };
}

void from_json(const json& value, Election& election) {
    value.at("name").get_to(election.name_);
    value.at("races").get_to(election.races_);

    std::size_t tmp = 0;
    value.at("start_timestamp").get_to(tmp);
    election.start_timestamp_ = std::chrono::system_clock::from_time_t(tmp);
    value.at("duration").get_to(tmp);
    election.duration_ = std::chrono::hours(tmp);
}

#endif // DC2019Q_ELECTION_COIN_ELECTION_H
