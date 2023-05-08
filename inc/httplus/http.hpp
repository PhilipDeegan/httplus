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
#ifndef _HTTPLUS_HTTP_HPP_
#define _HTTPLUS_HTTP_HPP_

#include "mkn/kul/io.hpp"
#include "mkn/kul/dbg.hpp"
#include "mkn/ram/https.hpp"

#include "httplus/def.hpp"
#include "httplus/html.hpp"

#ifdef _HTTPLUS_ACCEPT_GZIP_
#include <zlib.h>
#endif /* _HTTPLUS_ACCEPT_GZIP_ */

namespace httplus {

#ifdef _HTTPLUS_ACCEPT_GZIP_
class GzException : public mkn::kul::Exception {
 public:
  GzException(const char* f, const uint16_t& l, const std::string& s)
      : mkn::kul::Exception(f, l, s) {}
};

class Gzipper {
 public:
  static void COMPRESS(const std::string& in, std::string& out,
                       int16_t compression = Z_BEST_COMPRESSION) KTHROW(GzException) {
    z_stream zstr;
    memset(&zstr, 0, sizeof(zstr));
    if (deflateInit(&zstr, compression) != Z_OK) KEXCEPT(GzException, "zlib deflateInit failed");
    zstr.avail_in = in.size();
    zstr.next_in = (Bytef*)in.data();
    char buff[23456];
    int16_t e = 0;
    do {
      zstr.next_out = reinterpret_cast<Bytef*>(buff);
      zstr.avail_out = sizeof(buff);
      e = deflate(&zstr, Z_FINISH);
      if (out.size() < zstr.total_out) out.append(std::string(buff, zstr.total_out - out.size()));
    } while (e == Z_OK);
    deflateEnd(&zstr);
    if (e != Z_STREAM_END) {
      std::stringstream ss;
      ss << "zlib compression issue: (" << e << ") " << zstr.msg;
      KEXCEPT(GzException, ss.str());
    }
  }
};
#endif /* _HTTPLUS_ACCEPT_GZIP_ */

class App;
namespace https {
class Server;
}
namespace http {

class Exception : public mkn::kul::Exception {
 public:
  Exception(const char* f, const uint16_t& l, const std::string& s)
      : mkn::kul::Exception(f, l, s) {}
};

class Responder;
class Conf {
 protected:
  const std::string home, root;
  mkn::kul::File a, e;
  mkn::kul::io::Writer acc, err;
  std::vector<std::string> txt;

 public:
  Conf(const std::string& r, const std::string& h = "", const std::string& t = "")
      : home(h),
        root(r),
        a(mkn::kul::File("access.log", mkn::kul::Dir(r).join("log"))),
        e(mkn::kul::File("error.log", mkn::kul::Dir(r).join("log"))),
        acc(a, 1),
        err(e, 1) {
    mkn::kul::String::SPLIT(t, ' ', txt);
  }
  friend class Responder;
};
typedef mkn::kul::hash::map::S2T<std::shared_ptr<Conf>> Confs;

class Responder {
 private:
  mkn::kul::hash::map::S2S dlts;
  Responder() {
    dlts.insert("pdf", "application/pdf");
    dlts.insert("exe", "application/octet-stream");
    dlts.insert("zip", "application/zip");
    dlts.insert("doc", "application/msword");
    dlts.insert("xls", "application/vnd.ms-excel");
    dlts.insert("ppt", "application/vnd.ms-powerpoint");
    dlts.insert("gif", "image/gif");
    dlts.insert("png", "image/png");
    dlts.insert("jpeg", "image/jpg");
    dlts.insert("jpg", "image/jpg");
  }
  static Responder& INSTANCE() {
    static Responder i;
    return i;
  }
  const mkn::ram::http::Response& response(mkn::ram::http::Response& res,
                                           const mkn::ram::http::A1_1Request& req, const Pages& ps,
                                           const Confs& confs) KTHROW(httplus::http::Exception);

  friend class Server;
  friend class https::Server;
};

class AServer {
 protected:
  mkn::kul::hash::map::S2S _headers;

 public:
  virtual mkn::ram::http::Response respond(const mkn::ram::http::A1_1Request& req) = 0;
  virtual void stop() = 0;
  virtual void operator()() = 0;
};

class Server : public httplus::http::AServer, public mkn::ram::http::MultiServer {
  friend class mkn::kul::Thread;
  friend class httplus::App;
  friend class https::Server;

 private:
  const Pages& ps;
  Confs confs;
  void operator()() override { mkn::ram::http::MultiServer::start(); }
  mkn::ram::http::Response respond(const mkn::ram::http::A1_1Request& req) override {
    KUL_DBG_FUNC_ENTER
    mkn::ram::http::_1_1Response r;
    Responder::INSTANCE().response(r, req, ps, confs);
    return RESPONSE_HEADERS(r);
  }

 protected:
  static const mkn::ram::http::_1_1Response& RESPONSE_HEADERS(mkn::ram::http::_1_1Response& r) {
    if (!r.header("Date")) r.header("Date", mkn::kul::DateTime::NOW());
    if (!r.header("Connection")) r.header("Connection", "close");
    if (!r.header("Content-Type")) r.header("Content-Type", "text/html");
    if (!r.header("Content-Length")) r.header("Content-Length", std::to_string(r.body().size()));
    return r;
  }

 public:
  Server(const uint16_t& p, const uint16_t& threads, const Pages& ps)
      : mkn::ram::http::MultiServer(p, threads), ps(ps) {}
  void stop() override { mkn::ram::http::MultiServer::stop(); }
};
}  // end namespace http

namespace https {
class Server : public httplus::http::AServer, public mkn::ram::https::MultiServer {
  friend class mkn::kul::Thread;
  friend class httplus::App;

 private:
  const Pages& ps;
  http::Confs confs;
  void operator()() override { mkn::ram::https::MultiServer::start(); }
  mkn::ram::http::Response respond(const mkn::ram::http::A1_1Request& req) override {
    KUL_DBG_FUNC_ENTER
    mkn::ram::http::_1_1Response r;
    http::Responder::INSTANCE().response(r, req, ps, confs);
    return http::Server::RESPONSE_HEADERS(r);
  }

 public:
  Server(const uint16_t& p, const uint16_t& threads, const Pages& ps, const mkn::kul::File& crt,
         const mkn::kul::File& key, const std::string& cs = "")
      : mkn::ram::https::MultiServer(p, threads, crt, key, cs), ps(ps) {}
  void stop() override { mkn::ram::https::MultiServer::stop(); }
};

}  // end namespace https
}  // end namespace httplus
#endif /* _HTTPLUS_HTTP_HPP_ */
