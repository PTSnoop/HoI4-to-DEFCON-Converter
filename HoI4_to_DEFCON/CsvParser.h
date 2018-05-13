#pragma once

#include <vector>

bool ParseCsv(std::vector< std::vector<std::string> >& FileData, std::string sInText);
bool ParseCsvFile(std::vector< std::vector<std::string> >& FileData, std::string sPath);