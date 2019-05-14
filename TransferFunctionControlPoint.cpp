#include "TransferFunctionControlPoint.h"

//Color control points
//Takes rgb color components that specify the color at the supplied isovalue
TransferFunctionControlPoint::TransferFunctionControlPoint(float r, float g, float b, int isoValue)
{
	color.r = r;
	color.g = g;
	color.b = b;
	color.w = 1.0f;
	this->isoValue = isoValue;
}


//Alpha Control Points
//Takes an alpha that specifies the alpha at the supplied isovalue
TransferFunctionControlPoint::TransferFunctionControlPoint(float alpha, int isoValue)
{
	color.r = 0.0f;
	color.g = 0.0f;
	color.b = 0.0f;
	color.w = alpha;
	this->isoValue = isoValue;
}

TransferFunctionControlPoint::~TransferFunctionControlPoint()
{

}