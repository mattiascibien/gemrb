/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Region.cpp,v 1.6 2003/12/14 09:17:17 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Region.h"

Region::Region(void)
{
	x = y = w = h = 0;
DEBUG=0;
}

Region::~Region(void)
{
}

/*Region::Region(Region & rgn)
{
	x = rgn.x;
	y = rgn.y;
	w = rgn.w;
	h = rgn.h;
DEBUG=rgn.DEBUG;
}*/

Region::Region(const Region & rgn)
{
  x = rgn.x;
  y = rgn.y;
  w = rgn.w;
  h = rgn.h;
DEBUG=rgn.DEBUG;
}

Region & Region::operator=(const Region & rgn)
{
	x = rgn.x;
	y = rgn.y;
	w = rgn.w;
	h = rgn.h;
DEBUG=rgn.DEBUG;
	return *this;
}

bool Region::operator==(const Region & rgn)
{
	if((x == rgn.x) && (y == rgn.y) && (w == rgn.w) && (h == rgn.h))
		return true;
	return false;
}

bool Region::operator!=(const Region & rgn)
{
	if((x != rgn.x) || (y != rgn.y) || (w != rgn.w) || (h != rgn.h))
		return true;
	return false;
}

Region::Region(int x, int y, int w, int h, int debug)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
this->DEBUG=DEBUG;
}
