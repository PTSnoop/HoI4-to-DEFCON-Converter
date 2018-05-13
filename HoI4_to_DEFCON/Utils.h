#pragma once

#include <tuple>
#include <string>

//#define BOOST_NO_SCOPED_ENUMS
//#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>

typedef std::tuple<unsigned char, unsigned char, unsigned char> ColourTriplet;
typedef std::pair<double, double> Coord;

bool is_number(const std::string& s);

bool copyDir(boost::filesystem::path const & source, boost::filesystem::path const & destination);