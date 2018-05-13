#include "stdafx.h"
#include "Utils.h"

#include <cctype>
#include <iostream>

#include <boost/icl/type_traits/is_numeric.hpp>

#include "Log.h"

bool is_number(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

bool copyDir(
	boost::filesystem::path const & source,
	boost::filesystem::path const & destination
	)
{
	namespace fs = boost::filesystem;
	try
	{
		// Check whether the function call is valid
		if (
			!fs::exists(source) ||
			!fs::is_directory(source)
			)
		{
			LOG(LogLevel::Error) << "Source directory " << source.string()
				<< " does not exist or is not a directory.";
			return false;
		}
		if (fs::exists(destination))
		{
			LOG(LogLevel::Error) << "Destination directory " << destination.string()
				<< " already exists.";
			return false;
		}
		// Create the destination directory
		if (!fs::create_directory(destination))
		{
			LOG(LogLevel::Error) << "Unable to create destination directory"
				<< destination.string();
			return false;
		}
	}
	catch (fs::filesystem_error const & e)
	{
		LOG(LogLevel::Error) << e.what();
		return false;
	}
	// Iterate through the source directory
	for (
		fs::directory_iterator file(source);
		file != fs::directory_iterator(); ++file
		)
	{
		try
		{
			fs::path current(file->path());
			if (fs::is_directory(current))
			{
				// Found directory: Recursion
				if (
					!copyDir(
					current,
					destination / current.filename()
					)
					)
				{
					return false;
				}
			}
			else
			{
				// Found file: Copy
				fs::copy_file(
					current,
					destination / current.filename()
					);
			}
		}
		catch (fs::filesystem_error const & e)
		{
			LOG(LogLevel::Error) << e.what();
		}
	}
	return true;
}
