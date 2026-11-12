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
#include "mkn/kul/log.hpp"

#include "html.hpp"
#include "httplus.hpp"

#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

namespace {

volatile std::sig_atomic_t interrupted = 0;

struct Result {
  std::uint16_t status = 0;
  std::string body, type;
};

template <class Get = mkn::ram::http::Get>
Result get(std::string const& host, std::string const& path, std::uint16_t const port) {
  Result result;
  Get(host, path, port)
      .withResponse([&](httplus::Response const& r) {
        result.status = r.status();
        result.body = r.body();
        if (auto const it = r.headers().find("Content-Type"); it != r.headers().end())
          result.type = it->second;
      })
      .send();
  return result;
}

template <class Get = mkn::ram::http::Get>
int check(std::string const& host, std::string const& path, std::uint16_t const status,
          std::string_view const contains, std::uint16_t const port = 8888) {
  auto const r = get<Get>(host, path, port);
  bool const ok = r.status == status && r.body.find(contains) != std::string::npos;
  std::cout << (ok ? "PASS " : "FAIL ") << host << ":" << port << "/" << path << " -> " << r.status
            << " " << r.type << std::endl;
  return ok ? 0 : 1;
}

int self_test() {
  std::ofstream("res/www/pub/hello.txt") << "hello world";
  int failures = 0;
  failures += check("localhost", "", 200, "SOME TEXT HERE");              // home page
  failures += check("localhost", "res/css.css", 200, "mark.red");         // generator
  failures += check("localhost", "hello.txt", 200, "hello world");        // static file
  failures += check("localhost", "hell%6F.txt", 200, "hello world");      // percent decoding
  failures += check("localhost", "nope", 404, "NOT FOUND - 404");         // 404 generator
  failures += check("localhost", "../log/access.log", 404, "NOT FOUND");  // traversal
  failures += check("127.0.0.1", "", 200, "SOME TEXT HERE");              // default host

  using HttpsGet = mkn::ram::https::Get;
  failures += check<HttpsGet>("localhost", "", 200, "SOME TEXT HERE", 8443);
  failures += check<HttpsGet>("localhost", "res/css.css", 200, "mark.red", 8443);
  failures += check<HttpsGet>("localhost", "hello.txt", 200, "hello world", 8443);
  failures += check<HttpsGet>("localhost", "nope", 404, "NOT FOUND - 404", 8443);
  failures += check<HttpsGet>("127.0.0.1", "", 404, "Unknown host", 8443);  // https host mandatory
  return failures;
}

}  // namespace

// without arguments: start, run the self test, stop
// --serve: run until interrupted
int main(int argc, char* argv[]) {
  bool const serve = argc > 1 && std::string_view(argv[1]) == "--serve";
  if (serve)  // self test keeps default handling so it can be interrupted or timed out
    for (auto const sig : {SIGINT, SIGTERM}) std::signal(sig, [](int) { interrupted = 1; });

  httplus::Pages pages{
      {"404", std::make_shared<NotFound>()},
      {"index", std::make_shared<Index>()},
      {"res/css.css", std::make_shared<CSS>()},
  };

  try {
    httplus::App app;
    app.load({{"res/www", pages}}).start();

    int code = 0;
    if (serve) {
      while (!interrupted && !app.exception())
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } else
      code = self_test();

    auto const e = app.exception();
    app.stop().join();
    if (e) std::rethrow_exception(e);
    return code;
  } catch (mkn::kul::Exception const& e) {
    KERR << e.stack();
    return 2;
  } catch (std::exception const& e) {
    KERR << e.what();
    return 3;
  } catch (...) {
    KERR << "UNKNOWN EXCEPTION CAUGHT";
    return 5;
  }
}
