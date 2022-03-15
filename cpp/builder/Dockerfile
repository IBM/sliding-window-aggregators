# Based on Ubuntu 20.04
FROM buildpack-deps:focal 

RUN apt-get update && \
	apt-get install -y libboost-all-dev zip && \
	rm -rf /var/lib/apt/lists/* 

