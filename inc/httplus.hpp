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
#ifndef HTTPLUS_HPP_
#define HTTPLUS_HPP_

#include "mkn/kul/except.hpp"

#include "httplus/html.hpp"
#include "httplus/http.hpp"
#include "httplus/yaml.hpp"

#include <cstdint>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <string>

namespace httplus {

class Exception : public mkn::kul::Exception {
 public:
  Exception(char const* f, std::uint16_t const& l, std::string const& s)
      : mkn::kul::Exception(f, l, s) {}
};

using Servers = std::map<std::uint16_t, std::shared_ptr<http::AServer>>;  // port -> server

class App {
 public:
  // reads httplus.yaml from `dir`
  explicit App(std::filesystem::path const& dir = std::filesystem::current_path())
      : config(yaml::Conf::CREATE(dir)) {}

  // creates a server per configured port; `sites` keys are root directories as written in
  // httplus.yaml, relative to the process working directory, or absolute
  App& load(Sites const& sites);

  App& start();
  App& stop();
  App& join();
  // the first server exception, if any
  std::exception_ptr exception() const;

  Servers const& servers() const { return _servers; }

 private:
  yaml::Conf config;
  Servers _servers;
};

}  // namespace httplus

#endif /* HTTPLUS_HPP_ */
