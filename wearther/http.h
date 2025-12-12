#pragma once

#include<iostream>


std::string GetCity();
struct climate
{
    double wind;
    double temp;
};

climate Getweather( double lat, double lon);
std::string GetWeatherStatus(int code);



struct loc1
{

    double lat;
    double lon;
};
loc1 getpos();


