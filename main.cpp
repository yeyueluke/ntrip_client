/*
MIT License

Copyright (c) 2025 Noah Giustini

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "ntrip_client.h"
#include <math.h>
#include <csignal>
#include <iostream>
#include <ostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <string>
#include <ctime>

bool run = true;

/**
 * @brief Signal handler for SIGINT.
 * 
 * @param signal The signal number.
 */
void signal_handler(int signal) {
    std::cout << "SIGINT received, shutting down..." << std::endl;
    run = false;
}

/**
 * @brief Converts a decimal degree to a DDMM format.
 * 
 * @param degree The decimal degree to convert.
 * @return The DDMM format of the decimal degree.
 */
static double degree_to_ddmm(double const& degree) {
  int deg = static_cast<int>(floor(degree));
  double minute = degree - deg*1.0;
  return (deg*1.0 + minute*60.0/100.0);
}

/**
 * @brief Generates a GGA message from the provided latitude, longitude, and altitude.
 * 
 * @param lat The latitude in decimal degrees.
 * @param lon The longitude in decimal degrees.
 * @param alt The altitude in meters.
 * @return The GGA message.
 */
static std::string generage_gga_message(double lat, double lon, double alt) {
    char buffer[256];
    time_t now = time(0);
    struct tm tstruct;
    tstruct = *gmtime(&now);
    char utc_time[20];
    strftime(utc_time, sizeof(utc_time), "%H%M%S", &tstruct);

    double lat_ddmm = degree_to_ddmm(lat);
    double lon_ddmm = degree_to_ddmm(lon);

    snprintf(buffer, sizeof(buffer),
             "$GPGGA,%s,%.4f,%c,%.4f,%c,1,08,0.9,%.1f,M,0.0,M,,",
             utc_time,
             fabs(lat_ddmm), (lat >= 0) ? 'N' : 'S',
             fabs(lon_ddmm), (lon >= 0) ? 'E' : 'W',
             alt);

    int checksum = 0;
    for (int i = 1; buffer[i] != '\0'; i++) {
        checksum ^= buffer[i];
    }

    char checksum_str[10];
    snprintf(checksum_str, sizeof(checksum_str), "*%02X", checksum);
    std::string gga_message = std::string(buffer) + std::string(checksum_str) + "\r\n";

    return gga_message;
}

static void generage_gga_message(double lat, double lon, double alt, std::string* gga_out) {
    char src[256];
    time_t time_now = time(0);
    struct tm tm_now = {0};
    localtime_r(&time_now, &tm_now);

    char *ptr = src;
    ptr += snprintf(ptr, sizeof(src)+src-ptr,
        "$GPGGA,%02.0f%02.0f%05.2f,%012.7f,%s,%013.7f,%s,1,"
        "30,1.2,%.4f,M,-2.860,M,,0000",
        tm_now.tm_hour*1.0, tm_now.tm_min*1.0, tm_now.tm_sec*1.0,
        fabs(degree_to_ddmm(lat))*100.0,
        lat > 0.0 ? "N" : "S",
        fabs(degree_to_ddmm(lon))*100.0,
        lon > 0.0 ? "E" : "W",
        alt);
    uint8_t checksum = 0;
    for (char *q = src + 1; q <= ptr; q++) {
        checksum ^= *q;  // check sum.
    }
    ptr += snprintf(ptr, sizeof(src)+src-ptr, "*%02X%c%c", checksum, 0x0D, 0x0A);
    *gga_out = std::string(src, ptr-src);
}

/**
 * @brief Main function for the NtripClient.
 * 
 * @return 0 if the program exits successfully.
 */
int main() {
    std::string gga_message;
    generage_gga_message(31.167692767, 121.216608817, 10, &gga_message);
    NtripClient client;
    client.Init("120.253.239.161", "8002", "RTCM33_GRCEJ", "csha6912", "umt6n5hu");
    client.UpdateGGA(gga_message);
    client.Run();
    std::signal(SIGINT, signal_handler);
    std::cout << "NtripClient is running. Press Ctrl+C to stop." << std::endl;
    std::cout << gga_message << std::endl;
    while (run) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        client.UpdateGGA(gga_message);
    }
    client.Stop();
    return 0;
}