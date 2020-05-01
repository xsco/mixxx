/*
    This file is part of libdjinterop.

    libdjinterop is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libdjinterop is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libdjinterop.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef DJINTEROP_PERFORMANCE_DATA_HPP
#define DJINTEROP_PERFORMANCE_DATA_HPP

#if __cplusplus < 201103L && _MSVC_LANG < 201103L
#error This library needs at least a C++11 compliant compiler
#endif

#include <cstdint>
#include <string>

#include <djinterop/pad_color.hpp>


namespace djinterop
{
struct sampling_info
{
    double sample_rate = 0;  // usually 44100.0 or 48000.0
    int64_t sample_count = 0;
};

struct beatgrid_marker
{
    int32_t index = 0;
    double sample_offset = 0;
};

struct hot_cue
{
    std::string label;
    double sample_offset = 0;
    pad_color color;
};

struct loop
{
    std::string label;
    double start_sample_offset = 0;
    double end_sample_offset = 0;
    pad_color color;
};

struct waveform_point
{
    uint8_t value = 0;
    uint8_t opacity = 255;
};

/**
 * A single high-resolution waveform entry
 *
 * Note that, when rendering the high-resolution waveform, each individual
 * band is scaled so that the largest value across the entire waveform hits the
 * top of the display.  Note also that the mid frequency is always drawn over
 * the low, and the high frequency is always drawn over the low and mid, meaning
 * that very loud high-frequency sounds will hide any low or mid activity on the
 * waveform rendering.
 *
 * A further note is that when the opacity is set to zero, this appears to
 * translate into roughly 50% opacity on a real rendering.
 */
struct waveform_entry
{
    waveform_point low;
    waveform_point mid;
    waveform_point high;
};

}  // namespace djinterop

#endif  // DJINTEROP_PERFORMANCE_DATA_HPP
