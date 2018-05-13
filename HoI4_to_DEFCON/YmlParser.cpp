// Not a real YAML parser.

#include "stdafx.h"

#include "YmlParser.h"

#include <vector>
#include <string>
#include <fstream>
#include <streambuf>

#include <boost/algorithm/string.hpp>

bool ParseYml(std::map<std::string,std::string>& FileData, std::string sInText)
{
	std::vector<std::string> Lines;
	boost::split(Lines, sInText, boost::is_any_of("\r\n"));
	for (std::string sLine : Lines)
	{
		std::size_t iColonPos = sLine.find(":");
		if (iColonPos == std::string::npos) continue;
		std::string sKey = sLine.substr(0,iColonPos);
		sLine = sLine.substr(iColonPos);

		std::size_t iSpacePos = sLine.find(" ");
		if (iSpacePos == std::string::npos) continue;
		std::string sValue = sLine.substr(iSpacePos);

		boost::erase_all(sValue, "\"");

		if (sKey[0] == ' ') sKey = sKey.substr(1, sKey.size() - 1);
		if (sValue[0] == ' ') sValue = sValue.substr(1, sValue.size() - 1);

		FileData[sKey] = sValue;
	}
	return true;
}

bool ParseYmlFile(std::map<std::string, std::string>& FileData, std::string sPath)
{
	std::ifstream Filestream(sPath);
	std::string sRawFileData((std::istreambuf_iterator<char>(Filestream)), std::istreambuf_iterator<char>());
	return ParseYml(FileData, sRawFileData);
}
