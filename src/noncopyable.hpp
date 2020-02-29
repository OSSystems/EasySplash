/*
 * EasySplash - tool for animated splash screens
 * Copyright (C) 2014  O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */


#ifndef EASYSPLASH_NONCOPYABLE_HPP
#define EASYSPLASH_NONCOPYABLE_HPP


namespace easysplash
{


namespace noncopyable_
{

class noncopyable
{
protected:
	noncopyable() {}
	~noncopyable() {}
private:
	noncopyable(noncopyable const &);
	noncopyable const & operator = (noncopyable const &);
};

}


typedef noncopyable_::noncopyable noncopyable;


}


#endif

