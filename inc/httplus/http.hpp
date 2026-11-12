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
#ifndef HTTPLUS_HTTP_HPP_
#define HTTPLUS_HTTP_HPP_

#include "mkn/kul/except.hpp"
#include "mkn/kul/os.hpp"
#include "mkn/ram/http.hpp"
#include "mkn/ram/https.hpp"

#include "httplus/html.hpp"

#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace httplus::http {

class Exception : public mkn::kul::Exception {
 public:
  Exception(char const* f, std::uint16_t const& l, std::string const& s)
      : mkn::kul::Exception(f, l, s) {}
};

// One virtual host: static files under ${root}/pub, generators in `pages`, logs in ${root}/log
class Conf {
 public:
  Conf(std::filesystem::path const& root, Pages pages, std::string home = "",
       std::vector<std::string> text = {},
       std::unordered_map<std::string, std::string> headers = {});

  // the file under pub/ that `resource` names, if it exists and does not escape pub/
  std::optional<std::filesystem::path> file(std::string_view resource) const;

  void access(std::string_view line);
  void error(std::string_view line);

  std::filesystem::path const root, pub;
  std::string const home;
  std::vector<std::string> const text;  // file extensions served as text/plain
  std::unordered_map<std::string, std::string> const headers;
  Pages const pages;

 private:
  std::mutex _log_mutex;
  std::ofstream _acc, _err;
};

using Confs = std::unordered_map<std::string, std::shared_ptr<Conf>>;  // host -> conf, "" any host

Response respond(Request const& req, Confs const& confs);

class AServer {
 public:
  virtual ~AServer() = default;
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual void join() = 0;
  virtual std::exception_ptr exception() const = 0;

  // host "" serves requests whose Host matches no other conf
  void add(std::string const& host, std::shared_ptr<Conf> const& conf);
  Confs const& confs() const { return _confs; }

 protected:
  Confs _confs;
};

class Server : public httplus::http::AServer, public mkn::ram::http::MultiServer {
 public:
  Server(std::uint16_t const port, std::uint8_t const threads)
      : mkn::ram::http::MultiServer(port, threads, threads) {}

  void start() override { mkn::ram::http::MultiServer::start(); }
  void stop() override { mkn::ram::http::MultiServer::stop(); }
  void join() override { mkn::ram::http::MultiServer::join(); }
  std::exception_ptr exception() const override { return mkn::ram::http::MultiServer::exception(); }

 protected:
  Response respond(Request const& req) override { return http::respond(req, _confs); }
};

}  // namespace httplus::http

namespace httplus::https {

class Server : public httplus::http::AServer, public mkn::ram::https::MultiServer {
 public:
  Server(std::uint16_t const port, std::uint8_t const threads, mkn::kul::File const& crt,
         mkn::kul::File const& key, std::string const& ciphers = "",
         std::optional<mkn::kul::File> const& chain = std::nullopt)
      : mkn::ram::https::MultiServer(port, threads, threads, crt, key, ciphers) {
    init();
    if (chain) setChain(*chain);
  }

  void start() override { mkn::ram::https::MultiServer::start(); }
  void stop() override { mkn::ram::https::MultiServer::stop(); }
  void join() override { mkn::ram::https::MultiServer::join(); }
  std::exception_ptr exception() const override {
    return mkn::ram::https::MultiServer::exception();
  }

 protected:
  Response respond(Request const& req) override { return http::respond(req, _confs); }
};

}  // namespace httplus::https

#endif /* HTTPLUS_HTTP_HPP_ */
