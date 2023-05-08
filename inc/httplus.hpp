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
#ifndef _HTTPLUS_HPP_
#define _HTTPLUS_HPP_

#include "mkn/kul/log.hpp"
#include "mkn/kul/threads.hpp"

#include "httplus/html.hpp"
#include "httplus/http.hpp"
#include "httplus/yaml.hpp"

namespace httplus {

class Exception : public mkn::kul::Exception {
 public:
  Exception(const char* f, const uint16_t& l, const std::string& s)
      : mkn::kul::Exception(f, l, s) {}
};

class App {
 private:
  httplus::yaml::Conf config;

 public:
  App() : config(httplus::yaml::Conf::CREATE()) {}
  void load(mkn::kul::hash::map::S2T<std::shared_ptr<http::AServer>>& http, Sites& sites)
      KTHROW(httplus::Exception);
};
}  // namespace httplus

#endif /* _HTTPLUS_HPP_ */