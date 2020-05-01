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
#ifndef DJINTEROP_ENGINELIBRARY_HPP
#define DJINTEROP_ENGINELIBRARY_HPP

#if __cplusplus < 201703L
#error This library needs at least a C++17 compliant compiler
#endif

#include <djinterop/pad_color.hpp>
#include <djinterop/semantic_version.hpp>


namespace djinterop
{
class database;
struct beatgrid_marker;

namespace enginelibrary
{
static constexpr semantic_version version_1_6_0{1, 6, 0};
static constexpr semantic_version version_1_7_1{1, 7, 1};
static constexpr semantic_version version_latest = version_1_7_1;

namespace standard_pad_colors
{
constexpr const pad_color pad_1{0xEA, 0xC5, 0x32, 0xFF};
constexpr const pad_color pad_2{0xEA, 0x8F, 0x32, 0xFF};
constexpr const pad_color pad_3{0xB8, 0x55, 0xBF, 0xFF};
constexpr const pad_color pad_4{0xBA, 0x2A, 0x41, 0xFF};
constexpr const pad_color pad_5{0x86, 0xC6, 0x4B, 0xFF};
constexpr const pad_color pad_6{0x20, 0xC6, 0x7C, 0xFF};
constexpr const pad_color pad_7{0x00, 0xA8, 0xB1, 0xFF};
constexpr const pad_color pad_8{0x15, 0x8E, 0xE2, 0xFF};

constexpr const std::array<pad_color, 8> pads{pad_1, pad_2, pad_3, pad_4,
                                              pad_5, pad_6, pad_7, pad_8};

}  // namespace standard_pad_colors

constexpr const char* default_database_dir_name = "Engine Library";

/// Creates a new, empty database in a directory using the schema version
/// provided.
///
/// By convention, the last part of the directory path is "Engine Library".  If
/// a database already exists in the target directory, an exception will be
/// thrown.
database create_database(
    std::string directory,
    const semantic_version& schema_version = version_latest);

/// Create or load an Engine Library database in a given directory.
///
/// If a database already exists in the directory, it will be loaded.  If not,
/// it will be created at the specified schema version.  In both cases, the
/// database is returned.  The boolean reference parameter `created` can be used
/// to determine whether the database was created or merely loaded.
database create_or_load_database(
    std::string directory, const semantic_version& schema_version,
    bool& created);

/// Returns a boolean indicating whether an Engine Library already exists in a
/// given directory.
bool database_exists(const std::string& directory);

/// Loads an Engine Library database from a given directory.
database load_database(std::string directory);

/// Given an Engine Library database, returns the path to its m.db sqlite
/// database file
///
/// If the given database is not an enginelibrary, then the behaviour of this
/// function is undefined.
std::string music_db_path(const database& db);

/// Normalizes a beat-grid, so that the beat indexes are in the form normally
/// expected by Engine Prime.
///
/// By convention, the Engine Prime analyses tracks so that the first beat is
/// at index -4 (yes, negative!) and the last beat is the first beat past the
/// usable end of the track, which may not necessarily be aligned to the first
/// beat of a 4-beat bar.  Therefore, the sample offsets typically recorded by
/// Engine Prime do not lie within the actual track.
std::vector<beatgrid_marker> normalize_beatgrid(
    std::vector<beatgrid_marker> beatgrid, int64_t sample_count);

/// Given an enginelibrary database, returns the path to its p.db sqlite
/// database file
///
/// If the given database is not an enginelibrary, then the behaviour of this
/// function is undefined.
std::string perfdata_db_path(const database& db);

}  // namespace enginelibrary
}  // namespace djinterop

#endif  // DJINTEROP_ENGINELIBRARY_HPP
