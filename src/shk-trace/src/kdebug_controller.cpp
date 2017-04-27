/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#include "kdebug_controller.h"

#include <bitset>

#include <errno.h>

#include "syscall_constants.h"

namespace shk {
namespace {

int kdebugFilterIndex(int klass, int subclass) {
  return ((klass & 0xff) << 8) | (subclass & 0xff);
}


class RealKdebugController : public KdebugController {
 public:
  void start(int nbufs) override {
    setNumbufs(nbufs);
    setup();
    setFilter();
  }

  virtual kbufinfo_t getBufinfo() override {
    kbufinfo_t ret;
    size_t len = sizeof(ret);
    static int name[] = { CTL_KERN, KERN_KDEBUG, KERN_KDGETBUF, 0, 0, 0 };
    if (sysctl(name, 3, &ret, &len, 0, 0)) {
      throw std::runtime_error("Failed KERN_KDGETBUF sysctl");
    }
    return ret;
  }

  virtual void enable(bool enabled) override {
    static int name[] = { CTL_KERN, KERN_KDEBUG, KERN_KDENABLE, enabled, 0, 0 };
    if (sysctl(name, 4, nullptr, nullptr, nullptr, 0)) {
      throw std::runtime_error("Failed KERN_KDENABLE sysctl");
    }
  }

  virtual void teardown() override {
    static int name[] = { CTL_KERN, KERN_KDEBUG, KERN_KDREMOVE, 0, 0, 0 };
    if (sysctl(name, 3, nullptr, nullptr, nullptr, 0) < 0) {
      if (errno == EBUSY) {
        throw std::runtime_error("Kdebug tracing is already in use");
      } else {
        throw std::runtime_error("Failed KERN_KDREMOVE sysctl");
      }
    }
  }

  virtual size_t readBuf(kd_buf *bufs, size_t num_bufs) override {
    size_t count = num_bufs * sizeof(kd_buf);
    static int name[] = { CTL_KERN, KERN_KDEBUG, KERN_KDREADTR, 0, 0, 0 };
    if (sysctl(name, 3, bufs, &count, nullptr, 0)) {
      throw std::runtime_error("Failed KERN_KDREADTR sysctl");
    }
    return count;
  }

 private:
  void setNumbufs(int nbufs) {
    static size_t len = 0;
    static int name_1[] = { CTL_KERN, KERN_KDEBUG, KERN_KDSETBUF, nbufs, 0, 0 };
    if (sysctl(name_1, 4, nullptr, &len, nullptr, 0)) {
      throw std::runtime_error("Failed KERN_KDSETBUF sysctl");
    }

    static int name_2[] = { CTL_KERN, KERN_KDEBUG, KERN_KDSETUP, 0, 0, 0 };
    if (sysctl(name_2, 3, nullptr, &len, nullptr, 0)) {
      throw std::runtime_error("Failed KERN_KDSETUP sysctl");
    }
  }

  void setFilter() {
    std::bitset<KDBG_TYPEFILTER_BITMAP_SIZE * 8> filter{};

    filter.set(kdebugFilterIndex(DBG_TRACE, DBG_TRACE_DATA));
    filter.set(kdebugFilterIndex(DBG_TRACE, DBG_TRACE_STRING));
    filter.set(kdebugFilterIndex(DBG_MACH, DBG_MACH_EXCP_SC));
    filter.set(kdebugFilterIndex(DBG_FSYSTEM, DBG_FSRW));
    filter.set(kdebugFilterIndex(DBG_BSD, DBG_BSD_EXCP_SC));
    filter.set(kdebugFilterIndex(DBG_BSD, DBG_BSD_PROC));
    filter.set(kdebugFilterIndex(FILEMGR_CLASS, 0)); // Carbon File Manager
    filter.set(kdebugFilterIndex(FILEMGR_CLASS, 1)); // Carbon File Manager

    static int name[] = { CTL_KERN, KERN_KDEBUG, KERN_KDSET_TYPEFILTER };
    size_t len = filter.size() / 8;
    if (sysctl(name, 3, &filter, &len, nullptr, 0)) {
      throw std::runtime_error("Failed KERN_KDSET_TYPEFILTER sysctl");
    }
  }

  void setup() {
    kd_regtype kr;

    kr.type = KDBG_RANGETYPE;
    kr.value1 = 0;
    kr.value2 = -1;
    size_t len = sizeof(kd_regtype);

    static int name_1[] = { CTL_KERN, KERN_KDEBUG, KERN_KDSETREG, 0, 0, 0 };
    if (sysctl(name_1, 3, &kr, &len, nullptr, 0)) {
      throw std::runtime_error("Failed KERN_KDSETREG sysctl");
    }

    static int name_2[] = { CTL_KERN, KERN_KDEBUG, KERN_KDSETUP, 0, 0, 0 };
    if (sysctl(name_2, 3, nullptr, nullptr, nullptr, 0)) {
      throw std::runtime_error("Failed KERN_KDSETUP sysctl");
    }
  }
};

}  // anonymous namespace

std::unique_ptr<KdebugController> makeKdebugController() {
  return std::unique_ptr<KdebugController>(new RealKdebugController());
}

}  // namespace shk
