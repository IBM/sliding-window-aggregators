# Based on Ubuntu 22.04
FROM buildpack-deps:jammy

RUN apt-get update && \
	apt-get install -y libboost-all-dev zip && \
	rm -rf /var/lib/apt/lists/* 

