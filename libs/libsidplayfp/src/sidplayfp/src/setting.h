/*
 * This file is part of sidplayfp, a SID player engine.
 *
 * Copyright 2025 Leandro Nini <drfiemost@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SETTING_H
#define SETTING_H

#include "sidcxx11.h"

#ifdef HAVE_CXX17

#include <optional>

template <typename T>
using Setting = std::optional<T>;

#else

#include <type_traits>

template <typename T>
class Setting
{
    static_assert(std::is_scalar<T>(), "T must be a scalar type");

private:
    T m_val;
    bool m_isSet;

public:
    Setting() :
        m_isSet(false) {}

    inline bool has_value() const { return m_isSet; }
    inline T value() const { return m_val; }
    inline Setting<T>& operator =(const T& val) { m_val = val; m_isSet = true;  return *this; }
};

#endif // HAVE_CXX17

#endif // SETTING_H
