//#define BOOST_SPIRIT_DEBUG
//#include <boost/spirit/include/qi.hpp>

#include <stdafx.h>

#include <vector>
#include <string>
#include <fstream>
#include <streambuf>

#include <boost/algorithm/string.hpp>

bool ParseCsv(std::vector< std::vector<std::string> >& FileData, std::string sInText)
{
	std::vector<std::string> Lines;
	boost::split(Lines, sInText, boost::is_any_of("\r\n"));
	for (std::string sLine : Lines)
	{
		std::vector<std::string> Vars;
		boost::split(Vars, sLine, boost::is_any_of(",;"));
		FileData.push_back(Vars);
	}
	return true;
}

bool ParseCsvFile(std::vector< std::vector<std::string> >& FileData, std::string sPath)
{
	std::ifstream Filestream (sPath);
	std::string sRawFileData ((std::istreambuf_iterator<char>(Filestream)), std::istreambuf_iterator<char>());
	return ParseCsv(FileData, sRawFileData);
}
