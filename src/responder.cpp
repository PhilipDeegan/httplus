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
#include "httplus/http.hpp"

const kul::http::Response& httplus::http::Responder::response(
    kul::http::Response& res, const kul::http::A1_1Request& req,
    const Pages& ps, const Confs& confs) throw(httplus::http::Exception) {
  KUL_DBG_FUNC_ENTER
  const std::string& resource(req.path());

  http::Conf* conf = 0;
  if (req.headers().count("Host")) {
    std::vector<std::string> bits;
    std::string host((*req.headers().find("Host")).second);
    kul::String::SPLIT(host, ':', bits);
    if (bits.size() > 1) host = bits[0];
    if (confs.count(host)) conf = (*confs.find(host)).second.get();
  }
  std::string ct = "text/html; charset=utf-8";
  bool e = 0;
  if (conf) {
    std::string r(resource.substr(1));
    if (r.empty() && conf->home.size()) r = conf->home;
    const kul::Dir root(conf->root);
    const kul::Dir log(root.join("log"), 1);
    const kul::Dir pub(root.join("pub"), 1);
    std::stringstream ss;
    kul::File f(pub.join(r));
    if (f && f.dir().real().find(pub.real()) != std::string::npos) {
      bool bin = 0;
      if (f.name().find('.') != std::string::npos &&
          f.name().rfind('.') + 1 < f.name().size()) {
        const std::string& ft(f.name().substr(f.name().rfind('.') + 1));
        if (ft == "css")
          ct = "text/css; charset=utf-8";
        else if (std::find(conf->txt.begin(), conf->txt.end(), ft) ==
                 conf->txt.end()) {
          bin = 1;
          res.header("Content-Disposition",
                     "attachment; filename=\"" + f.name() + "\"");
          if (dlts.count(ft))
            res.header("Content-Type", (*dlts.find(ft)).second);
          else
            res.header("Content-Type", "application/force-download");
        }
      }

      std::shared_ptr<kul::io::AReader> rr;
      if (bin)
        rr = std::make_shared<kul::io::BinaryReader>(f);
      else
        rr = std::make_shared<kul::io::Reader>(f);
      char* s = 0;
      while ((rr->read(s, 1024))) ss << s;
      res.body(ss.str());
    } else {
      if (!ps.count(r) && ps.count("404")) r = "404";
      if (ps.count(r)) {
        auto p((*ps.find(r)).second->clone());
        try {
          p->pre(req);
          res.body(*p->render());
          p->post(req, res);
        } catch (httplus::XXXError& e) {
          e.recover(*p.get());
        } catch (const kul::Exception& e) {
          conf->err << kul::LogMan::INSTANCE().str(__FILE__, __func__, __LINE__,
                                                   kul::log::mode::ERR)
                    << e.stack() << std::flush;
        } catch (const std::exception& e) {
          conf->err << kul::LogMan::INSTANCE().str(__FILE__, __func__, __LINE__,
                                                   kul::log::mode::ERR)
                    << e.what() << std::flush;
        } catch (...) {
          conf->err << kul::LogMan::INSTANCE().str(__FILE__, __func__, __LINE__,
                                                   kul::log::mode::ERR)
                    << "Unknown exception in httplus Reponder" << std::flush;
        }
      } else
        e = 1;
    }
  } else
    KEXCEPT(kul::http::Exception, "DENIED");
  if (e) res.body("ERROR");

#ifdef _HTTPLUS_ACCEPT_GZIP_
  else {
    if (req.headers().count("Accept-Encoding") &&
        (*req.headers().find("Accept-Encoding")).second.find("gzip") !=
            std::string::npos) {
      std::string gz;
      Gzipper::COMPRESS(res.body(), gz);
      res.body(gz);
      res.header("Content-Encoding", "gzip");
    }
  }
#endif /* _HTTPLUS_ACCEPT_GZIP_ */
  if (!res.header("Content-Type")) res.header("Content-Type", ct);
  return res;
}