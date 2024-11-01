// ****************************************************************************************************
// File: ./Box.cpp
// Authors: R. Escassut
// Copyright (c) BULL S.A. 2000-2018, All Rights Reserved
//
// This file is part of Hurricane.
//
// Hurricane is free software: you can redistribute it  and/or  modify it under the  terms  of the  GNU
// Lesser General Public License as published by the Free Software Foundation, either version 3 of  the
// License, or (at your option) any later version.
//
// Hurricane is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without  even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A  PARTICULAR  PURPOSE. See  the  Lesser  GNU
// General Public License for more details.
//
// You should have received a copy of the Lesser GNU General Public License along  with  Hurricane.  If
// not, see <http://www.gnu.org/licenses/>.
// ****************************************************************************************************

#include "hurricane/Box.h"
#include "hurricane/Error.h"

namespace Hurricane {



// ****************************************************************************************************
// Box implementation
// ****************************************************************************************************

Box::Box()
// *******
    : _xMin(1),
    _yMin(1),
    _xMax(-1),
    _yMax(-1)
{}

Box::Box(const DbU::Unit& x, const DbU::Unit& y)
// ***********************************
    : _xMin(x),
    _yMin(y),
    _xMax(x),
    _yMax(y)
{}

Box::Box(const Point& point)
// *************************
    : _xMin(point.getX()),
    _yMin(point.getY()),
    _xMax(point.getX()),
    _yMax(point.getY())
{}

Box::Box(const DbU::Unit& x1, const DbU::Unit& y1, const DbU::Unit& x2, const DbU::Unit& y2)
// *********************************************************************
    : _xMin(min(x1, x2)),
    _yMin(min(y1, y2)),
    _xMax(max(x1, x2)),
    _yMax(max(y1, y2))
{}

Box::Box(const Point& point1, const Point& point2)
// ***********************************************
    : _xMin(min(point1.getX(), point2.getX())),
    _yMin(min(point1.getY(), point2.getY())),
    _xMax(max(point1.getX(), point2.getX())),
    _yMax(max(point1.getY(), point2.getY()))
{}

Box::Box(const Box& box)
// *********************
    : _xMin(box._xMin),
    _yMin(box._yMin),
    _xMax(box._xMax),
    _yMax(box._yMax)
{}

Box& Box::operator=(const Box& box)
// ********************************
{
    _xMin = box._xMin;
    _yMin = box._yMin;
    _xMax = box._xMax;
    _yMax = box._yMax;
    return *this;
}

bool Box::operator==(const Box& box) const
// ***************************************
{
    return (!isEmpty() &&
              !box.isEmpty() &&
              (_xMin == box._xMin) &&
              (_yMin == box._yMin) &&
              (_xMax == box._xMax) &&
              (_yMax == box._yMax));
}

bool Box::operator!=(const Box& box) const
// ***************************************
{
    return (isEmpty() ||
              box.isEmpty() ||
              (_xMin != box._xMin) ||
              (_yMin != box._yMin) ||
              (_xMax != box._xMax) ||
              (_yMax != box._yMax));
}

Box Box::getUnion(const Box& box) const
// ************************************
{
    if (isEmpty() && box.isEmpty()) return Box();
    return Box(min(_xMin, box._xMin),
                  min(_yMin, box._yMin),
                  max(_xMax, box._xMax),
                  max(_yMax, box._yMax));
}

Box Box::getIntersection(const Box& box) const
// *******************************************
{
    if (!intersect(box)) return Box();
    return Box(max(_xMin, box._xMin),
                  max(_yMin, box._yMin),
                  min(_xMax, box._xMax),
                  min(_yMax, box._yMax));
}

DbU::Unit Box::manhattanDistance(const Point& pt) const
// ***********************************************
{
    DbU::Unit dist = 0;
    if (isEmpty())
        throw Error("Can't compute distance to an empty Box");
    if (pt.getX() < _xMin) dist = _xMin - pt.getX();
    else if (pt.getX() > _xMax) dist = pt.getX() - _xMax;
    // else                     dist = 0;
    if (pt.getY() < _yMin) dist += _yMin - pt.getY();
    else if (pt.getY() > _yMax) dist += pt.getY() - _yMax;
    // else                     dist += 0;
    return dist;
}

DbU::Unit Box::manhattanDistance(const Box& box) const
// **********************************************
{
    if (isEmpty() || box.isEmpty())
        throw Error("Can't compute distance to an empty Box");
    DbU::Unit dx, dy;
    if ((dx=box.getXMin() - _xMax) < 0)
        if ((dx=_xMin-box.getXMax()) < 0) dx=0;
    if ((dy=box.getYMin() - _yMax) < 0)
        if ((dy=_yMin-box.getYMax()) < 0) dy=0;
    return dx+dy;
}

bool Box::isEmpty() const
// **********************
{
    return ((_xMax < _xMin) || (_yMax < _yMin));
}

bool Box::isFlat() const
// *********************
{
    return (!isEmpty() &&
              (((_xMin == _xMax) && (_yMin < _yMax)) ||
               ((_xMin < _xMax) && (_yMin == _yMax))));
}

bool Box::isPonctual() const
// *************************
{
    return (!isEmpty() && (_xMax == _xMin) && (_yMax == _yMin));
}

bool Box::contains(const DbU::Unit& x, const DbU::Unit& y) const
// ***************************************************
{
    return (!isEmpty() &&
              (_xMin <= x) &&
              (_yMin <= y) &&
              (x <= _xMax) &&
              (y <= _yMax));
}

bool Box::contains(const Point& point) const
// *****************************************
{
    return contains(point.getX(), point.getY());
}

bool Box::contains(const Box& box) const
// *************************************
{
    return (!isEmpty() &&
              !box.isEmpty() &&
              (_xMin <= box._xMin) &&
              (box._xMax <= _xMax) &&
              (_yMin <= box._yMin) &&
              (box._yMax <= _yMax));
}

bool Box::intersect(const Box& box) const
// **************************************
{
    return (!isEmpty() &&
              !box.isEmpty() &&
              !((_xMax < box._xMin) ||
                 (box._xMax < _xMin) ||
                 (_yMax < box._yMin) ||
                 (box._yMax < _yMin)));
}

bool Box::isConstrainedBy(const Box& box) const
// ********************************************
{
    return (!isEmpty() &&
              !box.isEmpty() &&
              ((_xMin == box.getXMin()) ||
                (_yMin == box.getYMin()) ||
                (_xMax == box.getXMax()) ||
                (_yMax == box.getYMax())));
}

Box& Box::makeEmpty()
// ******************
{
  _xMin = 1;
    _yMin = 1;
    _xMax = -1;
    _yMax = -1;
    return *this;
}

Box& Box::inflate(const DbU::Unit& d)
// *****************************
{
    return inflate(d, d, d, d);
}

Box& Box::inflate(const DbU::Unit& dx, const DbU::Unit& dy)
// **********************************************
{
    return inflate(dx, dy, dx, dy);
}

Box& Box::inflate(const DbU::Unit& dxMin, const DbU::Unit& dyMin, const DbU::Unit& dxMax, const DbU::Unit& dyMax)
// ******************************************************************************************
{
    if (!isEmpty()) {
        _xMin -= dxMin;
        _yMin -= dyMin;
        _xMax += dxMax;
        _yMax += dyMax;
    }
    return *this;
}

Box Box::getInflated(const DbU::Unit& d) const {
    return Box(*this).inflate(d);
}

Box& Box::shrinkByFactor(double factor)
// **************************************
{
    assert((0.0 <= factor) && (factor <= 1.0));
    DbU::Unit dx = DbU::grid ( 0.5 * (1-factor) * (DbU::getGrid(_xMax) - DbU::getGrid(_xMin)) );
    DbU::Unit dy = DbU::grid ( 0.5 * (1-factor) * (DbU::getGrid(_yMax) - DbU::getGrid(_yMin)) );

  //DbU::Unit dx=getUnit(0.5*(1- factor) * (getValue(_xMax) - getValue(_xMin)));
  //DbU::Unit dy=getUnit(0.5*(1- factor) * (getValue(_yMax) - getValue(_yMin)));
    return inflate(-dx, -dy);
}

Box& Box::merge(const DbU::Unit& x, const DbU::Unit& y)
// ******************************************
{
    if (isEmpty()) {
        _xMin = x;
        _yMin = y;
        _xMax = x;
        _yMax = y;
    }
    else {
        _xMin = min(_xMin, x);
        _yMin = min(_yMin, y);
        _xMax = max(_xMax, x);
        _yMax = max(_yMax, y);
    }
    return *this;
}

Box& Box::merge(const Point& point)
// ********************************
{
    return merge(point.getX(), point.getY());
}

Box& Box::merge(const DbU::Unit& x1, const DbU::Unit& y1, const DbU::Unit& x2, const DbU::Unit& y2)
// ****************************************************************************
{
    merge(x1, y1);
    merge(x2, y2);
    return *this;
}

Box& Box::merge(const Box& box)
// ****************************
{
    if (!box.isEmpty()) {
        merge(box.getXMin(), box.getYMin());
        merge(box.getXMax(), box.getYMax());
    }
    return *this;
}

Box& Box::translate(const DbU::Unit& dx, const DbU::Unit& dy)
// **********************************************************
{
    if (!isEmpty()) {
        _xMin += dx;
        _yMin += dy;
        _xMax += dx;
        _yMax += dy;
    }
    return *this;
}

Box& Box::translate(const Point& p)
// ********************************
{
    if (!isEmpty()) {
        _xMin += p.getX();
        _yMin += p.getY();
        _xMax += p.getX();
        _yMax += p.getY();
    }
    return *this;
}

string Box::_getString() const
// ***************************
{
  if (isEmpty())
    return "<" + _TName("Box") + " empty>";
  else
    return "<" + _TName("Box") + " "
      + DbU::getValueString(_xMin) + " " + DbU::getValueString(_yMin) + " "
      + DbU::getValueString(_xMax) + " " + DbU::getValueString(_yMax) + ">";
}

Record* Box::_getRecord() const
// **********************
{
    if (isEmpty()) return NULL;

    Record* record = new Record(getString(this));
    record->add(DbU::getValueSlot("XMin", &_xMin));
    record->add(DbU::getValueSlot("YMin", &_yMin));
    record->add(DbU::getValueSlot("XMax", &_xMax));
    record->add(DbU::getValueSlot("YMax", &_yMax));
    return record;
}

void  Box::toJson(JsonWriter* w) const
// ***********************************
{
  w->startObject();
  jsonWrite( w, "@typename", "Box" );
  jsonWrite( w, "_xMin", getXMin() );
  jsonWrite( w, "_yMin", getYMin() );
  jsonWrite( w, "_xMax", getXMax() );
  jsonWrite( w, "_yMax", getYMax() );
  w->endObject();
}


Initializer<JsonBox>  jsonBoxInit ( 0 );

void  JsonBox::initialize()
// **************************
{ JsonTypes::registerType( new JsonBox (JsonWriter::RegisterMode) ); }

JsonBox::JsonBox(unsigned long flags)
// **********************************
  : JsonObject(flags)
{ 
  add( "_xMin", typeid(int64_t) );
  add( "_yMin", typeid(int64_t) );
  add( "_xMax", typeid(int64_t) );
  add( "_yMax", typeid(int64_t) );
}

string  JsonBox::getTypeName() const
// *********************************
{ return "Box"; }

JsonBox* JsonBox::clone(unsigned long flags) const
// ***********************************************
{ return new JsonBox ( flags ); }

void JsonBox::toData(JsonStack& stack)
// ***********************************
{
  check( stack, "JsonBox::toData" );

  DbU::Unit xMin = DbU::fromDb(get<int64_t>(stack,"_xMin"));
  DbU::Unit yMin = DbU::fromDb(get<int64_t>(stack,"_yMin"));
  DbU::Unit xMax = DbU::fromDb(get<int64_t>(stack,"_xMax"));
  DbU::Unit yMax = DbU::fromDb(get<int64_t>(stack,"_yMax"));

  Box box;
  
  if ( (xMin <= xMax) and (yMin <= yMax) )
    box.merge( xMin, yMin, xMax, yMax ); 

  cdebug_log(19,0) << "Box(" << xMin << ", "
                 <<           yMin << ", "
                 <<           xMax << ", "
                 <<           yMax << ")" << endl;

  update( stack, box );
}

} // End of Hurricane namespace.


// ****************************************************************************************************
// Copyright (c) BULL S.A. 2000-2018, All Rights Reserved
// ****************************************************************************************************
