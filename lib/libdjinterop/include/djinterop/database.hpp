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
#ifndef DJINTEROP_DATABASE_HPP
#define DJINTEROP_DATABASE_HPP

#if __cplusplus < 201703L
#error This library needs at least a C++17 compliant compiler
#endif

#include <memory>
#include <optional>
#include <vector>


namespace sqlite
{
class database;
}

namespace djinterop
{
class crate;
class database_impl;
class semantic_version;
class track;
class transaction_guard;

class database_not_found : public std::runtime_error
{
public:
    explicit database_not_found(const std::string& what_arg) noexcept
        : runtime_error{what_arg}
    {
    }
};

class database
{
public:
    /// Copy constructor
    database(const database& db);

    /// Destructor
    ~database();

    /// Copy assignment operator
    database& operator=(const database& db);

    transaction_guard begin_transaction() const;

    /// Returns the crate with the given ID
    ///
    /// If no such crate exists in the database, then `djinterop::std::nullopt`
    /// is returned.
    std::optional<crate> crate_by_id(int64_t id) const;

    /// Returns all crates contained in the database
    std::vector<crate> crates() const;

    /// Returns all crates with the given name
    std::vector<crate> crates_by_name(const std::string& name) const;

    /// Creates a new root crate with the given name.
    ///
    /// The created crate has no parent.
    crate create_root_crate(std::string name) const;

    /// Creates a new track associated to a given music file
    ///
    /// The music file is given by its relative path from the directory passed
    /// to the `database` constructor. The created track is not contained in any
    /// crates.
    track create_track(std::string relative_path) const;

    /// Returns the path directory of the database
    ///
    /// This is the same as the directory passed to the `database` constructor.
    std::string directory() const;

    /// Returns true iff the database version is supported by this version of
    /// `libdjinterop` or not
    bool is_supported() const;

    /// Returns the UUID of the database
    std::string uuid() const;

    /// Verifies the consistence of the internal storage of the database.
    ///
    /// A `database_inconsistency` (or some exception derived from it) is thrown
    /// if any kind of inconsistency is found.
    void verify() const;

    /// Returns the schema version of the database
    semantic_version version() const;

    /// Removes a crate from the database
    ///
    /// All handles to that crate become invalid.
    void remove_crate(crate cr) const;

    /// Removes a track from the database
    ///
    /// All handles to that track become invalid.
    void remove_track(track tr) const;

    /// Returns the root-level crate with the given name.
    ///
    /// If no such crate exists, then `djinterop::std::nullopt` is returned.
    std::optional<crate> root_crate_by_name(const std::string& name) const;

    /// Returns all root crates contained in the database
    ///
    /// A root crate is a crate that has no parent.
    std::vector<crate> root_crates() const;

    /// Returns the track with the given id
    ///
    /// If no such track exists in the database, then `djinterop::std::nullopt`
    /// is returned.
    std::optional<track> track_by_id(int64_t id) const;

    /// Returns all tracks whose `relative_path` attribute in the database
    /// matches the given string
    std::vector<track> tracks_by_relative_path(
        const std::string& relative_path) const;

    /// Returns all tracks contained in the database
    std::vector<track> tracks() const;

    // TODO (haslersn): non public?
    database(std::shared_ptr<database_impl> pimpl);

private:
    std::shared_ptr<database_impl> pimpl_;
};

}  // namespace djinterop

#endif  // DJINTEROP_DATABASE_HPP
