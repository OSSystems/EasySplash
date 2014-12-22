/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#ifndef EASYSPLASH_ZIP_ARCHIVE_HPP
#define EASYSPLASH_ZIP_ARCHIVE_HPP

#include <string>
#include <iostream>
#include <cstdint>
#include <functional>
#include <map>

#include "noncopyable.hpp"
#include "types.hpp"


namespace easysplash
{


class zip_archive
	: private noncopyable
{
public:
	struct entry
	{
		std::string m_filename;
		std::uint16_t m_compression_method;
		std::uint32_t m_CRC32_checksum;
		std::uint32_t m_compressed_size;
		std::uint32_t m_uncompressed_size;
		std::uint32_t m_offset_to_local_header;
		mutable std::uint32_t m_offset_to_data;
	};

	// By using std::map instead of std::unordered_map for the entries,
	// it is guaranteed they are sorted lexicographically by their filenames,
	// since map sorts the keys in a strict weak order
	typedef std::map < std::string, entry > entries;
	typedef std::function < void(entry const &p_entry) > entry_iter_callback;


	explicit zip_archive(std::istream &p_input);

	entries const & get_entries() const;
	void find_entries_with_path(std::string const &p_path, entry_iter_callback const &p_callback);

	// rvalue references and return value optimization
	// get rid of unnecessary datablock copies
	datablock uncompress_data(entry const &p_entry);


private:
	void read_central_directory();
	void scan_central_directory(std::uint16_t const p_num_entries, std::uint32_t const central_dir_size, std::uint32_t const central_dir_pos);
	std::streamoff find_end_of_central_directory();

	std::istream &m_input;
	entries m_entries;
};


zip_archive::entry const * find_entry_by_name(zip_archive const &p_archive, std::string const &p_name);


} // namespace easysplash end


#endif
