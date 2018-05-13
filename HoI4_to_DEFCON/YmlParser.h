// Not a real YAML parser.

#include <vector>
#include <map>

bool ParseYml(std::map<std::string, std::string>& FileData, std::string sInText);
bool ParseYmlFile(std::map<std::string, std::string>& FileData, std::string sPath);