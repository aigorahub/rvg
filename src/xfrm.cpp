#include <Rcpp.h>
#include "xfrm.h"
#include <iostream>

using namespace Rcpp;

std::string xfrm::xml()
{
  std::stringstream os;

  if( rot == 0 )
    os << "<a:xfrm>";
  else {
    os << "<a:xfrm rot=\"" << (int)(this->rot*60000) << "\">";
  }
  os << "<a:off x=\"" << (int)(this->x * 12700) << "\" y=\"" << (int)(this->y * 12700) << "\"/>";

  // The following lines are here to fix bug of pptx file with negative lengths of rectangles and lines
  // This is a quick-and-dirty solution, which should be handled in some more sophisticated way

  int width = (int)(this->width * 12700);
  width = width >= 0 ? width : 10;
  int height = (int)(this->height * 12700);
  height = height >= 0 ? height : 10;

  os << "<a:ext cx=\"" << width << "\" cy=\"" << height << "\"/>";
  os << "</a:xfrm>";
  return os.str();
}

xfrm::xfrm (double x, double y,
    double width, double height, double rot ):
  x(x),
  y(y),
  width(width),
  height(height),
  rot(rot) {
}
xfrm::xfrm (NumericVector x, NumericVector y ){
  this->x = min(x);
  this->y = min(y);
  this->width = max(x) - this->x;
  this->height = max(y) - min(y);
  this->rot = 0.0;
}

