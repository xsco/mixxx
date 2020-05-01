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

#include <sqlite_modern_cpp.h>

#include <djinterop/djinterop.hpp>
#include <djinterop/enginelibrary/el_database_impl.hpp>
#include <djinterop/enginelibrary/schema.hpp>
#include <djinterop/impl/database_impl.hpp>
#include <djinterop/impl/util.hpp>
#include <djinterop/transaction_guard.hpp>

namespace djinterop
{
database::database(const database& db) = default;

database::~database() = default;

database& database::operator=(const database& db) = default;

transaction_guard database::begin_transaction() const
{
    return pimpl_->begin_transaction();
}

std::optional<crate> database::crate_by_id(int64_t id) const
{
    return from_boost_optional(pimpl_->crate_by_id(id));
}

std::vector<crate> database::crates() const
{
    return pimpl_->crates();
}

std::vector<crate> database::crates_by_name(const std::string& name) const
{
    return pimpl_->crates_by_name(name);
}

crate database::create_root_crate(std::string name) const
{
    return pimpl_->create_root_crate(name);
}

track database::create_track(std::string relative_path) const
{
    return pimpl_->create_track(relative_path);
}

std::string database::directory() const
{
    return pimpl_->directory();
}

bool database::is_supported() const
{
    return pimpl_->is_supported();
}

void database::verify() const
{
    pimpl_->verify();
}

void database::remove_crate(crate cr) const
{
    pimpl_->remove_crate(cr);
}

void database::remove_track(track tr) const
{
    pimpl_->remove_track(tr);
}

std::vector<crate> database::root_crates() const
{
    return pimpl_->root_crates();
}

std::optional<crate> database::root_crate_by_name(
    const std::string& name) const
{
    return from_boost_optional(pimpl_->root_crate_by_name(name));
}

std::optional<track> database::track_by_id(int64_t id) const
{
    return from_boost_optional(pimpl_->track_by_id(id));
}

std::vector<track> database::tracks() const
{
    return pimpl_->tracks();
}

std::vector<track> database::tracks_by_relative_path(
    const std::string& relative_path) const
{
    return pimpl_->tracks_by_relative_path(relative_path);
}

std::string database::uuid() const
{
    return pimpl_->uuid();
}

semantic_version database::version() const
{
    return pimpl_->version();
}

database::database(std::shared_ptr<database_impl> pimpl) :
    pimpl_{std::move(pimpl)}
{
}

}  // namespace djinterop
