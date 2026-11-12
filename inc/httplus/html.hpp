/**
Copyright (c) 2023, Philip Deegan.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Philip Deegan nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef HTTPLUS_HTML_HPP_
#define HTTPLUS_HTML_HPP_

#include "mkn/ram/http.hpp"
#include "mkn/ram/html4.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace httplus {

using Request = mkn::ram::http::A1_1Request;
using Response = mkn::ram::http::Response;

// A generator registered under a resource key. The registered instance is a prototype: every
// request renders a fresh instance from clone(), so pre/render/post may mutate freely.
class Page : public mkn::ram::html4::Page {
 public:
  virtual ~Page() = default;
  virtual std::shared_ptr<Page> clone() const = 0;
  virtual void pre(Request const& /*req*/) {}
  virtual void post(Request const& /*req*/, Response& /*res*/) {}
};

// Implements clone() by default constructing Derived: `class Index : public PageT<Index> {...}`
// or, to share a common base, `class Index : public PageT<Index, Head> {...}`
template <class Derived, class Base = Page>
class PageT : public Base {
 public:
  std::shared_ptr<Page> clone() const override { return std::make_shared<Derived>(); }
};

// Thrown from pre/render/post to let the page repair itself, the page is then rendered again
class XXXError {
 public:
  virtual ~XXXError() = default;
  virtual void recover(Page& page) = 0;
};

using Pages = std::unordered_map<std::string, std::shared_ptr<Page>>;  // resource -> generator
using Sites = std::unordered_map<std::string, Pages>;                  // root directory -> pages

}  // namespace httplus

#endif /* HTTPLUS_HTML_HPP_ */
