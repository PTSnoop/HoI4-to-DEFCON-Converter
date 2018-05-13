#include "stdafx.h"
#include "GetEdges.h"

#include <iostream>
#include <algorithm>

GetEdges::GetEdges()
	: m_lastAngle(-1)
	, m_nudge(0.5)
	, m_angles({ 
		 std::make_pair(1, 0),
		 std::make_pair(1, 1),
		 std::make_pair(0, 1),
		 std::make_pair(-1, 1),
		 std::make_pair(-1, 0),
		 std::make_pair(-1, -1),
		 std::make_pair(0, -1),
		 std::make_pair(1, -1)
		 })
{
}

bool GetEdges::Init(CImg<unsigned char>* texture)
{
	m_texture = texture;
	return (NULL != m_texture);
}

GetEdges::~GetEdges()
{
}

std::tuple<int,int,int> GetEdges::GetPixelColor(int x, int y)
{
	if (!m_texture)
	{
		return std::tuple<int,int,int>(0,0,0);
	}
	
	if (x < 0 || y < 0)
	{
		return std::tuple<int, int, int>(0, 0, 0);
	}
	
	std::pair<int, int> size (m_texture->width(), m_texture->height());
	
	if (x >= size.first || y >= size.second)
	{
		return std::tuple<int, int, int>(0, 0, 0);
	}
	
	return std::tuple<int, int, int>(
		(*m_texture)(x, y, 0, 0),
		(*m_texture)(x, y, 0, 1),
		(*m_texture)(x, y, 0, 2)
		);
}

std::tuple<int,int,int> GetEdges::GetPixelColor(std::pair<double,double> pos)
{
	return GetPixelColor((int)floor(pos.first), (int)floor(pos.second));
}


bool GetEdges::Get(std::vector< std::pair<double, double> >& points, std::tuple<int, int, int> wantedColour)
{
	m_lastAngle = -1;
	std::pair<int, int> size = std::make_pair(m_texture->width(), m_texture->height());
	std::pair<int, int> firstpx (0, 0);
	
	bool found = false;
	
	for (firstpx.second = 0; firstpx.second < size.second; firstpx.second++)
	{
		for (firstpx.first = 0; firstpx.first < size.first; firstpx.first++)
		{
			if (GetPixelColor(firstpx) == wantedColour)
			{
				found = true;
				break;
			}
		}
		
		if (found)
		{
			break;
		}
	}
	
	if (!found)
	{
		return false;
	}
	
	std::pair<int,int> currentpx = firstpx;
	std::pair<int, int> nextpx = firstpx;
	int currentang = -1;
	
	std::vector< std::pair<int, int> > ourPoints;
	std::vector<int> ourAngs;
	
	int nextang = -1;
	
	do
	{
		bool found = false;
		bool ignore = false;
		
		for (int i = 0; i < 8; i++)
		{
			nextang = GetNextAngle();
			nextpx.first = currentpx.first + m_angles[nextang].first;
			nextpx.second = currentpx.second + m_angles[nextang].second;
			std::tuple<int,int,int> foundColour = GetPixelColor(nextpx);
			
			if (foundColour == wantedColour)
			{
				found = true;
				break;
			}
			else
			{
				// If we're bordering coastline, don't bother drawing this one.
				if (std::get<0>(foundColour) == 0 && std::get<1>(foundColour) == 0 && std::get<2>(foundColour) == 0)
				{
					ignore = true;
				}
				//borders.insert(foundColour);
			}
		}
		
		if (!found)
		{
			return false;    // Wait, how did you get here?
		}
		
		if (currentang != nextang)
		{
			if (ignore)
				ourPoints.push_back(std::make_pair(-1,-1));
			else
				ourPoints.push_back(currentpx);
			ourAngs.push_back(currentang);
		}
		
		currentpx = nextpx;
		currentang = nextang;
		SetLastAngle(currentang);
	}
	while (currentpx != firstpx);
	
	ourAngs[0] = nextang;
	
	if (m_nudge < 0.0005)
	{
		for (size_t i = 0; i < ourPoints.size(); i++)
		{
			points.push_back(std::pair<double,double>(ourPoints[i].first, ourPoints[i].second));
		}
		
		return true;
	}
	
	for (size_t i = 0; i < ourPoints.size(); i++)
	{
		if (ourPoints[i].first == -1 && ourPoints[i].second == -1)
		{
			points.push_back(std::pair<double, double>(-1,-1));
		}
		else
		{
			std::vector< std::pair<double, double> > nudges;
			GetNudge(nudges, ourAngs[(i) % ourAngs.size()], ourAngs[(i + 1) % ourAngs.size()]);

			for (std::pair<double, double> nudge : nudges)
			{
				std::pair<double, double> nudgedpx(ourPoints[i].first, ourPoints[i].second);
				nudgedpx.first += m_nudge * nudge.first;
				nudgedpx.second += m_nudge * nudge.second;
				points.push_back(nudgedpx);
			}
		}
	}
	
	return true;
}

void GetEdges::GetNudge(std::vector< std::pair<double, double> >& nudges, int ang1, int ang2)
{
	ang1 = ang1 % 8;
	ang2 = ang2 % 8;
	
	switch (ang1) // holy compound switch statement, Batman!
	{
	case 0: // right
		switch (ang2)
		{
		case 0: // right
			nudges.push_back(std::pair<double, double>(0, -1));
			break;
			
		case 1: // downright
			nudges.push_back(std::pair<double, double>(0, -1));
			break;
			
		case 2: // down
			nudges.push_back(std::pair<double, double>(1, -1));
			break;
			
		case 3: // downleft
			nudges.push_back(std::pair<double, double>(2, -1));
			break;
			
		case 4: // left
			nudges.push_back(std::pair<double, double>(0, -1));
			nudges.push_back(std::pair<double, double>(0, 1));
			break;
			
		case 5: // upleft
			nudges.push_back(std::pair<double, double>(2, 1));
			break;
			
		case 6: // up
			nudges.push_back(std::pair<double, double>(1, 1));
			break;
			
		case 7: // upright
			nudges.push_back(std::pair<double, double>(0, -1));
			break;
		}
		
		break;
		
	case 1: // downright
		switch (ang2)
		{
		case 0: // right
			nudges.push_back(std::pair<double, double>(0, -1));
			break;
			
		case 1: // downright
			nudges.push_back(std::pair<double, double>(0, 0));
			break;
			
		case 2: // down
			nudges.push_back(std::pair<double,double>(1, 0));
			break;
			
		case 3: // downleft
			nudges.push_back(std::pair<double,double>(1, 0));
			break;
			
		case 4: // left
			nudges.push_back(std::pair<double,double>(2, 1));
			break;
			
		case 5: // upleft
			nudges.push_back(std::pair<double,double>(0, 0)); // WAT
			break;
			
		case 6: // up
			nudges.push_back(std::pair<double,double>(1, 2));
			break;
			
		case 7: // upright
			nudges.push_back(std::pair<double,double>(0, 1));
			break;
		}
		
		break;
		
	case 2: // down
		switch (ang2)
		{
		case 0: // right
			nudges.push_back(std::pair<double,double>(-1, -1));
			break;
			
		case 1: // downright
			nudges.push_back(std::pair<double,double>(1, 0));
			break;
			
		case 2: // down
			nudges.push_back(std::pair<double,double>(1, 0)); // shouldn't be here anyway
			break;
			
		case 3: // downleft
			nudges.push_back(std::pair<double,double>(1, 0));
			break;
			
		case 4: // left
			nudges.push_back(std::pair<double,double>(1, 1));
			break;
			
		case 5: // upleft
			nudges.push_back(std::pair<double,double>(1, 2));
			break;
			
		case 6: // up
			nudges.push_back(std::pair<double,double>(1, 0));
			nudges.push_back(std::pair<double,double>(-1, 0));
			break;
			
		case 7: // upright
			nudges.push_back(std::pair<double,double>(-1, 2));
			break;
		}
		
		break;
		
	case 3: // downleft
		switch (ang2)
		{
		case 0: // right
			nudges.push_back(std::pair<double,double>(-2, 1));
			break;
			
		case 1: // downright
			nudges.push_back(std::pair<double,double>(-1, 0));
			break;
			
		case 2: // down
			nudges.push_back(std::pair<double,double>(1, 0));
			break;
			
		case 3: // downleft
			nudges.push_back(std::pair<double,double>(0, 0));
			break;
			
		case 4: // left
			nudges.push_back(std::pair<double,double>(0, 1));
			break;
			
		case 5: // upleft
			nudges.push_back(std::pair<double,double>(0, 1));
			break;
			
		case 6: // up
			nudges.push_back(std::pair<double,double>(-1, 2));
			break;
			
		case 7: // upright
			nudges.push_back(std::pair<double,double>(0, 0));
			break;
		}
		
		break;
		
	case 4: // left
		switch (ang2)
		{
		case 0: // right
			nudges.push_back(std::pair<double,double>(0, 1));
			nudges.push_back(std::pair<double,double>(0, -1));
			break;
			
		case 1: // downright
			nudges.push_back(std::pair<double,double>(-2, -1));
			break;
			
		case 2: // down
			nudges.push_back(std::pair<double,double>(-1, -1));
			break;
			
		case 3: // downleft
			nudges.push_back(std::pair<double,double>(0, 1));
			break;
			
		case 4: // left
			nudges.push_back(std::pair<double,double>(0, 0));
			break;
			
		case 5: // upleft
			nudges.push_back(std::pair<double,double>(0, 1));
			break;
			
		case 6: // up
			nudges.push_back(std::pair<double,double>(-1, 1));
			break;
			
		case 7: // upright
			nudges.push_back(std::pair<double,double>(-2, 1));
			break;
		}
		
		break;
		
	case 5: // upleft
		switch (ang2)
		{
		case 0: // right
			nudges.push_back(std::pair<double,double>(-2, -1));
			break;
			
		case 1: // downright
			nudges.push_back(std::pair<double,double>(0, 0)); // WAT
			break;
			
		case 2: // down
			nudges.push_back(std::pair<double,double>(-1, -2));
			break;
			
		case 3: // downleft
			nudges.push_back(std::pair<double,double>(0, -1));
			break;
			
		case 4: // left
			nudges.push_back(std::pair<double,double>(0, 1));
			break;
			
		case 5: // upleft
			nudges.push_back(std::pair<double,double>(0, 0));
			break;
			
		case 6: // up
			nudges.push_back(std::pair<double,double>(-1, 0));
			break;
			
		case 7: // upright
			nudges.push_back(std::pair<double,double>(-1, 0));
			break;
		}
		
		break;
		
	case 6: // up
		switch (ang2)
		{
		case 0: // right
			nudges.push_back(std::pair<double,double>(-1, -1));
			break;
			
		case 1: // downright
			nudges.push_back(std::pair<double,double>(-1, -2));
			break;
			
		case 2: // down
			nudges.push_back(std::pair<double,double>(-1, 0));
			nudges.push_back(std::pair<double,double>(1, 0));
			break;
			
		case 3: // downleft
			nudges.push_back(std::pair<double,double>(1, -2));
			break;
			
		case 4: // left
			nudges.push_back(std::pair<double,double>(1, -1));
			break;
			
		case 5: // upleft
			nudges.push_back(std::pair<double,double>(-1, 0));
			break;
			
		case 6: // up
			nudges.push_back(std::pair<double,double>(0, 0));
			break;
			
		case 7: // upright
			nudges.push_back(std::pair<double,double>(-1, 0));
			break;
		}
		
		break;
		
	case 7: // upright
		switch (ang2)
		{
		case 0: // right
			nudges.push_back(std::pair<double,double>(0, -1));
			break;
			
		case 1: // downright
			nudges.push_back(std::pair<double,double>(0, -1));
			break;
			
		case 2: // down
			nudges.push_back(std::pair<double,double>(1, -2));
			break;
			
		case 3: // downleft
			nudges.push_back(std::pair<double,double>(0, 0));
			break;
			
		case 4: // left
			nudges.push_back(std::pair<double,double>(2, -1));
			break;
			
		case 5: // upleft
			nudges.push_back(std::pair<double,double>(1, 0));
			break;
			
		case 6: // up
			nudges.push_back(std::pair<double,double>(-1, 0));
			break;
			
		case 7: // upright
			nudges.push_back(std::pair<double,double>(0, 0));
			break;
		}
		
		break;
		
	} // ...why did I think this was a good idea?
	
}

///////////////////

// This code should really be in a separate class, but meh

void GetEdges::SetLastAngle(int index)
{
	if (index >= (int)m_angles.size())
	{
		return;
	}
	
	m_lastAngle = (index + 4) % 8;
}

int GetEdges::GetNextAngle()
{
	m_lastAngle++;
	m_lastAngle %= 8;
	return m_lastAngle;
}

bool GetEdges::GetAllColours( std::set<std::tuple<int,int,int> >& colours)
{
	std::tuple<int, int, int> pixel(0, 0, 0);
	
	std::pair<int, int> size(m_texture->width(), m_texture->height());
	int iPixelCount = size.first * size.second;
	
	for (int y = 0; y < size.second; y++)
	{
		for (int x = 0; x < size.first; x++)
		{
			std::tuple<int, int, int> s(
				(*m_texture)(x, y, 0, 0), 
				(*m_texture)(x, y, 0, 1), 
				(*m_texture)(x, y, 0, 2));
			colours.insert(s);
		}
	}
	return true;
}