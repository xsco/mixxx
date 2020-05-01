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
#ifndef DJINTEROP_EXCEPTIONS_HPP
#define DJINTEROP_EXCEPTIONS_HPP

#if __cplusplus < 201703L
#error This library needs at least a C++17 compliant compiler
#endif

#include <djinterop/semantic_version.hpp>


namespace djinterop
{
/// The `database_inconsistency` exception is thrown when the schema of a
/// database does not match the expectations suggested by its reported version
/// number.
class database_inconsistency : public std::runtime_error
{
public:
    explicit database_inconsistency(const std::string& what_arg) noexcept
        : runtime_error{what_arg}
    {
    }
};

/// The `unsupported_database_version` exception is thrown when a database
/// schema version is encountered that is not yet supported by this version of
/// the library.
class unsupported_database_version : public std::runtime_error
{
public:
    explicit unsupported_database_version(const semantic_version version)
        noexcept : runtime_error{"Unsupported database version"},
                   version_{version}
    {
    }

    explicit unsupported_database_version(
        const std::string& what_arg, const semantic_version version) noexcept
        : runtime_error{what_arg},
          version_{version}
    {
    }

    const semantic_version version() const { return version_; }

private:
    semantic_version version_;
};

/// The `crate_deleted` exception is thrown when an invalid `crate` object is
/// used, i.e. one that does not exist in the database anymore.
class crate_deleted : public std::runtime_error
{
public:
    /// Constructs the exception for a given crate ID
    explicit crate_deleted(int64_t id) noexcept
        : runtime_error{"Crate does not exist in database anymore"},
          id_{id}
    {
    }

    /// Returns the crate ID that was deemed non-existent
    int64_t id() const noexcept { return id_; }

private:
    int64_t id_;
};

/// The `crate_database_inconsistency` exception is thrown when a database
/// inconsistency is found that correlates to a crate.
class crate_database_inconsistency : public database_inconsistency
{
public:
    /// Construct the exception for a given crate ID
    explicit crate_database_inconsistency(
        const std::string& what_arg, int64_t id) noexcept
        : database_inconsistency{what_arg.c_str()},
          id_{id}
    {
    }

    /// Get the crate ID that was deemed inconsistent
    int64_t id() const noexcept { return id_; }

private:
    int64_t id_;
};

/// The `crate_database_inconsistency` exception is thrown when a database
/// inconsistency is found that correlates to a crate.
class crate_invalid_name : public std::runtime_error
{
public:
    /// Construct the exception for a given crate name
    explicit crate_invalid_name(const std::string& what_arg, std::string name)
        noexcept : runtime_error{what_arg.c_str()},
                   name_{name}
    {
    }

    /// Get the name that was deemed invalid
    std::string name() const noexcept { return name_; }

private:
    std::string name_;
};

/// The `track_deleted` exception is thrown when an invalid `track` object is
/// used, i.e. one that does not exist in the database anymore.
class track_deleted : public std::invalid_argument
{
public:
    /// Constructs the exception for a given track ID
    explicit track_deleted(int64_t id) noexcept
        : invalid_argument{"Track does not exist in database"},
          id_{id}
    {
    }

    /// Returns the track ID that was found to be non-existent
    int64_t id() const noexcept { return id_; }

private:
    int64_t id_;
};

/// The `track_database_inconsistency` exception is thrown when a database
/// inconsistency is found that correlates to a track.
class track_database_inconsistency : public database_inconsistency
{
public:
    /// Construct the exception for a given track ID
    explicit track_database_inconsistency(
        const std::string& what_arg, int64_t id) noexcept
        : database_inconsistency{what_arg},
          id_{id}
    {
    }

    /// Get the track ID that is the subject of this exception
    int64_t id() const noexcept { return id_; }

private:
    int64_t id_;
};

}  // namespace djinterop

#endif  // DJINTEROP_EXCEPTIONS_HPP
