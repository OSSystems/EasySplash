/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * Please refer to the LICENSE file included in the source code for
 * licensing terms.
 */


#ifndef EASYSPLASHLRU_CACHE_HPP
#define EASYSPLASHLRU_CACHE_HPP

#include <list>
#include <unordered_map>
#include <functional>
#include <assert.h>


namespace easysplash
{


template < typename Key, typename T >
class lru_cache
{
public:
	typedef std::function < void(Key const &p_key, T & p_value) > unload_function;

	explicit lru_cache(std::size_t const p_max_entries, unload_function const &p_unload_function = unload_function())
		: m_max_entries(p_max_entries)
		, m_unload_function(p_unload_function)
	{
		assert(p_max_entries > 0);
	}

	T* get_entry(Key const &p_key)
	{
		auto map_iter = m_entry_map.find(p_key);
		if (map_iter == m_entry_map.end())
			return nullptr;

		auto &list_iter = map_iter->second;

		if (list_iter != m_entry_list.begin())
		{
			m_entry_list.splice(m_entry_list.begin(), m_entry_list, list_iter);
			list_iter = m_entry_list.begin();
			map_iter->second = list_iter;
		}

		return &(list_iter->second);
	}

	T& add_entry(Key const &p_key, T const & p_value)
	{
		m_entry_list.push_front(entry_list::value_type(p_key, p_value));
		m_entry_map[p_key] = m_entry_list.begin();
		trim_list();
		return m_entry_list.front()->second;
	}

	T& add_entry(Key const &p_key, T && p_value)
	{
		m_entry_list.push_front(typename entry_list::value_type(p_key, std::move(p_value)));
		m_entry_map[p_key] = m_entry_list.begin();
		trim_list();
		return m_entry_list.front().second;
	}

private:
	void trim_list()
	{
		while (m_entry_list.size() > m_max_entries)
		{
			auto list_iter = m_entry_list.end();
			list_iter--;

			if (m_unload_function)
				m_unload_function(list_iter->first, list_iter->second);

			auto map_iter = m_entry_map.find(list_iter->first);
			if (map_iter != m_entry_map.end())
				m_entry_map.erase(map_iter);

			m_entry_list.erase(list_iter);
		}
	}

	typedef std::list < std::pair < Key, T > > entry_list;
	typedef std::unordered_map < Key, typename entry_list::iterator > entry_map;

	std::size_t m_max_entries;
	entry_list m_entry_list;
	entry_map m_entry_map;

	unload_function m_unload_function;
};


} // namespace easysplash end


#endif
