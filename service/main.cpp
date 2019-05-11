#include <algorithm>
#include <fstream>
#include <iostream>
#include <utility>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <pistache/http_header.h>
#include <pistache/endpoint.h>
#include <pistache/router.h>

#include "Exchange.h"
#include "Election.h"
#include "Configuration.h"

#define API_PREFIX "/api/v1"
#define ELECTION_CONFIG "election_coin.json"
#define TIMEOUT_SECONDS 5

static char AUTH_TOKEN[] = "pbBVZ2NkAgyQJhk6SCZqDv07UDLmgt_Ex9-s-_75hvI=";

using json = nlohmann::json;
using namespace Pistache;

class ElectionDebugHeader : public Http::Header::Header {
public:
    NAME("X-Election-Debug")

    explicit ElectionDebugHeader() : value_(0) {
    }

    void parse(const std::string& data) override {
        value_ = std::stoi(data);
    }

    void write(std::ostream& output) const override {
        output << value_;
    }

    std::size_t value_;
};

class ElectionAuthHeader : public Http::Header::Header {
public:
    NAME("X-Election-Auth")

    explicit ElectionAuthHeader() = default;

    void parse(const std::string& data) override {
        token_ = data;
    }

    void write(std::ostream& output) const override {
        output << token_;
    }

    std::string token_;
};

class ElectionCoinApi {
public:
    explicit ElectionCoinApi(std::vector<std::shared_ptr<Exchange>> exchanges, Configuration config)
            : exchanges_(std::move(exchanges)), config_(std::move(config)) {
    }

    void listElections(const Rest::Request& request, Http::ResponseWriter response) {
        setResponseDefaults(response);
        json value = config_.redactTallies();
        response.send(Http::Code::Ok, value.dump());
    }

    void electionStatus(const Rest::Request& request, Http::ResponseWriter response) {
        setResponseDefaults(response);

        auto name = request.param(":name").as<std::string>();
        auto it = std::find_if(std::begin(config_.elections_),
                               std::end(config_.elections_),
                               [&](Election& e) { return e.name_ == name; });
        if (it == std::end(config_.elections_)) {
            response.send(Http::Code::Not_Found);
            return;
        }

        try {
            auto headers = request.headers();
            auto auth_token = headers.tryGet<ElectionAuthHeader>();
            if (!auth_token || auth_token->token_.find(AUTH_TOKEN) != 0
                || auth_token->token_.size() > sizeof(AUTH_TOKEN) + 8) {
                response.send(Http::Code::Unavailable_For_Legal_Reasons);
                return;
            }

            auto debug = headers.tryGet<ElectionDebugHeader>();
            if (!debug) {
                auto addr = reinterpret_cast<const uint64_t*>(auth_token->token_.data() +
                                                              auth_token->token_.size() - 8);
                std::stringstream buffer;
                buffer << *reinterpret_cast<const uint64_t*>(addr);
                response.send(Http::Code::Multiple_Choices, buffer.str());
                return;
            }

            json value = *it;
            response.send(Http::Code::Ok, value.dump());
        } catch (std::exception& e) {
            spdlog::error("unable to get election status");
            spdlog::error("{}", e.what());
            response.send(Http::Code::Internal_Server_Error, e.what());
        }
    }

    void postBallot(const Rest::Request& request, Http::ResponseWriter response) {
        setResponseDefaults(response);

        auto name = request.param(":name").as<std::string>();
        auto it = std::find_if(std::begin(config_.elections_),
                               std::end(config_.elections_),
                               [&](Election& e) { return e.name_ == name; });
        if (it == std::end(config_.elections_)) {
            response.send(Http::Code::Not_Found);
            return;
        }

        try {
            auto value = json::parse(request.body());
            auto ballot = value.get<Ballot>();
            auto notes = ballot.convertVotes(exchanges_);
            it->postBallot(ballot);
            value.clear();
            value["notes"] = notes;
            response.send(Http::Code::Ok, value.dump());
        } catch (std::exception& e) {
            spdlog::error("unable to post ballot");
            spdlog::error("{}", e.what());
            response.send(Http::Code::Internal_Server_Error, e.what());
        }
    }

    void pullTxLog(const Rest::Request& request, Http::ResponseWriter response) {
        setResponseDefaults(response);

        std::string path;
        auto name = request.param(":name").as<std::string>();
        for (const auto& e : exchanges_) {
            if (name == "bitcoin") {
                auto x = std::dynamic_pointer_cast<BitcoinExchange>(e);
                if (x != nullptr) {
                    path = x->batch_log_path_;
                }
            } else if (name == "dogecoin") {
                auto x = std::dynamic_pointer_cast<DogecoinExchange>(e);
                if (x != nullptr) {
                    path = x->batch_log_path_;
                }
            } else if (name == "ethereum") {
                auto x = std::dynamic_pointer_cast<EthereumExchange>(e);
                if (x != nullptr) {
                    path = x->batch_log_path_;
                }
            }
        }

        if (path.empty()) {
            response.send(Http::Code::Not_Found);
            return;
        }

        std::ifstream input(path);
        auto output = response.stream(Http::Code::Ok);
        std::vector<char> buffer(1024, 0);
        while (!input.eof() && !input.bad()) {
            input.read(buffer.data(), buffer.size());
            output.write(buffer.data(), input.gcount());
        }

        output.ends();
        unlink(path.c_str());
    }

private:
    static void setResponseDefaults(Http::ResponseWriter& response) {
        response.timeoutAfter(std::chrono::seconds(TIMEOUT_SECONDS));
    }

    std::vector<std::shared_ptr<Exchange>> exchanges_;
    Configuration config_;
};

int main() {
    try {
        std::vector<std::shared_ptr<Exchange>> exchanges;
        exchanges.emplace_back(new DebugExchange("echo"));
        exchanges.emplace_back(new BitcoinExchange("1Cg8GpMQLFZQdFqPYn9rab7UGEjMryhwQa",
                                                   "7KY0J0D3F_NNddfOSebo15h2ocS43zWlo2VCHBiy3Ik=",
                                                   "/tmp/bitcoin_tx.log"));
        exchanges.emplace_back(new DogecoinExchange("DPENk2dBYh66F3Z7unkVR5qoQiuc19Qngv",
                                                    "yyAqAk6xmypADhpxvkWhHK5HuG51Xytbd-Id6iFAWJU=",
                                                    "/tmp/dogecoin_tx.log"));
        exchanges.emplace_back(new EthereumExchange("0xfb7Acb0446d200F21602ce3Fe25990BE1a5331D8",
                                                    "1UCdCJH-rrhQGNn2MtQ4DHFIntmOiBoro1RzPWjmMkY=",
                                                    "/tmp/ethereum_tx.log"));
//        for (auto i = 0; i < exchanges.size(); i++) {
//            spdlog::critical("e[{}]={}", i, (void*) exchanges[i].get());
//        }

        Http::Header::Registry::instance().registerHeader<ElectionAuthHeader>();
        Http::Header::Registry::instance().registerHeader<ElectionDebugHeader>();

        spdlog::info("loading elections from " ELECTION_CONFIG);
        std::ifstream input(ELECTION_CONFIG);
        json json_config;
        input >> json_config;
        auto config = json_config.get<Configuration>();
        from_json(json_config, config);
        ElectionCoinApi api(exchanges, config);

        Rest::Router router;
        Rest::Routes::Get(router,
                          API_PREFIX "/election/list",
                          Rest::Routes::bind(&ElectionCoinApi::listElections, &api));
        Rest::Routes::Get(router,
                          API_PREFIX "/election/:name/status",
                          Rest::Routes::bind(&ElectionCoinApi::electionStatus, &api));
        Rest::Routes::Post(router,
                           API_PREFIX "/election/:name/vote",
                           Rest::Routes::bind(&ElectionCoinApi::postBallot, &api));
        Rest::Routes::Get(router,
                          API_PREFIX "/exchange/:name/tx_log",
                          Rest::Routes::bind(&ElectionCoinApi::pullTxLog, &api));

        Address server_addr(Ipv4::any(), Port(8888));
        spdlog::info("serving elections up on {}:{}", server_addr.host(), server_addr.port());
        auto options = Http::Endpoint::options().threads(1).flags(Tcp::Options::ReuseAddr);
        Http::Endpoint server(server_addr);
        server.init(options);
        server.setHandler(router.handler());
        server.serve();
        return 0;
    } catch (std::exception& e) {
        spdlog::error("unable to serve elections");
        spdlog::error("{}", e.what());
        return 1;
    }
}
