/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_REFLECT_H
#define _FNORD_REFLECT_H
#include "fnord/reflect/metaclass.h"

namespace fnord {
namespace reflect {

template <class T>
MetaClass<T>* reflect();

template <typename ClassType, typename ReturnType, typename... ArgTypes>
MethodCall<ClassType, ReturnType, ArgTypes...> reflectMethod(
    ReturnType (ClassType::* method_fn)(ArgTypes...)) {
  return MethodCall<ClassType, ReturnType, ArgTypes...>(method_fn);
}

template <class T>
void reflect(MetaClass<T>* service);

}
}

#include "reflect_impl.h"
#endif