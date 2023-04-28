/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2017 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2014-2022 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2014-2019 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2011-2022 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE.nxcomp which comes in the       */
/* source distribution.                                                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

#ifndef Types_H
#define Types_H

using namespace std;

#include <vector>
#include <list>
#include <map>
#include <set>

#include "MD5.h"

//
// This is MD5 length.
//

#define MD5_LENGTH          16

//
// Types of repositories. Replace the original
// clear() methods from STL in order to actually
// free the unused memory.
//

class Message;

class T_data : public vector < unsigned char >
{
  public:

  unsigned char *begin()
  {
    return &*(vector < unsigned char >::begin());
  }

  const unsigned char *begin() const
  {
    return &*(vector < unsigned char >::begin());
  }

  // Avoid overriding clear() when using libc++. Fiddling with STL internals
  // doesn't really seem like a good idea to me anyway.
  #ifndef _LIBCPP_VECTOR
  void clear()
  {
    #if defined(__STL_USE_STD_ALLOCATORS) || defined(__GLIBCPP_INTERNAL_VECTOR_H)

    #if defined(__GLIBCPP_INTERNAL_VECTOR_H)

    _Destroy(_M_start, _M_finish);

    #else /* #if defined(__GLIBCPP_INTERNAL_VECTOR_H) */

    destroy(_M_start, _M_finish);

    #endif /* #if defined(__GLIBCPP_INTERNAL_VECTOR_H) */

    _M_deallocate(_M_start, _M_end_of_storage - _M_start);

    _M_start = _M_finish = _M_end_of_storage = 0;

    #else /* #if defined(__STL_USE_STD_ALLOCATORS) || defined(__GLIBCPP_INTERNAL_VECTOR_H) */

    #if defined(_GLIBCXX_VECTOR)

    _Destroy(this->_M_impl._M_start, this->_M_impl._M_finish);

    _M_deallocate(this->_M_impl._M_start, this->_M_impl._M_end_of_storage - this->_M_impl._M_start);

    this->_M_impl._M_start = this->_M_impl._M_finish = this->_M_impl._M_end_of_storage = 0;

    #else /* #if defined(_GLIBCXX_VECTOR) */

    destroy(start, finish);

    deallocate();

    start = finish = end_of_storage = 0;

    #endif /* #if defined(_GLIBCXX_VECTOR) */

    #endif  /* #if defined(__STL_USE_STD_ALLOCATORS) || defined(__GLIBCPP_INTERNAL_VECTOR_H) */
  }
  #endif /* #ifdef _LIBCPP_VECTOR */
};

class T_messages : public vector < Message * >
{
  public:

  // Avoid overriding clear() when using libc++. Fiddling with STL internals
  // doesn't really seem like a good idea to me anyway.
  #ifndef _LIBCPP_VECTOR
  void clear()
  {
    #if defined(__STL_USE_STD_ALLOCATORS) || defined(__GLIBCPP_INTERNAL_VECTOR_H)

    #if defined(__GLIBCPP_INTERNAL_VECTOR_H)

    _Destroy(_M_start, _M_finish);

    #else /* #if defined(__GLIBCPP_INTERNAL_VECTOR_H) */

    destroy(_M_start, _M_finish);

    #endif /* #if defined(__GLIBCPP_INTERNAL_VECTOR_H) */

    _M_deallocate(_M_start, _M_end_of_storage - _M_start);

    _M_start = _M_finish = _M_end_of_storage = 0;

    #else /* #if defined(__STL_USE_STD_ALLOCATORS) || defined(__GLIBCPP_INTERNAL_VECTOR_H) */

    #if defined(_GLIBCXX_VECTOR)

    _Destroy(this->_M_impl._M_start, this->_M_impl._M_finish);

    _M_deallocate(this->_M_impl._M_start, this->_M_impl._M_end_of_storage - this->_M_impl._M_start);

    this->_M_impl._M_start = this->_M_impl._M_finish = this->_M_impl._M_end_of_storage = 0;

    #else /* #if defined(_GLIBCXX_VECTOR) */

    destroy(start, finish);

    deallocate();

    start = finish = end_of_storage = 0;

    #endif /* #if defined(_GLIBCXX_VECTOR) */

    #endif /* #if defined(__STL_USE_STD_ALLOCATORS) || defined(__GLIBCPP_INTERNAL_VECTOR_H) */
  }
  #endif /* #ifndef _LIBCPP_VECTOR */
};

typedef md5_byte_t * T_checksum;

struct T_less
{
  bool operator()(T_checksum a, T_checksum b) const
  {
    return (memcmp(a, b, MD5_LENGTH) < 0);
  }
};

typedef map < T_checksum, int, T_less > T_checksums;

class Split;

typedef list < Split * > T_splits;

class File;

struct T_older
{
  bool operator()(File *a, File *b) const;
};

typedef set < File *, T_older > T_files;

typedef list < int > T_list;

//
// Used to accommodate data to be read and
// written to a socket.
//

typedef struct
{
  T_data data_;
  int    length_;
  int    start_;
}
T_buffer;

//
// The message store operation that was
// executed for the message. The channels
// use these values to determine how to
// handle the message after it has been
// received at the decoding side.
//

// Since ProtoStep8 (#issue 108)
enum T_store_action
{
  is_hit,
  is_added,
  is_discarded,
  is_removed
};

// Since ProtoStep8 (#issue 108)
#define IS_HIT is_hit
#define IS_ADDED is_added

enum T_checksum_action
{
  use_checksum,
  discard_checksum
};

enum T_data_action
{
  use_data,
  discard_data
};

//
// Message is going to be weighted for
// deletion at insert or cleanup time?
//

enum T_rating
{
  rating_for_insert,
  rating_for_clean
};

//
// How to handle the writes to the X
// and proxy connections.
//

enum T_write
{
  write_immediate,
  write_delayed
};

enum T_flush
{
  flush_if_needed,
  flush_if_any
};

//
// This is the value to indicate an
// invalid position in the message
// store.
//

static const int nothing = -1;

#endif /* Types_H */
