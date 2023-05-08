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
#ifndef _HTTPLUS_YAML_HPP_
#define _HTTPLUS_YAML_HPP_

#include "mkn/kul/yaml.hpp"

#include "httplus/html.hpp"

namespace httplus {
namespace yaml {

class Exception : public mkn::kul::Exception {
 public:
  Exception(const char* f, const uint16_t& l, const std::string& s)
      : mkn::kul::Exception(f, l, s) {}
};

class Conf : public mkn::kul::yaml::File {
 private:
  const mkn::kul::Dir d;

 protected:
  Conf(const mkn::kul::Dir d) : mkn::kul::yaml::File(d.join("httplus.yaml")), d(d) {}
  static Conf CREATE(const mkn::kul::Dir& d) {
    mkn::kul::File f("httplus.yaml", d);
    if (!f.is()) KEXCEPTION("httplus,yaml does not exist:\n" + f.full());
    return mkn::kul::yaml::File::CREATE<Conf>(d.path());
  }

 public:
  Conf(const Conf& p) : mkn::kul::yaml::File(p), d(p.d) {}
  const mkn::kul::Dir& dir() const { return d; }
  const mkn::kul::yaml::Validator validator() const {
    using namespace mkn::kul::yaml;

    NodeValidator sys("system",
                      {
                          NodeValidator("threads"),
                          NodeValidator("max_request_bytes"),
                          NodeValidator("ssl_cyphers"),
                      },
                      0, NodeType::MAP);

    NodeValidator http_headers("header", {NodeValidator("*")}, 0, NodeType::MAP);
    NodeValidator http(
        "http",
        {NodeValidator("root", 1), NodeValidator("text"), NodeValidator("host"),
         NodeValidator("port"), NodeValidator("home"), NodeValidator("threads"), http_headers},
        0, NodeType::LIST);
    NodeValidator https("https",
                        {NodeValidator("root", 1), NodeValidator("text"), NodeValidator("host", 1),
                         NodeValidator("port"), NodeValidator("crt", 1), NodeValidator("key", 1),
                         NodeValidator("chain"), NodeValidator("home"), NodeValidator("threads"),
                         NodeValidator("ssl_cyphers"), http_headers},
                        0, NodeType::LIST);
    return Validator({sys, http, https});
  }
  static Conf CREATE() { return Conf::CREATE(mkn::kul::Dir(mkn::kul::env::CWD())); }
  friend class mkn::kul::yaml::File;
};
}  // namespace yaml
}  // namespace httplus
#endif /* _HTTPLUS_YAML_HPP_ */
