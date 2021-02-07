/*
 *  Copyright (C) 2011 Leandro Nini
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef ARRAY_H
#define ARRAY_H

class counter {

private:
	unsigned int c;

public:
	counter() : c(1) {}
	void increase() { ++c; }
	unsigned int decrease() { return --c; }
};

template<typename T>
class array {

private:
	counter* count;
	unsigned int x, y;
	T* data;

public:
	array(const unsigned int x, const unsigned int y) :
		count(new counter()),
		x(x),
		y(y),
		data(new T[x * y]) {}

	array(const array& p) :
		count(p.count),
		x(p.x),
		y(p.y),
		data(p.data) { count->increase(); }

	~array() { if (count->decrease() == 0) { delete count; delete [] data; } }

	unsigned int length() const { return x * y; }

	T* operator[](unsigned int a) { return a<x ? &data[a * y] : 0; }

	T const* operator[](unsigned int a) const { return a<x ? &data[a * y] : 0; }
};

#endif
