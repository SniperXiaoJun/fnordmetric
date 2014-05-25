/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * Licensed under the MIT license (see LICENSE).
 */
#ifndef _FNORDMETRIC_FILEBACKEND_STREAMREF_H
#define _FNORDMETRIC_FILEBACKEND_STREAMREF_H

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <memory>

namespace fnordmetric {
namespace filebackend {

/**
 * This is an internal class. For usage instructions and extended documentation
 * please refer to "storagebackend.h" and "filebackend.h"
 */
class StreamRef {
public:
  explicit StreamRef(
      uint64_t stream_id,
      const std::string& stream_key);

  StreamRef(const StreamRef& copy) = delete;
  StreamRef& operator=(const StreamRef& copy) = delete;

protected:
  const uint64_t stream_id_;
  const std::string stream_key_;
};

}
}
#endif
