/**
Copyright (c) 2016, Philip Deegan.
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
#include "httplus.hpp"

void httplus::App::load(
    kul::hash::map::S2T<std::shared_ptr<http::AServer>>& http_orS,
    Sites& sites) throw(httplus::Exception) {
  std::shared_ptr<http::Conf> defHttp;

  auto threads_lambda = [&](const YAML::Node& c) -> uint16_t {
    if (c["threads"]) return kul::String::UINT16(c["threads"].Scalar());
    if (config.root()["threads"])
      return kul::String::UINT16(config.root()["threads"].Scalar());
    return 1;
  };

  if (config.root()["http"])
    for (const YAML::Node& c : config.root()["http"]) {
      if (defHttp && !c["host"])
        KEXCEPTION("Only one http allowed without 'host' parameter");
      const std::string& port(c["port"] ? c["port"].Scalar() : "80");
      kul::Dir d(c["root"].Scalar());
      if (!d) KEXCEPTION("Directory does not exist: " + c["root"].Scalar());
      kul::Dir p("pub", d);
      if (!p && !p.mk()) KEXCEPTION("Invalid access on directory: " + d.real());
      std::string home(c["home"] ? c["home"].Scalar() : "");
      const std::string txt(c["text"] ? c["text"].Scalar() : "");
      const std::string hsh(std::to_string(std::hash<std::string>()(d.real())));
      if (!sites.count(hsh)) {
        KERR << "WARN: NO GENERATORS FOR HTTPS ROOT: " << d;
        continue;
      }
      const Pages& pages((*sites.find(hsh)).second);
      auto ser(std::make_shared<http::Server>(kul::String::UINT16(port),
                                              threads_lambda(c), pages));
      ser->confs.insert(c["host"].Scalar(), std::make_shared<http::Conf>(
                                                c["root"].Scalar(), home, txt));
      http_orS.insert(port, ser);
    }
  if (config.root()["https"])
    for (const YAML::Node& c : config.root()["https"]) {
      kul::Dir d(c["root"].Scalar());
      if (!d) KEXCEPTION("Directory does not exist: " + c["root"].Scalar());
      kul::Dir p("pub", d);
      if (!p && !p.mk()) KEXCEPTION("Invalid access on directory: " + d.real());
      const std::string& port(c["port"] ? c["port"].Scalar() : "443");
      const std::string hsh(std::to_string(std::hash<std::string>()(d.real())));
      if (!sites.count(hsh)) {
        KERR << "WARN: NO GENERATORS FOR HTTPS ROOT: " << d;
        continue;
      }
      const Pages& pages((*sites.find(hsh)).second);
      const std::string ssls([&] {
        if (c["ssl_cyphers"]) return c["ssl_cyphers"].Scalar();
        if (config.root()["ssl_cyphers"])
          return config.root()["ssl_cyphers"].Scalar();
        return std::string("");
      }());
      kul::File crt(c["crt"].Scalar());
      if (!crt) KEXCEPTION("File does not exist: " + crt.full());
      kul::File key(c["key"].Scalar());
      if (!key) KEXCEPTION("File does not exist: " + key.full());
      auto ser(std::make_shared<https::Server>(
          kul::String::UINT16(port), threads_lambda(c), pages, crt, key, ssls));
      ser->init();
      if (c["chain"]) {
        if (!kul::File(c["chain"].Scalar()))
          KEXCEPTION("File does not exist: " + c["chain"].Scalar());
        ser->setChain(c["chain"].Scalar());
      }
      std::string home(c["home"] ? c["home"].Scalar() : "");
      const std::string txt(c["text"] ? c["text"].Scalar() : "");
      ser->confs.insert(c["host"].Scalar(), std::make_shared<http::Conf>(
                                                c["root"].Scalar(), home, txt));
      http_orS.insert(port, ser);
    }
}