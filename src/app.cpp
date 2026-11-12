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
#include "httplus.hpp"

#include "mkn/kul/log.hpp"
#include "mkn/kul/string.hpp"

#include <sstream>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string scalar(YAML::Node const& node, std::string const& key, std::string const& def = "") {
  if (!node) return def;
  auto const value = node[key];
  return value ? value.Scalar() : def;
}

std::uint8_t threads(YAML::Node const& entry, YAML::Node const& system) {
  auto const s = scalar(entry, "threads", scalar(system, "threads", "1"));
  auto const t = mkn::kul::String::UINT16(s);
  if (t < 1 || t > 255) KEXCEPT(httplus::Exception, "threads must be between 1 and 255: " + s);
  return static_cast<std::uint8_t>(t);
}

std::vector<std::string> words(std::string const& s) {
  std::vector<std::string> v;
  std::istringstream ss(s);
  for (std::string w; ss >> w;) v.emplace_back(w);
  return v;
}

std::unordered_map<std::string, std::string> headers(YAML::Node const& node) {
  std::unordered_map<std::string, std::string> hs;
  if (node)
    for (auto const& kv : node) hs.emplace(kv.first.Scalar(), kv.second.Scalar());
  return hs;
}

fs::path resolve(fs::path const& dir, std::string const& p) {
  fs::path const path(p);
  return path.is_relative() ? dir / path : path;
}

}  // namespace

httplus::App& httplus::App::load(Sites const& sites) {
  std::unordered_map<std::string, Pages const*> roots;  // canonical root -> pages
  for (auto const& [dir, pages] : sites) {
    std::error_code ec;
    auto const root = fs::canonical(dir, ec);
    if (ec) KEXCEPT(Exception, "Site directory does not exist: " + dir);
    roots.emplace(root.string(), &pages);
  }

  auto const& yaml = config.root();
  auto const system = yaml["system"];

  auto const conf = [&](YAML::Node const& c) {
    auto const root = resolve(config.dir(), c["root"].Scalar());
    if (!fs::is_directory(root)) KEXCEPT(Exception, "Directory does not exist: " + root.string());
    static Pages const none;
    Pages const* pages = &none;
    if (auto const it = roots.find(fs::canonical(root).string()); it != roots.end())
      pages = it->second;
    else
      KOUT(NON) << "WARN: no generators for root, serving files only: " << root;
    return std::make_shared<http::Conf>(root, *pages, scalar(c, "home"), words(scalar(c, "text")),
                                        headers(c["header"]));
  };

  if (auto const nodes = yaml["http"])
    for (auto const& c : nodes) {
      auto const port = mkn::kul::String::UINT16(scalar(c, "port", "80"));
      auto& server = _servers[port];
      if (!server)
        server = std::make_shared<http::Server>(port, threads(c, system));
      else if (!dynamic_cast<http::Server*>(server.get()))
        KEXCEPT(Exception, "Port used for both http and https: " + std::to_string(port));
      server->add(scalar(c, "host"), conf(c));
    }

  if (auto const nodes = yaml["https"])
    for (auto const& c : nodes) {
      auto const port = mkn::kul::String::UINT16(scalar(c, "port", "443"));
      if (_servers.count(port))
        KEXCEPT(Exception, "Only one https entry allowed per port: " + std::to_string(port));
      mkn::kul::File const crt(resolve(config.dir(), c["crt"].Scalar()).string());
      if (!crt) KEXCEPT(Exception, "File does not exist: " + crt.full());
      mkn::kul::File const key(resolve(config.dir(), c["key"].Scalar()).string());
      if (!key) KEXCEPT(Exception, "File does not exist: " + key.full());
      std::optional<mkn::kul::File> chain;
      if (c["chain"]) {
        chain.emplace(resolve(config.dir(), c["chain"].Scalar()).string());
        if (!*chain) KEXCEPT(Exception, "File does not exist: " + chain->full());
      }
      auto const ciphers = scalar(c, "ssl_cyphers", scalar(system, "ssl_cyphers"));
      auto server =
          std::make_shared<https::Server>(port, threads(c, system), crt, key, ciphers, chain);
      server->add(c["host"].Scalar(), conf(c));
      _servers.emplace(port, server);
    }

  return *this;
}

httplus::App& httplus::App::start() {
  for (auto const& [port, server] : _servers) server->start();
  return *this;
}

httplus::App& httplus::App::stop() {
  for (auto const& [port, server] : _servers) server->stop();
  return *this;
}

httplus::App& httplus::App::join() {
  for (auto const& [port, server] : _servers) server->join();
  return *this;
}

std::exception_ptr httplus::App::exception() const {
  for (auto const& [port, server] : _servers)
    if (auto e = server->exception()) return e;
  return nullptr;
}

httplus::yaml::Conf httplus::yaml::Conf::CREATE(std::filesystem::path const& dir) {
  auto const file = dir / "httplus.yaml";
  if (!fs::is_regular_file(file)) KEXCEPTION("httplus.yaml does not exist: " + file.string());
  return mkn::kul::yaml::File::CREATE<Conf>(file.string());
}

mkn::kul::yaml::Validator httplus::yaml::Conf::validator() const {
  using namespace mkn::kul::yaml;

  NodeValidator const system("system",
                             {
                                 NodeValidator("threads"),
                                 NodeValidator("max_request_bytes"),
                                 NodeValidator("ssl_cyphers"),
                             },
                             0, NodeType::MAP);

  NodeValidator const headers("header", {NodeValidator("*")}, 0, NodeType::MAP);
  NodeValidator const http(
      "http",
      {NodeValidator("root", 1), NodeValidator("text"), NodeValidator("host"),
       NodeValidator("port"), NodeValidator("home"), NodeValidator("threads"), headers},
      0, NodeType::LIST);
  NodeValidator const https(
      "https",
      {NodeValidator("root", 1), NodeValidator("text"), NodeValidator("host", 1),
       NodeValidator("port"), NodeValidator("crt", 1), NodeValidator("key", 1),
       NodeValidator("chain"), NodeValidator("home"), NodeValidator("threads"),
       NodeValidator("ssl_cyphers"), headers},
      0, NodeType::LIST);
  return Validator({system, http, https});
}
