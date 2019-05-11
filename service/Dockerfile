FROM ubuntu:19.04
EXPOSE 8888
RUN apt-get update && apt-get -y upgrade
RUN apt-get -y install build-essential cmake clang cppcheck
ADD json /src/json/
ADD pistache /src/pistache/
ADD spdlog /src/spdlog/
ADD *.h *.cpp *.json *.txt cfi_blacklist flag /src/
WORKDIR /build
RUN cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ /src && make && strip election_coin
WORKDIR /
RUN cp /build/election_coin /src/election_coin.json /src/flag / && rm -rf /src /build
ADD run.sh /
RUN groupadd -r election && useradd --no-log-init -r -g election election
USER election
CMD /run.sh
