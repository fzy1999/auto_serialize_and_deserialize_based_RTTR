/************************************************************************************
 *                                                                                   *
 *   Copyright (c) 2014 - 2018 Axel Menzel <info@rttr.org>                           *
 *                                                                                   *
 *   This file is part of RTTR (Run Time Type Reflection)                            *
 *   License: MIT License                                                            *
 *                                                                                   *
 *   Permission is hereby granted, free of charge, to any person obtaining           *
 *   a copy of this software and associated documentation files (the "Software"),    *
 *   to deal in the Software without restriction, including without limitation       *
 *   the rights to use, copy, modify, merge, publish, distribute, sublicense,        *
 *   and/or sell copies of the Software, and to permit persons to whom the           *
 *   Software is furnished to do so, subject to the following conditions:            *
 *                                                                                   *
 *   The above copyright notice and this permission notice shall be included in      *
 *   all copies or substantial portions of the Software.                             *
 *                                                                                   *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
 *   SOFTWARE.                                                                       *
 *                                                                                   *
 *************************************************************************************/

#include "myrttr/detail/comparison/compare_less.h"

#include "myrttr/detail/type/type_register_p.h"
#include "myrttr/type.h"
#include "myrttr/variant.h"

#include <type_traits>

namespace rttr {
namespace detail {

/////////////////////////////////////////////////////////////////////////////////////////

bool compare_types_less_than(const void* lhs, const void* rhs, const type& t, int& result)
{
  if (auto cmp_f = t.get_less_than_comparator()) {
    result = cmp_f->cmp(lhs, rhs) ? -1 : cmp_f->cmp(rhs, lhs) ? 1 : 0;
    return true;
  } else {
    return false;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////

}  // end namespace detail
}  // end namespace rttr