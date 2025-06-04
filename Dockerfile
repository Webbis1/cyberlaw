FROM ubuntu:22.04

WORKDIR /app


RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential g++ cmake \
    libcurl4-openssl-dev \
    libxml2-dev \
    libjsoncpp-dev \
    default-libmysqlclient-dev \
    pkg-config \
    cron \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && \
    apt-get install -y g++ make curl libcurl4-openssl-dev libxml2-dev libmysqlclient-dev libjsoncpp-dev pkg-config

COPY . .

RUN g++ -std=c++17 -o main main.cpp $(pkg-config --cflags --libs libxml-2.0) -lcurl -ljsoncpp -lmysqlclient

RUN chmod +x /app/run.sh && crontab /app/crontab.txt

CMD ["cron", "-f"]
