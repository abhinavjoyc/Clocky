#include "httplib.h"
#include "json.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include "http.h"



using json = nlohmann::json;

// --------------------------------------------------------
// WEATHER STATUS TEXT
// --------------------------------------------------------
std::string GetWeatherStatus(int code)
{
    switch (code)
    {
    case 0: return "Clear sky";
    case 1: return "Mainly clear";
    case 2: return "Partly cloudy";
    case 3: return "Overcast";

    case 45:
    case 48: return "Foggy";

    case 51: return "Light drizzle";
    case 53: return "Moderate drizzle";
    case 55: return "Dense drizzle";

    case 56:
    case 57: return "Freezing drizzle";

    case 61: return "Light rain";
    case 63: return "Moderate rain";
    case 65: return "Heavy rain";

    case 66:
    case 67: return "Freezing rain";

    case 71: return "Light snowfall";
    case 73: return "Moderate snowfall";
    case 75: return "Heavy snowfall";

    case 80: return "Rain showers";
    case 81: return "Moderate showers";
    case 82: return "Violent showers";

    case 95: return "Thunderstorm";
    case 96:
    case 99: return "Thunderstorm with hail";

    default: return "Unknown weather";
    }
}

// --------------------------------------------------------
// GET CITY NAME BY IP
// --------------------------------------------------------
std::string GetCity()
{
    std::string city = "unknown";

    httplib::Client geo("http://ip-api.com");

    if (auto res = geo.Get("/json"))
    {
        if (res->status == 200)
        {
            json j = json::parse(res->body);
            city = j.value("city", "unknown");

            std::cout << "City: " << city << "\n";
        }
    }

    return city;
}

// --------------------------------------------------------
// WEATHER FETCH FUNCTION
// --------------------------------------------------------
climate Getweather(double lat, double lon)
{
    double temp = 0.0;
    double wind = 0.0;

    if (lat != 0.0 || lon != 0.0)
    {
        httplib::Client cli("http://api.open-meteo.com");

        std::ostringstream url;
        url << "/v1/forecast?"
            << "latitude=" << lat
            << "&longitude=" << lon
            << "&current_weather=true"
            << "&hourly=temperature_2m,wind_speed_10m,relative_humidity_2m,weathercode"
            << "&timezone=auto";

        if (auto res = cli.Get(url.str().c_str()))
        {
            if (res->status == 200)
            {
                json w = json::parse(res->body);

                if (w.contains("current_weather"))
                {
                    temp = w["current_weather"].value("temperature", 0.0);
                    wind = w["current_weather"].value("windspeed", 0.0);
                }
            }
        }
        climate current;
        current.temp = temp;
        current.wind = wind;
        return current;
    }

    // i = 1 ? return temperature
    // i = anything else ? wind
    
}

// --------------------------------------------------------
// GET LAT/LON VIA IP LOOKUP
// --------------------------------------------------------
loc1 getpos()
{
    double lat1 = 0.0;
    double lon1 = 0.0;

    httplib::Client geo("http://ip-api.com");

    if (auto res = geo.Get("/json"))
    {
        if (res->status == 200)
        {
            json j = json::parse(res->body);
            lat1 = j.value("lat", 0.0);
            lon1 = j.value("lon", 0.0);

            std::cout << "Lat: " << lat1 << " Lon: " << lon1 << "\n";
        }
    }

    loc1 result;
    result.lat = lat1;
    result.lon = lon1;

    return result;
}
