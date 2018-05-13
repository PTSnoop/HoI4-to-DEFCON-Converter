#pragma once

#include <vector>
#include <tuple>
#include <set>

#include "CImg.h"
using namespace cimg_library;

class GetEdges
{
public:
	GetEdges();
	bool Init(CImg<unsigned char>* texture);
	bool Get(std::vector< std::pair<double,double> >& points, std::tuple<int,int,int> wantedColour);
	bool GetAllColours(std::set< std::tuple<int, int, int> >& colours);
	
	float m_nudge;
	
	~GetEdges();

	std::tuple<int, int, int> GetPixelColor(std::pair<double,double> pos);
	std::tuple<int, int, int> GetPixelColor(int x, int y);
	
private:
	
	CImg<unsigned char>* m_texture;
	
	const std::vector< std::pair<int, int> > m_angles;
	
	void GetNudge(std::vector< std::pair<double,double> >& nudges, int ang1, int ang2);
	
	int m_lastAngle;
	void SetLastAngle(int lastAngle);
	int GetNextAngle();
	
};