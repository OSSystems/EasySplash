/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#include <assert.h>
#include <algorithm>
#include <type_traits>
#include <zlib.h>
#include <cstring>
#include "zip_archive.hpp"
#include "log.hpp"


namespace easysplash
{


namespace
{


std::string normalize_path(std::string const &p_path)
{
	// If the path starts with "./", remove this prefix
	std::string normalized_path = p_path;
	if (normalized_path.find("./") == 0)
		normalized_path = normalized_path.substr(2);
	return normalized_path;
}


template < typename T, class = typename std::enable_if < std::is_arithmetic < T > ::value > ::type >
std::istream& read_binary(std::istream &p_input, T &p_value)
{
	p_input.read(reinterpret_cast < char* >(&p_value), sizeof(T));
	return p_input;
}


bool inflate_data(datablock &p_compressed_data, datablock &p_uncompressed_data)
{
	z_stream stream;
	int ret;
	bool ok = true;

	memset(&stream, 0, sizeof(stream));
	stream.next_in   = reinterpret_cast < Bytef* > (&(p_compressed_data[0]));
	stream.avail_in  = p_compressed_data.size();
	stream.next_out  = reinterpret_cast < Bytef* > (&(p_uncompressed_data[0]));
	stream.avail_out = p_uncompressed_data.size();

	// -MAX_WBITS is necessary for deflated data in a zip
	if (ok && ((ret = inflateInit2(&stream, -MAX_WBITS)) != Z_OK))
	{
		LOG_MSG(error, "could not initialize zlib inflate: " << zError(ret));
		ok = false;
	}
	else
	{
		if (ok)
		{
			ret = inflate(&stream, Z_SYNC_FLUSH);
		
			if ((ret != Z_OK) && (ret != Z_STREAM_END))
			{
				LOG_MSG(error, "could not inflate data: " << zError(ret));
				ok = false;
			}
		}

		// uninitialize even if inflate() failed to properly clean up zlib resources
		if ((ret = inflateEnd(&stream) != Z_OK))
		{
			LOG_MSG(error, "error while cleaning up zlib inflate: " << zError(ret));
			ok = false;
		}
	}

	return ok;
}


} // unnamed namespace end


zip_archive::zip_archive(std::istream &p_input)
	: m_input(p_input)
{
	read_central_directory();
}


zip_archive::entries const & zip_archive::get_entries() const
{
	return m_entries;
}


void zip_archive::find_entries_with_path(std::string const &p_path, entry_iter_callback const &p_callback)
{
	std::string path = normalize_path(p_path);
	for (auto const &pair : m_entries)
	{
		entry const &p_entry = pair.second;
		if (p_entry.m_filename.find(path) == 0)
		{
			LOG_MSG(trace, "entry filename \"" << p_entry.m_filename << "\" starts with path \"" << path << "\" - invoking callback");
			p_callback(p_entry);
		}
		else
			LOG_MSG(trace, "entry filename \"" << p_entry.m_filename << "\" does not start with path \"" << path << "\" - skipping");
	}
}


datablock zip_archive::uncompress_data(entry const &p_entry)
{
	datablock uncompressed_data(p_entry.m_uncompressed_size);

	if (p_entry.m_offset_to_data == 0)
	{
		uint16_t local_filename_len, local_extra_field_len;

		// go to local header, and skip signature, required version, bit flag, compression
		// method, last mod time & date, crc32, compressed & uncompressed size (all of this
		// data is already present in the central directory)
		m_input.seekg(p_entry.m_offset_to_local_header + 4 + 5*2 + 3*4, std::ios::beg);

		// skip local filename and extra fields
		read_binary(m_input, local_filename_len);
		read_binary(m_input, local_extra_field_len);
		m_input.seekg(local_filename_len + local_extra_field_len, std::ios::cur);

		p_entry.m_offset_to_data = m_input.tellg();
	}
	else
		m_input.seekg(p_entry.m_offset_to_data, std::ios::beg);

	switch (p_entry.m_compression_method)
	{
		case 0: // no compression
		{
			m_input.read(reinterpret_cast < char* > (&uncompressed_data[0]), uncompressed_data.size());
			break;
		}

		case 8: // deflate compression
		{
			datablock compressed_data(p_entry.m_compressed_size);
			m_input.read(reinterpret_cast < char* > (&compressed_data[0]), compressed_data.size());

			if (!inflate_data(compressed_data, uncompressed_data))
			{
				LOG_MSG(error, "could not decompress data - returning empty datablock");
				return datablock();
			}

			break;
		};

		default:
			// should not happen - compression methods other than 0 or 8 are filtered earlier
			assert(0);
	}

	return uncompressed_data;
}


void zip_archive::read_central_directory()
{
	std::streamoff end_of_central_dir_pos = find_end_of_central_directory();

	if (end_of_central_dir_pos == -1)
	{
		LOG_MSG(error, "could not find end of central directory");
		return;
	}

	// go to end of central directory, and skip:
	// signature (4 byte), disk nr (2 byte), disk nr. with start of central dir (2 byte),
	// number of entries on this disk (2 byte)
	m_input.seekg(end_of_central_dir_pos + 4 + 3*2, std::ios::beg);

	std::uint16_t num_entries;
	std::uint32_t central_dir_size;
	std::uint32_t central_dir_pos;

	read_binary(m_input, num_entries);
	read_binary(m_input, central_dir_size);
	read_binary(m_input, central_dir_pos);

	scan_central_directory(num_entries, central_dir_size, central_dir_pos);
}


void zip_archive::scan_central_directory(std::uint16_t const p_num_entries, std::uint32_t const p_central_dir_size, std::uint32_t const p_central_dir_pos)
{
	m_input.seekg(p_central_dir_pos, std::ios::beg);

	for (std::uint16_t i = 0; i < p_num_entries; ++i)
	{
		entry new_entry;
		std::uint16_t fn_length, extra_field_length, file_comment_length;

		m_input.seekg(4 + 3*2, std::ios::cur); // skipping signature, version, required version, bit flag
		read_binary(m_input, new_entry.m_compression_method);
		m_input.seekg(2*2, std::ios::cur); // skipping last mod file time & date
		read_binary(m_input, new_entry.m_CRC32_checksum);
		read_binary(m_input, new_entry.m_compressed_size);
		read_binary(m_input, new_entry.m_uncompressed_size);
		read_binary(m_input, fn_length);
		read_binary(m_input, extra_field_length);
		read_binary(m_input, file_comment_length);
		m_input.seekg(2*2 + 4, std::ios::cur); // skipping disk nr. start, internal & external file attributes
		read_binary(m_input, new_entry.m_offset_to_local_header);

		// copy the filename
		new_entry.m_filename.resize(fn_length);
		m_input.read(&(new_entry.m_filename[0]), new_entry.m_filename.size());
		new_entry.m_filename = normalize_path(new_entry.m_filename);

		// skip extra field & file comment so we are at the next file header in the central directory
		m_input.seekg(extra_field_length + file_comment_length, std::ios::cur);

		// check compression method
		// checking here, to ensure filename, comments etc have been read/skipped
		// and the input position is at the right place for reading the next header
		if ((new_entry.m_compression_method != 0) && (new_entry.m_compression_method != 8))
		{
			LOG_MSG(warning, "zip entry \"" << new_entry.m_filename << "\" uses unsupported compression method " << new_entry.m_compression_method << " - skipping this entry");
			continue;
		}

		// the offset is computed on-demand; 0 means "not computed yet"
		new_entry.m_offset_to_data = 0;

		m_entries[new_entry.m_filename] = new_entry;

		// check if we are outside of the bounds of the central directory
		assert((std::uint32_t(m_input.tellg()) - p_central_dir_pos) <= p_central_dir_size);
	}
}


std::streamoff zip_archive::find_end_of_central_directory()
{
	m_input.seekg(0, std::ios::end);
	std::streamsize filesize = m_input.tellg();
	m_input.seekg(0, std::ios::beg);

	if (filesize < 4)
	{
		LOG_MSG(error, "zip file is too small - must be at least 4 byte in size");
		return -1;
	}

	std::uint32_t const end_of_central_dir_signature = 0x06054b50;

	std::streamoff max_back_read = 0xffff, backwards_pos = 4, found_pos = -1;
	std::streamsize const max_read_size = 0x0400;

	uint8_t tempbuf[max_read_size];

	if (max_back_read > filesize)
		max_back_read = filesize;

	// Search backwards for the end-of-central directory signature.
	// If the signature isnt found, go backwards by max_back_read bytes,
	// and look again. Do so until the signature is found, or until the
	// limit is reached (to prevent it from looking through the entire file).
	while (true)
	{
		std::streamoff read_pos = filesize - backwards_pos;
		std::streamsize read_size = std::min(std::streamsize(backwards_pos), max_read_size);

		m_input.seekg(read_pos, std::ios::beg);
		m_input.read(reinterpret_cast < char* > (&(tempbuf[0])), read_size);

		for (std::streamoff i = read_size - 4; i >= 0; --i)
		{
			std::uint32_t test_sig =
				(std::uint32_t(tempbuf[i + 3]) << 24) |
				(std::uint32_t(tempbuf[i + 2]) << 16) |
				(std::uint32_t(tempbuf[i + 1]) << 8) |
				(std::uint32_t(tempbuf[i + 0]) << 0);

			if (test_sig == end_of_central_dir_signature)
			{
				found_pos = read_pos + i;
				break;
			}
		}

		if (found_pos != -1)
			break;

		if (backwards_pos == max_back_read)
			break;

		backwards_pos += max_read_size;
		if (backwards_pos > max_back_read)
			backwards_pos = max_back_read;
	}

	return found_pos;
}




zip_archive::entry const * find_entry_by_name(zip_archive const &p_archive, std::string const &p_name)
{
	auto iter = p_archive.get_entries().find(normalize_path(p_name));
	return (iter == p_archive.get_entries().end()) ? NULL : &(iter->second);
}


} // namespace easysplash end
