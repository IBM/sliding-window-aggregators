#ifndef __DATA_GENERATORS_H__
#define __DATA_GENERATORS_H__

#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <boost/tokenizer.hpp>

#include "utils.h"

typedef boost::tokenizer<boost::escaped_list_separator<char>> tokenizer;

template <typename T>
void assign_consume(T& value, tokenizer::iterator& curr) {
    value = from_string<T>(*curr);
    ++curr;
}

class MfgDEBS {
public:
    typedef typename std::chrono::system_clock::time_point timestamp;

    explicit operator timestamp() const {
        return ts;
    }

    timestamp order() const {
        return ts;
    }

    // Note: the following conversions are not really sensical at the moment.
    explicit operator int() const {
        return mf01;
    }

    explicit operator int64_t() const {
        return mf01;
    }

    explicit operator std::size_t() const {
        return index;
    }

    explicit operator double() const {
        return mf02;
    }

    void parse(std::string line) {
        // Note that this file uses tabs as separators, so we can't use the default 
        // separator object.
        tokenizer tokens(line, boost::escaped_list_separator<char>('\\', '\t', '\"'));
        tokenizer::iterator curr = tokens.begin();

        assign_consume(ts, curr);
        assign_consume(index, curr);
        assign_consume(mf01, curr);
        assign_consume(mf02, curr);
        assign_consume(mf03, curr);
        assign_consume(pc13, curr);
        assign_consume(pc14, curr);
        assign_consume(pc15, curr);
        assign_consume(pc25, curr);
        assign_consume(pc26, curr);
        assign_consume(pc27, curr);
        assign_consume(res, curr);
        assign_consume(bm05, curr);
        assign_consume(bm06, curr);
        assign_consume(bm07, curr);
        assign_consume(bm08, curr);
        assign_consume(bm09, curr);
        assign_consume(bm10, curr);

        // the following values are optional, so we have to always test for whether or not 
        // we hit the end of the line
        while (curr != tokens.end()) {
            pp.push_back(uint8_t());
            assign_consume(pp.back(), curr);
        }
    }

    friend std::ostream& operator<<(std::ostream& out, const MfgDEBS& m) {
        out << m.ts << " "
            << m.index << " "
            << m.mf01 << " "
            << m.mf02 << " "
            << m.mf03 << " "
            << m.pc13 << " "
            << m.pc14 << " "
            << m.pc15 << " "
            << m.pc25 << " "
            << m.pc26 << " "
            << m.pc27 << " "
            << m.res << " ";

        for (auto it = m.pp.begin(); it != m.pp.end(); ++it) {
            out << (*it ? "1 " : "0 ");
        }

        return out;
    }

    static constexpr int skip_lines() {
        return 0;
    }

private:
    timestamp ts;
    int64_t index;
    int32_t mf01;
    int32_t mf02;
    int32_t mf03;
    int32_t pc13;
    int32_t pc14;
    int32_t pc15;
    uint32_t pc25;
    uint32_t pc26;
    uint32_t pc27;
    uint32_t res;
    bool bm05;
    bool bm06;
    bool bm07;
    bool bm08;
    bool bm09;
    bool bm10;
    std::vector<uint8_t> pp;
};

class CitiBikeCsv {
public:
    typedef typename std::chrono::system_clock::time_point timestamp;

    CitiBikeCsv()
    {}

    explicit operator timestamp() const {
        return starttime;
    }

    explicit operator int() const {
        return tripduration;
    }

    explicit operator int64_t() const {
        return tripduration;
    }

    explicit operator std::size_t() const {
        return start_station_id;
    }

    explicit operator double() const {
        return tripduration;
    }

    void parse(std::string line) {
        tokenizer tokens(line);
        tokenizer::iterator curr = tokens.begin();

        assign_consume(tripduration, curr);
        assign_consume(starttime, curr);
        assign_consume(stoptime, curr);
        assign_consume(start_station_id, curr);
        assign_consume(start_station_name, curr);
        assign_consume(start_station_lat, curr);
        assign_consume(start_station_long, curr);
        assign_consume(end_station_id, curr);
        assign_consume(end_station_name, curr);
        assign_consume(end_station_lat, curr);
        assign_consume(end_station_long, curr);
        assign_consume(bike_id, curr);
        assign_consume(usertype, curr);
        assign_consume(birth_year, curr);
        assign_consume(gender, curr);
    }

    friend std::ostream& operator<<(std::ostream& out, const CitiBikeCsv& bike) {
        out << bike.tripduration << ","
            << bike.starttime << ","
            << bike.stoptime << ","
            << bike.start_station_id << ","
            << bike.start_station_name << ","
            << bike.start_station_lat << ","
            << bike.start_station_long << ","
            << bike.end_station_id << ","
            << bike.end_station_name << ","
            << bike.end_station_lat << ","
            << bike.end_station_long << ","
            << bike.bike_id << ","
            << bike.usertype << ","
            << bike.birth_year << ","
            << bike.gender;
        return out;
    }

    timestamp order() const {
        return starttime;
    }

    friend bool operator>(const CitiBikeCsv& a, const CitiBikeCsv& b) {
        return a.starttime > b.starttime;
    }

    friend bool operator==(const CitiBikeCsv& a, const CitiBikeCsv& b) {
        return a.tripduration == b.tripduration &&
               a.starttime == b.starttime &&
               a.stoptime == b.stoptime &&
               a.start_station_id == b.start_station_id &&
               a.start_station_name == b.start_station_name &&
               a.start_station_lat == b.start_station_lat &&
               a.start_station_long == b.start_station_long &&
               a.end_station_id == b.end_station_id &&
               a.end_station_name == b.end_station_name &&
               a.end_station_lat == b.end_station_lat &&
               a.end_station_long == b.end_station_long &&
               a.bike_id == b.bike_id &&
               a.usertype == b.usertype &&
               a.birth_year == b.birth_year &&
               a.gender == b.gender;
    }

    friend bool operator!=(const CitiBikeCsv& a, const CitiBikeCsv& b) {
        return !(a == b);
    }

    CitiBikeCsv& operator+=(const CitiBikeCsv& other) {
        // This is non-sensical; it has no real meaning. I just implemented a cheap 
        // operation to force data usage during queries.
        tripduration += other.tripduration; 
        return *this;
    }

    static const CitiBikeCsv identity;
    static constexpr int skip_lines() {
        return 1;
    }

private:
    int tripduration;
    timestamp starttime;
    timestamp stoptime;
    int start_station_id;
    std::string start_station_name;
    double start_station_lat;
    double start_station_long;
    int end_station_id;
    std::string end_station_name;
    double end_station_lat;
    double end_station_long;
    int bike_id;
    std::string usertype;
    int birth_year;
    int gender;
};

const CitiBikeCsv CitiBikeCsv::identity;

class YellowTaxiCsv {
public:
    typedef typename std::chrono::system_clock::time_point timestamp;

    YellowTaxiCsv()
    {}

    explicit operator timestamp() const {
        return dropoff_datetime;
    }

    explicit operator int() const {
        return passenger_count;
    }

    explicit operator int64_t() const {
        return passenger_count;
    }

    explicit operator std::size_t() const {
        return pickup_location_id;
    }

    explicit operator double() const {
        return trip_distance;
    }

    void parse(std::string line) {
        tokenizer tokens(line);
        tokenizer::iterator curr = tokens.begin();

        assign_consume(vendor_id, curr);
        assign_consume(pickup_datetime, curr);
        assign_consume(dropoff_datetime, curr);
        assign_consume(passenger_count, curr);
        assign_consume(trip_distance, curr);
        assign_consume(rate_code, curr);
        assign_consume(store_and_fwd_flag, curr);
        assign_consume(pickup_location_id, curr);
        assign_consume(dropoff_location_id, curr);
        assign_consume(payment_type, curr);
        assign_consume(fare_amount, curr);
        assign_consume(extra, curr);
        assign_consume(mta_tax, curr);
        assign_consume(tip_amount, curr);
        assign_consume(tolls_amount, curr);
        assign_consume(improvement_surcharge, curr);
        assign_consume(total_amount, curr);
    }

    friend std::ostream& operator<<(std::ostream& out, const YellowTaxiCsv& taxi) {
        out << taxi.vendor_id << ","
            << taxi.pickup_datetime << ","
            << taxi.dropoff_datetime << ","
            << taxi.passenger_count << ","
            << taxi.trip_distance << ","
            << taxi.rate_code << ","
            << taxi.store_and_fwd_flag << ","
            << taxi.pickup_location_id << ","
            << taxi.dropoff_location_id << ","
            << taxi.payment_type << ","
            << taxi.fare_amount << ","
            << taxi.extra << ","
            << taxi.mta_tax << ","
            << taxi.tip_amount << ","
            << taxi.tolls_amount << ","
            << taxi.improvement_surcharge << ","
            << taxi.total_amount;
        return out;
    }

    timestamp order() const {
        return dropoff_datetime;
    }

    friend bool operator>(const YellowTaxiCsv& a, const YellowTaxiCsv& b) {
        return a.dropoff_datetime > b.dropoff_datetime;
    }

    friend bool operator==(const YellowTaxiCsv& a, const YellowTaxiCsv& b) {
        return a.vendor_id == b.vendor_id &&
               a.pickup_datetime == b.pickup_datetime &&
               a.dropoff_datetime == b.dropoff_datetime &&
               a.passenger_count == b.passenger_count &&
               a.trip_distance == b.trip_distance &&
               a.rate_code == b.rate_code &&
               a.store_and_fwd_flag == b.store_and_fwd_flag &&
               a.pickup_location_id == b.pickup_location_id &&
               a.dropoff_location_id == b.dropoff_location_id &&
               a.payment_type == b.payment_type &&
               a.fare_amount == b.fare_amount &&
               a.extra == b.extra &&
               a.mta_tax == b.mta_tax &&
               a.tip_amount == b.tip_amount &&
               a.tolls_amount == b.tolls_amount &&
               a.improvement_surcharge == b.improvement_surcharge &&
               a.total_amount == b.total_amount;
    }

    friend bool operator!=(const YellowTaxiCsv& a, const YellowTaxiCsv& b) {
        return !(a == b);
    }

    YellowTaxiCsv& operator+=(const YellowTaxiCsv& other) {
        // This is non-sensical; it has no real meaning. I just implemented a cheap 
        // operation to force data usage during queries.
        passenger_count += other.passenger_count; 
        return *this;
    }

    static const YellowTaxiCsv identity;
    static constexpr int skip_lines() {
        return 2;
    }

private:
    std::string vendor_id;
    timestamp pickup_datetime;
    timestamp dropoff_datetime;
    int passenger_count;
    double trip_distance;
    int rate_code;
    std::string store_and_fwd_flag;
    int pickup_location_id;
    int dropoff_location_id;
    int payment_type;
    double fare_amount;
    double extra;
    double mta_tax;
    double tip_amount;
    double tolls_amount;
    double improvement_surcharge;
    double total_amount;
};

const YellowTaxiCsv YellowTaxiCsv::identity;

class DebsTaxiCsv {
public:
    typedef typename std::chrono::system_clock::time_point timestamp;

    DebsTaxiCsv():
        rate_code(0),
        passenger_count(0),
        trip_time_in_secs(0),
        trip_distance(0.0),
        pickup_longitude(0.0),
        pickup_latitude(0.0),
        dropoff_longitude(0.0),
        dropoff_latitude(0.0)
    {}

    explicit operator timestamp() const {
        return pickup_datetime;
    }

    explicit operator int() const {
        return passenger_count;
    }

    explicit operator int64_t() const {
        return passenger_count;
    }

    explicit operator std::size_t() const {
        return std::hash<std::string>()(medallion);
    }

    explicit operator double() const {
        return trip_distance;
    }

    void parse(std::string line) {
        tokenizer tokens(line);
        tokenizer::iterator curr = tokens.begin();

        assign_consume(medallion, curr);
        assign_consume(hack_license, curr);
        assign_consume(vendor_id, curr);
        assign_consume(rate_code, curr);
        assign_consume(store_and_fwd_flag, curr);
        assign_consume(pickup_datetime, curr);
        assign_consume(dropoff_datetime, curr);
        assign_consume(passenger_count, curr);
        assign_consume(trip_time_in_secs, curr);
        assign_consume(trip_distance, curr);
        assign_consume(pickup_longitude, curr);
        assign_consume(pickup_latitude, curr);
        assign_consume(dropoff_longitude, curr);
        assign_consume(dropoff_latitude, curr);
    }

    friend std::ostream& operator<<(std::ostream& out, const DebsTaxiCsv& taxi) {
        out << taxi.medallion << ","
            << taxi.hack_license << ","
            << taxi.vendor_id << ","
            << taxi.rate_code << ","
            << taxi.store_and_fwd_flag << ","
            << taxi.pickup_datetime << ","
            << taxi.dropoff_datetime << ","
            << taxi.passenger_count << ","
            << taxi.trip_time_in_secs << ","
            << taxi.trip_distance << ","
            << taxi.pickup_longitude << ","
            << taxi.pickup_latitude << ","
            << taxi.dropoff_longitude << ","
            << taxi.dropoff_latitude;

        return out;
    }

    timestamp order() const {
        return pickup_datetime;
    }

    friend bool operator>(const DebsTaxiCsv& a, const DebsTaxiCsv& b) {
        return a.pickup_datetime > b.pickup_datetime;
    }

    friend bool operator==(const DebsTaxiCsv& a, const DebsTaxiCsv& b) {
        return a.medallion == b.medallion &&
               a.hack_license == b.hack_license &&
               a.vendor_id == b.vendor_id &&
               a.rate_code == b.rate_code &&
               a.store_and_fwd_flag == b.store_and_fwd_flag &&
               a.pickup_datetime == b.pickup_datetime &&
               a.dropoff_datetime == b.dropoff_datetime &&
               a.passenger_count == b.passenger_count &&
               a.trip_time_in_secs == b.trip_time_in_secs &&
               a.trip_distance == b.trip_distance &&
               a.pickup_longitude == b.pickup_longitude &&
               a.pickup_latitude == b.pickup_latitude &&
               a.dropoff_longitude == b.dropoff_longitude &&
               a.dropoff_latitude == b.dropoff_latitude;
    }

    friend bool operator!=(const DebsTaxiCsv& a, const DebsTaxiCsv& b) {
        return !(a == b);
    }

    DebsTaxiCsv& operator+=(const DebsTaxiCsv& other) {
        // This is non-sensical; it has no real meaning. I just implemented a cheap 
        // operation to force data usage during queries.
        passenger_count += other.passenger_count; 
        return *this;
    }

    static const DebsTaxiCsv identity;
    static constexpr int skip_lines() {
        return 1;
    }

private:
    std::string medallion;
    std::string hack_license;
    std::string vendor_id;
    int rate_code;
    std::string store_and_fwd_flag;
    timestamp pickup_datetime;
    timestamp dropoff_datetime;
    int passenger_count;
    int trip_time_in_secs;
    double trip_distance;
    double pickup_longitude;
    double pickup_latitude;
    double dropoff_longitude;
    double dropoff_latitude;
};

const DebsTaxiCsv DebsTaxiCsv::identity;

template <typename Data>
class FileDataGenerator {
public:
    FileDataGenerator(const std::string& f, int s):
        file(f),
        skip_lines(s),
        next(0)
    {}

    void load() {
        auto start = std::chrono::system_clock::now();
        std::ifstream in(file, std::ios::binary);
        if (!in.is_open()) {
            std::cerr << "Could not open " << file << std::endl;
            throw std::invalid_argument(file);
        }

        int skipped = 0;
        std::string line;
        while (std::getline(in, line)) {
            if (skipped < skip_lines) {
                ++skipped;
                continue;
            }
            Data data;
            data.parse(line);
            cache.push_back(data);
        }
        std::chrono::duration<double> runtime = std::chrono::system_clock::now() - start;
        std::cout << "loaded " << cache.size() << " data items from " << file << std::endl << 
                     "in " << runtime.count() << " seconds " << std::endl;
    }

    const Data& operator*() const {
        return cache[next];
    }

    FileDataGenerator& operator++() {
        ++next;
        return *this;
    }

    bool is_valid() {
        return next < cache.size();
    }

    void reset() {
        next = 0;
    }

private:
    std::string file;
    int skip_lines;
    std::vector<Data> cache;
    size_t next;
};

#endif
