// HoI4_to_DEFCON.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <random>
#include <codecvt>

//#define BOOST_NO_SCOPED_ENUMS
//#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>

#include <CImg.h>
using namespace cimg_library;

#include "Utils.h"
#include "Log.h"
#include "Object.h"
#include "Parser.h"
#include "YmlParser.h"
#include "CsvParser.h"
#include "GetEdges.h"

#include "Configuration.h"

auto real_rand = std::bind(std::uniform_real_distribution<double>(0.0, 1.0), mt19937(12345));

std::string NoAccents(std::string sName)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8ToWstring;
	std::wstring wsName = utf8ToWstring.from_bytes(sName);

	std::wstring wsCharsWithAccents    = L"ŠŽšžŸÀÁÂÃÄÅÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖÙÚÛÜÝàáâãäåçèéêëìíîïðñòóôõöùúûüýÿ";
	std::wstring wsCharsWithoutAccents = L"SZszYAAAAAACEEEEIIIIDNOOOOOUUUUYaaaaaaceeeeiiiidnooooouuuuyy";

	for (size_t i = 0; i < wsCharsWithAccents.size(); i++)
	{
		std::replace(wsName.begin(), wsName.end(), wsCharsWithAccents[i], wsCharsWithoutAccents[i]);
	}

	return utf8ToWstring.to_bytes(wsName);
}

bool GetProvincePositions(std::map< int, std::pair<double, double> >& ProvincePositions, std::map<ColourTriplet, int>& ColourToId)
{
	// Eeh, if only we had all these numbers just sitting in map/provinces.txt like we did in HoI3. Ah well.

	Configuration& Config = Configuration::Get();
	boost::filesystem::path HoiProvinceMapPath = Config.GetModdedHoi4File("map/provinces.bmp");
	if (HoiProvinceMapPath.empty())
	{
		LOG(LogLevel::Error) << "Could not find Hoi4 map/provinces.bmp";
		return false;
	}

	CImg<unsigned char> ProvinceMap(HoiProvinceMapPath.string().c_str());
	if (NULL == ProvinceMap)
	{
		LOG(LogLevel::Error) << "Could not open " + HoiProvinceMapPath.string();
		return false;
	}

	//std::map<int, Coord> ProvincePositions;

	std::map<int, int> IdCounts;

	ColourTriplet ProvinceColour(0, 0, 0);

	for (int x = 0; x < ProvinceMap.width(); x++)
	{
		for (int y = 0; y < ProvinceMap.height(); y++)
		{
			std::get<0>(ProvinceColour) = ProvinceMap(x, y, 0, 0);
			std::get<1>(ProvinceColour) = ProvinceMap(x, y, 0, 1);
			std::get<2>(ProvinceColour) = ProvinceMap(x, y, 0, 2);

			int Id = ColourToId[ProvinceColour];
			int IdCountsSoFar = IdCounts[Id];

			ProvincePositions[Id].first = ((ProvincePositions[Id].first*IdCountsSoFar) + x) / (IdCountsSoFar + 1);
			ProvincePositions[Id].second = ((ProvincePositions[Id].second*IdCountsSoFar) + y) / (IdCountsSoFar + 1);

			IdCounts[Id]++;
		}
	}
	return true;
}

bool GetProvincesPerState(std::map<int, std::vector<int> >& ProvincesPerState, std::map<int, int>& StatePopulation, std::map<int, int>& StateCapitals)
{
	Configuration& Config = Configuration::Get();
	boost::filesystem::path Hoi4StatesFolder = Config.GetModdedHoi4File("history/states");

	boost::filesystem::directory_iterator DirectoryEnd;
	bool doneSomething = false;

	std::set<std::string> CityTypes{ "city", "large_city", "megalopolis", "metropolis" };

	for (boost::filesystem::directory_iterator Hoi4StatesIter(Hoi4StatesFolder); Hoi4StatesIter != DirectoryEnd; ++Hoi4StatesIter)
	{
		if (boost::filesystem::is_regular_file(Hoi4StatesIter->status()))
		{
			Object* stateFile = doParseFile(Hoi4StatesIter->path().string().c_str());
			Object* state = stateFile->safeGetObject("state");
			std::string sStateId = state->getLeaf("id");
			if (!is_number(sStateId))
			{
				LOG(LogLevel::Warning)
					<< "Warning: I don't understand the state ID '"
					<< sStateId
					<< "' in "
					<< Hoi4StatesIter->path().string();
				continue;
			}

			int iStateId = atoi(sStateId.c_str());

			std::vector<int>& RelevantProvinces = ProvincesPerState[iStateId];

			Object* provinces = state->safeGetObject("provinces");
			for (std::string sProvinceId : provinces->getTokens())
			{
				if (!is_number(sProvinceId))
				{
					LOG(LogLevel::Warning)
						<< "Warning: I don't understand province number '"
						<< sProvinceId
						<< "' in "
						<< Hoi4StatesIter->path().string();
					continue;
				}
				RelevantProvinces.push_back(atoi(sProvinceId.c_str()));
				doneSomething = true;
			}


			std::vector<Object*> History = state->getValue("history");
			if (false == History.empty())
			{
				std::vector<Object*> VictoryPoints = History[0]->getValue("victory_points");
				if (!VictoryPoints.empty())
				{
					std::vector<std::string> sVictoryProvinceness = VictoryPoints[0]->getTokens();
					if (false == sVictoryProvinceness.empty())
					{
						std::string sVictoryProvince = sVictoryProvinceness[0];
						if (is_number(sVictoryProvince))
						{
							int iVictoryProvince = atoi(sVictoryProvince.c_str());
							StateCapitals[iStateId] = iVictoryProvince;
						}
					}
				}
			}

			std::string sManpower = state->getLeaf("manpower");
			if (is_number(sManpower))
			{
				int iManpower = atoi(sManpower.c_str());

				// city-type states get more population
				std::string sCategory = state->getLeaf("state_category");
				if (CityTypes.find(sCategory) != CityTypes.end())
				{
					iManpower *= 2;
				}

				StatePopulation[iStateId] = iManpower;
			}

			delete stateFile;
		}
	}

	return doneSomething;
}

bool GetProvinceNumbers(std::map<int, std::string>& Territories, std::map<std::string, int>& TerritoryCounts, std::map<int, double>& CityScore, std::map<int, bool>& Capitals, Object* SaveFile, std::map<int, std::vector<int> >& ProvincesPerState, std::map<int, int> StatePopulation, std::map<int, int> StateCapitals, std::map<std::string, std::string>& GovernmentTypes)
{
	std::string sSeed = SaveFile->getLeaf("game_unique_seed");
	if (is_number(sSeed))
	{
		int iSeed = atoi(sSeed.c_str());
#ifdef MSC_VER
		//This line doesn't work on GCC, and I don't have time to figure out why not.
		real_rand = std::bind(std::uniform_real_distribution<double>(0.0, 1.0), mt19937(iSeed));
#endif
	}

	std::map<int, std::string> CapitalsByStateId;

	Object* Countries = SaveFile->safeGetObject("countries");
	for (Object* Country : Countries->getLeaves())
	{
		std::string sCountryId = Country->getKey();

		std::vector<Object*> Capital = Country->getValue("capital");
		if (false == Capital.empty())
		{
			std::string sCapitalState = Capital.at(0)->getLeaf();
			if (is_number(sCapitalState))
			{
				int iCapitalState = atoi(sCapitalState.c_str());
				CapitalsByStateId[iCapitalState] = sCountryId;
			}
		}

		Object* Politics = Country->safeGetObject("politics");
		std::string sGovernment = "democratic";
		if (Politics)
		{
			sGovernment = Politics->safeGetString("ruling_party", sGovernment);
		}
		GovernmentTypes[sCountryId] = sGovernment;
	}

	Object* States = SaveFile->safeGetObject("states");
	for (Object* State : States->getLeaves())
	{
		std::string sStateId = State->getKey();
		if (false == is_number(sStateId))
			continue;
		int iStateId = atoi(sStateId.c_str());

		std::vector<int>& Provinces = ProvincesPerState[iStateId];

		std::string sOwner = "";
		std::vector<Object*> Owners = State->getValue("owner");
		if (false == Owners.empty())
		{
			sOwner = Owners.at(0)->getLeaf();
			for (int iProvinceId : Provinces)
			{
				Territories[iProvinceId] = sOwner;
			}
		}

		std::vector<Object*> Controllers = State->getValue("controller");
		if (false == Controllers.empty())
		{
			sOwner = Controllers.at(0)->getLeaf();
			for (int iProvinceId : Provinces)
			{
				Territories[iProvinceId] = sOwner;
			}
		}

		for (int iProvinceId : Provinces)
		{
			TerritoryCounts[Territories[iProvinceId]]++;
		}

		int iStatePopulation = StatePopulation[iStateId];
		Object* ManpowerPool = State->safeGetObject("manpower_pool");
		if (ManpowerPool)
		{
			std::string sStatePopulation = ManpowerPool->getLeaf("total");
			if (is_number(sStatePopulation))
			{
				iStatePopulation = atoi(sStatePopulation.c_str());
			}
		}

		iStatePopulation *= 5;

		int iStateCapital = StateCapitals[iStateId];

		if (CapitalsByStateId[iStateId] == sOwner)
		{
			iStatePopulation *= 1.5;
			Capitals[iStateCapital] = true;
		}

		CityScore[iStateCapital] = iStatePopulation;
	}

	return true;
}

bool GetLocalisation(std::map<int, std::string>& ProvinceNames, std::map<std::string, std::string>& CountryNames)
{
	// TODO: support languages other than English.

	Configuration& Config = Configuration::Get();

	boost::filesystem::path VictoryProvinceNamesYmlPath = Config.GetModdedHoi4File("localisation/victory_points_l_english.yml");
	if (VictoryProvinceNamesYmlPath.empty())
	{
		LOG(LogLevel::Error) << "Could not find Hoi4 localisation/victory_points_l_english.yml";
		return false;
	}

	std::map<std::string, std::string> VictoryProvinceNames;
	if (false == ParseYmlFile(VictoryProvinceNames, VictoryProvinceNamesYmlPath.string().c_str()))
	{
		LOG(LogLevel::Error) << "Could not read " << VictoryProvinceNamesYmlPath.string();
		return false;
	}

	for (auto VictoryProvince : VictoryProvinceNames)
	{
		// Get rid of VICTORY_POINTS_
		std::string sProvinceId = VictoryProvince.first;
		size_t iUnderscore = sProvinceId.find('_');
		if (iUnderscore == std::string::npos) continue;
		iUnderscore = sProvinceId.find('_', iUnderscore + 1);
		if (iUnderscore == std::string::npos) continue;
		sProvinceId = sProvinceId.substr(iUnderscore + 1);

		if (false == is_number(sProvinceId)) continue;

		int iProvinceId = atoi(sProvinceId.c_str());

		ProvinceNames[iProvinceId] = VictoryProvince.second;
	}

	///

	boost::filesystem::path CountriesYmlPath = Config.GetModdedHoi4File("localisation/countries_mod_l_english.yml");
	if (CountriesYmlPath.empty())
	{
		CountriesYmlPath = Config.GetModdedHoi4File("localisation/countries_l_english.yml");
	}
	if (CountriesYmlPath.empty())
	{
		LOG(LogLevel::Error) << "Could not find Hoi4 localisation/countries_l_english.yml";
		return false;
	}

	if (false == ParseYmlFile(CountryNames, CountriesYmlPath.string().c_str()))
	{
		LOG(LogLevel::Error) << "Could not read " << CountriesYmlPath.string();
		return false;
	}

	return true;
}

bool CreateCitiesFile(std::vector< pair<int, double> >& SortedCityScores,
	std::map<int, std::string>& Territories,
	std::map<std::string, std::string>& CountryNames,
	std::map<int, std::string>& ProvinceNames,
	std::map< int, std::pair<double, double> >& ProvincePositions,
	std::map<int, bool>& Capitals,
	std::map<std::string, std::string>& GovernmentTypes)
{
	Configuration& Config = Configuration::Get();

	boost::filesystem::path CitiesDatPath = Config.GetOutputPath() / "data" / "earth" / "cities.dat";
	std::ofstream CitiesDat(CitiesDatPath.string().c_str());
	if (!CitiesDat)
	{
		LOG(LogLevel::Error) << "Could not write to " << CitiesDatPath.string();
		return false;
	}

	for (auto i = SortedCityScores.begin(); i != SortedCityScores.end(); ++i)
	{
		int iProvinceId = i->first;
		if (iProvinceId == 0)
			continue;

		std::string sTag = Territories[iProvinceId];
		std::string sGovernment = GovernmentTypes[sTag];
		sTag = sTag + "_" + sGovernment;

		std::string sCountryName = NoAccents(CountryNames[sTag]);
		std::string sProvinceName = NoAccents(ProvinceNames[iProvinceId]);
		std::pair<double, double> Pos = ProvincePositions[iProvinceId];

		if (sProvinceName.empty())
			continue;

		std::string sX = std::to_string((360.0*(Pos.first / 5632.0)) - 180.0);
		std::string sY = std::to_string(83.0 - (133.0*(Pos.second / 2048.0)));

		unsigned long long llPopulation = i->second;
		std::string sPopulation = std::to_string(llPopulation);
		bool bIsCapital = Capitals[iProvinceId];

		std::string sCityLine(125, ' ');

		sCityLine.replace(0, sProvinceName.size(), sProvinceName);
		sCityLine.replace(41, sCountryName.size(), sCountryName);
		sCityLine.replace(82, sX.size(), sX);
		sCityLine.replace(96, sY.size(), sY);
		sCityLine.replace(110, sPopulation.size(), sPopulation);
		sCityLine.replace(124, 1, bIsCapital ? "1" : "0");
		CitiesDat << sCityLine << std::endl;
	}

	CitiesDat.close();
	return true;
}

bool GetProvinceColourIds(std::map<ColourTriplet, int>& ColourToId)
{
	Configuration& Config = Configuration::Get();
	std::vector< std::vector<std::string> > Definitions;

	boost::filesystem::path DefinitionCsvPath = Config.GetModdedHoi4File("map/definition.csv");
	if (DefinitionCsvPath.empty())
	{
		LOG(LogLevel::Error) << "Could not find Hoi4 map/definition.csv";
		return false;
	}
	if (false == ParseCsvFile(Definitions, DefinitionCsvPath.string().c_str()))
	{
		LOG(LogLevel::Error) << "Could not read " << DefinitionCsvPath.string();
		return false;
	}

	for (auto Line : Definitions)
	{
		if (false == is_number(Line[0]))
			continue;

		ColourTriplet c(atoi(Line[1].c_str()), atoi(Line[2].c_str()), atoi(Line[3].c_str()));
		ColourToId[c] = atoi(Line[0].c_str());
	}
	return true;
}

bool GetSides(std::vector< std::vector< std::string > >& Sides, Object* SaveFile, std::vector< pair<std::string, int> >& SortedTerritoryCounts)
{
	Configuration& Config = Configuration::Get();
	Sides = std::vector< std::vector< std::string> >(6, std::vector<std::string>());

	switch (Config.GetSuperpowerOption())
	{
	case Superpowers::Custom:
	{
		Sides = std::vector< std::vector<std::string> >(Config.GetCustomSides());
		break;
	}
	case Superpowers::Factions:
	{
		auto FactionsObjects = SaveFile->getValue("faction");
		if (FactionsObjects.empty())
		{
			LOG(LogLevel::Error) << "Could not find factions in the save file.";
			return false;
		}

		std::vector<std::string> Added;
		int i = 0;
		for (Object* Faction : FactionsObjects)
		{
			std::vector<Object*> Members = Faction->getValue("members");
			if (Members.empty()) continue;

			for (std::string MemberTag : Members[0]->getTokens())
			{
				Sides[i].push_back(MemberTag);
				Added.push_back(MemberTag);
			}
			i++;
		}

		for (auto NextLargest : SortedTerritoryCounts)
		{
			std::string sNextLargest = NextLargest.first;
			if (std::find(Added.begin(), Added.end(), sNextLargest) == Added.end())
			{
				Sides[i].push_back(sNextLargest);
				i++;
				if (i >= 6) break;
			}
		}
		break;
	}
	case Superpowers::Powerful:
	{
		for (int i = 0; i < 6; i++)
		{
			if (i >= (int)SortedTerritoryCounts.size()) break;
			std::string sRelevant = SortedTerritoryCounts[i].first;
			Sides[i].push_back(sRelevant);
		}
		break;
	}
	}
	return true;
}

bool CreateTerritoryMaps(std::vector< std::vector< std::string > >& Sides, std::map<ColourTriplet, int>& ColourToId, std::map<int, std::string>& Territories)
{
	Configuration& Config = Configuration::Get();

	boost::filesystem::path LandbasePath = Config.GetOutputPath() / "data" / "earth" / "land_base.bmp";
	boost::filesystem::path SailablePath = Config.GetOutputPath() / "data" / "earth" / "sailable.bmp";
	boost::filesystem::path CoastlinesPath = Config.GetOutputPath() / "data" / "earth" / "coastlines.bmp";

	CImg<unsigned char> Landbase(LandbasePath.string().c_str());
	if (NULL == Landbase)
	{
		LOG(LogLevel::Error) << "Could not open " + LandbasePath.string();
		return false;
	}

	CImg<unsigned char> Sailable(SailablePath.string().c_str());
	if (NULL == Sailable)
	{
		LOG(LogLevel::Error) << "Could not open " + SailablePath.string();
		return false;
	}

	CImg<unsigned char> Coastlines(CoastlinesPath.string().c_str());
	if (NULL == Coastlines)
	{
		LOG(LogLevel::Error) << "Could not open " + CoastlinesPath.string();
		return false;
	}

	boost::filesystem::path HoiProvinceMapPath = Config.GetModdedHoi4File("map/provinces.bmp");
	if (HoiProvinceMapPath.empty())
	{
		LOG(LogLevel::Error) << "Could not find Hoi4 map/provinces.bmp";
		return false;
	}

	CImg<unsigned char> ProvinceMapFromFile(HoiProvinceMapPath.string().c_str());
	if (NULL == ProvinceMapFromFile)
	{
		LOG(LogLevel::Error) << "Could not open " + HoiProvinceMapPath.string();
		return false;
	}

	ProvinceMapFromFile.resize(512, 186, 1, 3, 1);

	CImg<unsigned char> ProvinceMap(512, 285, 1, 3, 0);
	ProvinceMap.draw_image(0, 26, ProvinceMapFromFile);

	std::vector< std::tuple< std::vector<std::string>, CImg<unsigned char>, CImg<unsigned char> > > TerritoryMaps;
	for (int i = 0; i < 6; i++)
	{
		TerritoryMaps.push_back(
			std::tuple< std::vector<std::string>, CImg<unsigned char>, CImg<unsigned char> >
			(Sides[i], CImg<unsigned char>(512, 285, 1, 3, 0), CImg<unsigned char>(512, 285, 1, 3, 0))
		);
	}

	const unsigned char grey[] = { 128, 128, 128 };

	for (int x = 0; x < ProvinceMap.width(); x++)
	{
		for (int y = 0; y < ProvinceMap.height(); y++)
		{
			ColourTriplet ProvinceColour(
				ProvinceMap(x, y, 0, 0),
				ProvinceMap(x, y, 0, 1),
				ProvinceMap(x, y, 0, 2)
			);

			int iProvinceId = ColourToId[ProvinceColour];
			std::string sTag = Territories[iProvinceId];

			for (int i = 0; i < 6; i++)
			{
				auto Relevants = std::get<0>(TerritoryMaps[i]);
				if (std::find(Relevants.begin(), Relevants.end(), sTag) != Relevants.end())
				{
					if (0 == Landbase(x, y, 0, 0))
						continue;

					std::get<1>(TerritoryMaps[i])(x, y, 0, 0) = 255;
					std::get<1>(TerritoryMaps[i])(x, y, 0, 1) = 255;
					std::get<1>(TerritoryMaps[i])(x, y, 0, 2) = 255;

					if (0 != Coastlines(x, y, 0, 0))
						std::get<2>(TerritoryMaps[i]).draw_circle(x, y, 30, grey);
				}
			}
		}

	}

	for (int i = 0; i < 6; i++)
	{
		std::get<2>(TerritoryMaps[i]) &= Sailable;
	}

	for (int i = 0; i < 6; i++)
	{
		std::get<1>(TerritoryMaps[i]) |= std::get<2>(TerritoryMaps[i]);
	}

	boost::filesystem::path OutputDataEarthPath = Config.GetOutputPath() / "data" / "earth";
	std::vector<boost::filesystem::path> ContinentPaths = {
		OutputDataEarthPath / "africa.bmp",
		OutputDataEarthPath / "europe.bmp",
		OutputDataEarthPath / "northamerica.bmp",
		OutputDataEarthPath / "russia.bmp",
		OutputDataEarthPath / "southamerica.bmp",
		OutputDataEarthPath / "southasia.bmp"
	};

	for (int i = 0; i < 6; i++)
	{
		std::get<1>(TerritoryMaps[i]).blur(1);
		std::get<1>(TerritoryMaps[i]).save(ContinentPaths[i].string().c_str());
	}
	return true;
}


bool QuickCreateMap()
{
	Configuration& Config = Configuration::Get();

	boost::filesystem::path LandbasePath = Config.GetOutputPath() / "oh_look_its_a_land_base.bmp";

	boost::filesystem::path HoiProvinceMapPath = "D:\\Files\\hoi4_to_defcon\\second\\HoI4_to_DEFCON\\HoI4_to_DEFCON\\output\\yellow3.png";
	std::string cHoiProvinceMapPath = "D:/Files/hoi4_to_defcon/second/HoI4_to_DEFCON/HoI4_to_DEFCON/output/yellow3.png";
	if (HoiProvinceMapPath.empty())
	{
		LOG(LogLevel::Error) << "Could not find Hoi4 map/provinces.bmp";
		return false;
	}

	CImg<unsigned char> ProvinceMap("D:/Files/hoi4_to_defcon/second/HoI4_to_DEFCON/HoI4_to_DEFCON/output/ProvincesAA.bmp");
	if (NULL == ProvinceMap)
	{
		LOG(LogLevel::Error) << "Could not open " + HoiProvinceMapPath.string();
		return false;
	}

	std::vector< std::vector<int> > allColourSet;

	for (int r = 254; r >= 128; r--)
	{
		for (int g = 254; g >= 128; g--)
		{
			for (int b = 254; b >= 128; b--)
			{
				std::vector<int> v = { r,g,b };
				allColourSet.push_back(v);
			}
		}
	}

	int used = 0;

	// Kludgy floodfills.
	for (int x = 0; x < ProvinceMap.width(); x++)
	{
		LOG(LogLevel::Info) << x;
		for (int y = 0; y < ProvinceMap.height(); y++)
		{
			if (ProvinceMap(x, y, 0, 0) != 255 && ProvinceMap(x, y, 0, 1) != 255 && ProvinceMap(x, y, 0, 2) != 255)
				continue;

			//if (real_rand() > 0.005) continue;

			//const unsigned char newcolour[] = {	1 + (254 * real_rand()), 1 + (254 * real_rand()), 1 + (254 * real_rand())};

			const unsigned char newcolour[] = { allColourSet[used][0], allColourSet[used][1], allColourSet[used][2] };
			used++;


			ProvinceMap.draw_fill(x, y, newcolour);
		}
	}

	ProvinceMap.save(LandbasePath.string().c_str());

	//

	std::set< std::tuple<int, int, int> > GotColours;
	GetEdges Edges;
	if (false == Edges.Init(&ProvinceMap))
	{
		LOG(LogLevel::Warning) << "Could not read country edges. This map will look kinda blank.";
	}
	else
	{
		Edges.GetAllColours(GotColours);
	}

	boost::filesystem::path InternationalDatPath = Config.GetOutputPath() / "data" / "earth" / "coastlines.dat";
	std::string s = InternationalDatPath.string();
	std::ofstream InternationalDat(InternationalDatPath.string().c_str());

	if (!InternationalDat)
	{
		LOG(LogLevel::Error) << "Could not write to " << InternationalDatPath.string();
		return false;
	}

	int i = 0;
	int j = GotColours.size();

	bool lastB = false;
	for (auto GotColour : GotColours)
	{
		i++;
		LOG(LogLevel::Info) << i << " / " << j;

		if (!lastB)
		{
			InternationalDat << "b" << std::endl;
			lastB = true;
		}

		std::vector< std::pair<double, double> > Points;
		Edges.Get(Points, GotColour);
		Points.push_back(Points.front());
		LOG(LogLevel::Info) << Points.size();
		for (std::pair<double, double>& Point : Points)
		{
			LOG(LogLevel::Info) << "\t" << Point.first << "\t" << Point.second;
			if (Point.first < 0 && Point.second < 0)
			{
				if (!lastB)
					InternationalDat << "b" << std::endl;
				lastB = true;
			}
			else
			{
				double x = ((Point.first / ProvinceMap.width())*360.0) - 180.0 - 14.0;
				double y = ((-Point.second / ProvinceMap.height())*131.0) + 33.0;
				InternationalDat << x << " " << y << std::endl;
				lastB = false;
			}
		}
	}

	InternationalDat.close();
	return true;
}

bool CreateInternationalBoundaries(std::map<ColourTriplet, int>& ColourToId, std::map<int, std::string>& Territories)
{
	Configuration& Config = Configuration::Get();
	boost::filesystem::path BigHoiProvinceMapPath = Config.GetModdedHoi4File("map/provinces.bmp");
	if (BigHoiProvinceMapPath.empty())
	{
		LOG(LogLevel::Error) << "Could not find Hoi4 map/provinces.bmp .";
		LOG(LogLevel::Error) << "But it was perfectly fine earlier. Now I'm confused.";
		return false;
	}

	CImg<unsigned char> BigProvinceMap(BigHoiProvinceMapPath.string().c_str());
	if (NULL == BigProvinceMap)
	{
		LOG(LogLevel::Error) << "Could not read " + BigHoiProvinceMapPath.string();
		LOG(LogLevel::Error) << "But it was perfectly fine earlier. Now I'm confused.";
		return false;
	}

	std::map<std::string, ColourTriplet> TmpNationColours;

	// Redundancy!
	for (int x = 0; x < BigProvinceMap.width(); x++)
	{
		for (int y = 0; y < BigProvinceMap.height(); y++)
		{
			ColourTriplet ProvinceColour(
				BigProvinceMap(x, y, 0, 0),
				BigProvinceMap(x, y, 0, 1),
				BigProvinceMap(x, y, 0, 2)
			);

			int Id = ColourToId[ProvinceColour];

			auto pOwner = Territories.find(Id);
			if (pOwner == Territories.end())
			{
				BigProvinceMap(x, y, 0, 0) = 0;
				BigProvinceMap(x, y, 0, 1) = 0;
				BigProvinceMap(x, y, 0, 2) = 0;
				continue;
			}

			std::string sOwner = pOwner->second;

			auto NewColour = TmpNationColours.find(sOwner);
			if (NewColour != TmpNationColours.end())
			{
				BigProvinceMap(x, y, 0, 0) = std::get<0>(NewColour->second);
				BigProvinceMap(x, y, 0, 1) = std::get<1>(NewColour->second);
				BigProvinceMap(x, y, 0, 2) = std::get<2>(NewColour->second);
			}
			else
			{
				TmpNationColours[sOwner] = ProvinceColour;
			}
		}
	}

	const unsigned char black[] = { 0, 0, 0 };
	BigProvinceMap.draw_fill(0, 0, black);

	// Kludgy floodfills.
	for (int x = 0; x < BigProvinceMap.width(); x++)
	{
		for (int y = 0; y < BigProvinceMap.height(); y++)
		{
			if (BigProvinceMap(x, y, 0, 0) == 0 && BigProvinceMap(x, y, 0, 1) == 0 && BigProvinceMap(x, y, 0, 2) == 0)
				continue;

			if (real_rand() > 0.0002) continue;

			const unsigned char newcolour[] = {
				BigProvinceMap(x, y, 0, 0) + floor((10.0 * real_rand()) - 5.0),
				BigProvinceMap(x, y, 0, 1) + floor((10.0 * real_rand()) - 5.0),
				BigProvinceMap(x, y, 0, 2) + floor((10.0 * real_rand()) - 5.0)
			};
			BigProvinceMap.draw_fill(x, y, newcolour);
		}
	}

	//


	std::set< std::tuple<int, int, int> > GotColours;
	GetEdges Edges;
	if (false == Edges.Init(&BigProvinceMap))
	{
		LOG(LogLevel::Warning) << "Could not read country edges. This map will look kinda blank.";
	}
	else
	{
		Edges.GetAllColours(GotColours);
	}

	boost::filesystem::path InternationalDatPath = Config.GetOutputPath() / "data" / "earth" / "international.dat";
	std::ofstream InternationalDat(InternationalDatPath.string().c_str());

	if (!InternationalDat)
	{
		LOG(LogLevel::Error) << "Could not write to " << InternationalDatPath.string();
		return false;
	}

	bool lastB = false;
	for (auto GotColour : GotColours)
	{
		if (!lastB)
		{
			InternationalDat << "b" << std::endl;
			lastB = true;
		}

		std::vector< std::pair<double, double> > Points;
		Edges.Get(Points, GotColour);
		for (std::pair<double, double>& Point : Points)
		{
			if (Point.first < 0 && Point.second < 0)
			{
				if (!lastB)
					InternationalDat << "b" << std::endl;
				lastB = true;
			}
			else
			{
				double x = ((Point.first / BigProvinceMap.width())*360.0) - 180.0;
				double y = ((-Point.second / BigProvinceMap.height())*131.0) + 82.0;
				InternationalDat << x << " " << y << std::endl;
				lastB = false;
			}
		}
	}

	InternationalDat.close();
	return true;
}

bool CreateModTxt()
{
	Configuration& Config = Configuration::Get();
	boost::filesystem::path OutputPath = Config.GetOutputPath();

	std::string sName = OutputPath.filename().string();

	boost::filesystem::path ModTxtPath = Config.GetOutputPath() / "mod.txt";
	std::ofstream ModTxt(ModTxtPath.string().c_str());
	if (!ModTxt)
	{
		LOG(LogLevel::Error) << "Could not write to " << ModTxtPath.string();
		return false;
	}

	ModTxt << "Name         " << "Converted - " << sName << std::endl;
	ModTxt << "Version      " << "1.0" << std::endl;
	ModTxt << "Author       " << "PTSnoop" << std::endl;
	ModTxt << "Website      " << "https://github.com/PTSnoop/Hoi4toDefconConverter" << std::endl;
	ModTxt << "Comment      " << "This mod was created using the Hearts Of Iron 4 to Defcon converter." << std::endl;

	ModTxt.close();
	return true;
}

bool CopyModIntoDefcon()
{
	Configuration& Config = Configuration::Get();
	boost::filesystem::path OutputPath = Config.GetOutputPath();
	boost::filesystem::path DefconPath = Config.GetDefconPath();

	if (DefconPath.empty())
		return true;

	boost::filesystem::path DefconModPath = DefconPath / "mods";
	if (false == boost::filesystem::exists(DefconModPath))
	{
		if (false == boost::filesystem::create_directories(DefconModPath))
		{
			LOG(LogLevel::Error) << "Could not create directory " << DefconModPath.string();
			return false;
		}
	}

	boost::filesystem::path Name = OutputPath.filename();
	DefconModPath = DefconModPath / Name;

	boost::filesystem::remove_all(DefconModPath);
	if (exists(DefconModPath))
	{
		LOG(LogLevel::Error) << "Could not overwrite preexisting Defcon mod " << DefconModPath.string();
		return false;
	}

	if (false == copyDir(OutputPath, DefconModPath))
	{
		LOG(LogLevel::Error) << "Could not create " << DefconModPath.string();
		return false;
	}

	return true;
}

int main()
{
	LOG(LogLevel::Info) << "Loading configuration...";
	Configuration& Config = Configuration::Get();

	if (false == Config.Init("configuration.txt"))
	{
		LOG(LogLevel::Error) << "Could not load configuration. Exiting.";
		return 1;
	}


	LOG(LogLevel::Info) << "Creating mod folder structure...";
	if (false == Config.CreateDirectories())
	{
		LOG(LogLevel::Error) << "Could not create mod folder structure. Exiting.";
		return 1;
	}

	LOG(LogLevel::Info) << "Loading save file " << Config.GetSavePath().string() << " ...";
	Object* SaveFile = doParseFile(Config.GetSavePath().string().c_str());

	if (nullptr == SaveFile)
	{
		LOG(LogLevel::Error) << "Could not load save file. Exiting.";
		return 1;
	}

	LOG(LogLevel::Info) << "Creating mod folder structure...";
	if (false == Config.CreateDirectories())
	{
		LOG(LogLevel::Error) << "Could not create mod folder structure. Exiting.";
		return 1;
	}

	std::map<int, std::vector<int> > ProvincesPerState;
	std::map<int, int> StatePopulation;
	std::map<int, int> StateCapitals;

	LOG(LogLevel::Info) << "Getting state and province data...";
	if (false == GetProvincesPerState(ProvincesPerState, StatePopulation, StateCapitals))
	{
		LOG(LogLevel::Error) << "Could not understand the state history files. Exiting.";
		return 1;
	}

	std::map<ColourTriplet, int> ColourToId;
	LOG(LogLevel::Info) << "Reading the province map...";
	if (false == GetProvinceColourIds(ColourToId))
	{
		LOG(LogLevel::Error) << "Could not understand the province map. Exiting.";
		return 1;
	}

	std::map< int, std::pair<double, double> > ProvincePositions;

	LOG(LogLevel::Info) << "Getting province coordinates...";
	if (false == GetProvincePositions(ProvincePositions, ColourToId))
	{
		LOG(LogLevel::Error) << "Could not get province coordinates. Exiting.";
		return 1;
	}


	std::map<int, std::string> Territories;
	std::map<std::string, int> TerritoryCounts;
	std::map<int, double> CityScore;
	std::map<int, bool> Capitals;
	std::map<std::string, std::string> GovernmentTypes;

	LOG(LogLevel::Info) << "Reading save file contents...";
	if (false == GetProvinceNumbers(Territories, TerritoryCounts, CityScore, Capitals, SaveFile, ProvincesPerState, StatePopulation, StateCapitals, GovernmentTypes))
	{
		LOG(LogLevel::Error) << "Could not understand the save file. Exiting.";
		return 1;
	}


	std::vector<pair<std::string, int>> SortedTerritoryCounts;
	for (auto itr = TerritoryCounts.begin(); itr != TerritoryCounts.end(); ++itr)
		SortedTerritoryCounts.push_back(*itr);

	sort(
		SortedTerritoryCounts.begin(),
		SortedTerritoryCounts.end(),
		[=](const pair<std::string, int>& a, const pair<std::string, int>& b)
	{ return a.second > b.second; }
	);

	std::vector<pair<int, double>> SortedCityScores;
	for (auto itr = CityScore.begin(); itr != CityScore.end(); ++itr)
		SortedCityScores.push_back(*itr);

	sort(
		SortedCityScores.begin(),
		SortedCityScores.end(),
		[=](const pair<int, double>& a, const pair<int, double>& b)
	{ return a.second > b.second; }
	);

	std::map<int, std::string> ProvinceNames;
	std::map<std::string, std::string> CountryNames;

	LOG(LogLevel::Info) << "Reading names from localization...";
	if (false == GetLocalisation(ProvinceNames, CountryNames))
	{
		LOG(LogLevel::Error) << "No comprende. Exiting.";
		return 1;
	}

	std::vector< std::vector< std::string > > Sides;

	LOG(LogLevel::Info) << "Taking sides...";
	if (false == GetSides(Sides, SaveFile, SortedTerritoryCounts))
	{
		LOG(LogLevel::Error) << "Could not assign sides. Exiting.";
		return 1;
	}

	LOG(LogLevel::Info) << "Creating cities file...";
	if (false == CreateCitiesFile(SortedCityScores, Territories, CountryNames, ProvinceNames, ProvincePositions, Capitals, GovernmentTypes))
	{
		LOG(LogLevel::Error) << "Could not create cities.dat . Exiting.";
		return 1;
	}

	LOG(LogLevel::Info) << "Creating territory maps...";
	if (false == CreateTerritoryMaps(Sides, ColourToId, Territories))
	{
		LOG(LogLevel::Error) << "Could not create territory maps. Exiting.";
		return 1;
	}

	LOG(LogLevel::Info) << "Drawing international boundaries...";
	if (false == CreateInternationalBoundaries(ColourToId, Territories))
	{
		LOG(LogLevel::Error) << "Could not create international.dat . Exiting.";
		return 1;
	}

	LOG(LogLevel::Info) << "Creating mod information file...";
	if (false == CreateModTxt())
	{
		LOG(LogLevel::Error) << "Could not create mod.txt .";
		return 1;
	}

	if (false == Config.GetDefconPath().empty())
	{
		LOG(LogLevel::Info) << "Copying into the Defcon mod directory...";
		if (false == CopyModIntoDefcon())
		{
			LOG(LogLevel::Warning) << "Could not copy the mod into the Defcon directory.";
		}
	}

	LOG(LogLevel::Info) << "Complete!";
	return 0;
}
