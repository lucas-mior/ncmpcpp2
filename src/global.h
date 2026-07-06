/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#ifndef NCMPCPP_GLOBAL_H
#define NCMPCPP_GLOBAL_H

#include <chrono>
#include <stddef.h>
#include <random>
#include <string>

#include "mpdpp.h"

namespace NC {
struct Window;
}

namespace Global {

// header window (above main window)
extern NC::Window *wHeader;

// footer window (below main window)
extern NC::Window *wFooter;

// Y coordinate of top of main window
extern size_t MainStartY;

// height of main window
extern size_t MainHeight;

// indicates whether seeking action in currently in progress
extern bool SeekingInProgress;

// string that represents volume in right top corner. being global
// to be used for calculating width offsets in various files.
extern std::string VolumeState;

// global timer
extern std::chrono::steady_clock::time_point Timer;

// global RNG
extern std::mt19937 RNG;

}

#endif // NCMPCPP_GLOBAL_H
