#pragma once

#include <string>

//#define BOOST_NO_SCOPED_ENUMS
//#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>

enum Superpowers {Powerful, Factions, Custom};

class Configuration
{
public:
	static Configuration& Get() // Singleton
	{
		static Configuration instance;
		return instance;
	}

	bool Init(std::string sConfigPath);
	bool CreateDirectories();

	boost::filesystem::path GetSavePath();
	boost::filesystem::path GetHoi4Path();
	boost::filesystem::path GetHoi4ModPath();
	boost::filesystem::path GetDefconPath();
	boost::filesystem::path GetBaseModPath();
	boost::filesystem::path GetOutputPath();

	boost::filesystem::path GetModdedHoi4File(boost::filesystem::path TargetPath);

	Superpowers GetSuperpowerOption();
	std::vector< std::vector< std::string> > GetCustomSides();

private:
	Configuration();
	Configuration(Configuration const&);  // Unimplemented
	void operator=(Configuration const&); // Unimplemented

	boost::filesystem::path m_SavePath;
	boost::filesystem::path m_Hoi4Path;
	boost::filesystem::path m_Hoi4ModPath;
	boost::filesystem::path m_DefconPath;
	boost::filesystem::path m_BaseModPath;
	boost::filesystem::path m_OutputPath;

	Superpowers m_SuperpowerOption;
	std::vector< std::vector< std::string> > m_Sides;

};