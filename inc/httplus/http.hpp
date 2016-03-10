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
#ifndef _HTTPLUS_HTTP_HPP_
#define _HTTPLUS_HTTP_HPP_

#include "kul/io.hpp"
#include "kul/https.hpp"

#include "httplus/html.hpp"

namespace httplus{ 
class App;
namespace https{
class Server;
}
namespace http{

class Exception : public kul::Exception{
    public:
        Exception(const char*f, const uint16_t& l, const std::string& s) : kul::Exception(f, l, s){}
};

class Responder;
class Conf{
    protected:
        const std::string home, root;
        kul::File a ,e;
        kul::io::Writer acc ,err;
        std::vector<std::string> txt;
    public:
        Conf(const std::string& r, const std::string& h = "", const std::string& t = "") : 
            home(h), root(r),
            a(kul::File("access.log", kul::Dir(r).join("log"))),
            e(kul::File("error.log", kul::Dir(r).join("log"))),
            acc(a, 1), err(e, 1){
                kul::String::SPLIT(t, ' ', txt);
            }
		friend class Responder;
};
typedef kul::hash::map::S2T<std::shared_ptr<Conf>> Confs;

class Responder{
    private:
        kul::hash::map::S2S dlts;
        Responder(){
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
        static Responder& INSTANCE(){
            static Responder i; return i;
        }
        const kul::http::AResponse& response(
                kul::http::AResponse& res, 
                const std::string& resource, 
                const kul::http::ARequest& req,
                const Pages& ps,
                const Confs& confs, 
                Conf* def){
            http::Conf* conf = def;
            if(req.headers().count("Host")){
                std::vector<std::string> bits;
                std::string host((*req.headers().find("Host")).second);
                kul::String::SPLIT(host, ':', bits);
                if(bits.size() > 1) host = bits[0];
                if(confs.count(host)) conf = (*confs.find(host)).second.get();
            }
            std::string ct = "text/html; charset=utf-8";
            bool e = 0;
            if(conf){
                std::string r(resource.substr(1));
                if(r.empty() && conf->home.size()) r = conf->home;
                const kul::Dir root(conf->root);
                const kul::Dir log(root.join("log"), 1);
                const kul::Dir pub(root.join("pub"), 1);
                std::stringstream ss;
                kul::File f(pub.join(r));
                if(f && f.dir().real().find(pub.real()) != std::string::npos){
                    bool bin = 0;
                    if(f.name().find('.') != std::string::npos && f.name().rfind('.') + 1 < f.name().size()){
                        const std::string& ft(f.name().substr(f.name().rfind('.') + 1));
                        if(ft == "css") ct = "text/css; charset=utf-8";
                        else if(std::find(conf->txt.begin(), conf->txt.end(), ft) == conf->txt.end()){
                            bin = 1;
                            res.header("Content-Disposition", "attachment; filename\""+f.name()+"\"");
                            if(dlts.count(ft))
                                res.header("Content-Type", (*dlts.find(ft)).second);
                            else
                                res.header("Content-Type", "application/force-download");
                        } 
                    }
                        
                    std::shared_ptr<kul::io::AReader> rr;
                    if(bin) rr = std::make_shared<kul::io::BinaryReader>(f);
                    else    rr = std::make_shared<kul::io::Reader>(f);
                    const std::string*s = 0;
                    while((s = rr->read(1024))) ss << *s;
                    res.body(ss.str());
                }else{
                    if(!ps.count(r) && ps.count("404")) r = "404";
                    if(ps.count(r)) {
                        auto p((*ps.find(r)).second->clone());
                        try{
                            p->pre(req);
                            res.body(*p->render());
                            p->post(req, res);
                        }catch(httplus::XXXError& e){
                            e.recover(*p.get());
                        }catch(const kul::Exception& e){
                            conf->err << kul::LogMan::INSTANCE().str(__FILE__, __LINE__, kul::log::mode::ERR) 
                                << e.stack() << std::flush;
                        }catch(const std::exception& e){
                            conf->err << kul::LogMan::INSTANCE().str(__FILE__, __LINE__, kul::log::mode::ERR) 
                                << e.what() << std::flush;
                        }catch(...){
                            conf->err << kul::LogMan::INSTANCE().str(__FILE__, __LINE__, kul::log::mode::ERR) 
                                << "Unknown exception in httplus Reponder" << std::flush;
                        }
                    } else e = 1;
                }

                //if(!e) conf->acc << "REALLY BIG SHOE!" << kul::os::EOL() << std::flush;
            }else KEXCEPT(kul::http::Exception, "DENIED");
            if(e) res.body("ERROR");
            if(!res.header("Content-Type"))res.header("Content-Type", ct);
            return res;
        }
    friend class Server;
    friend class https::Server;
};

class Server : public kul::http::Server{
    private:
        const Pages& ps;
        Confs confs;
        std::shared_ptr<Conf> def;
        void operator()(){
            kul::http::Server::start();
        }
        const kul::http::AResponse response(const std::string& res, const kul::http::ARequest& req){
            kul::http::_1_1Response r;
            Responder::INSTANCE().response(r, res, req, ps, confs, def.get());
            return kul::http::Server::response(r);
        }
    public:
        Server(const uint16_t& p, const Pages& ps) 
        	: kul::http::Server(p), ps(ps){}
        void stop(){
            kul::http::Server::stop();
        }
        friend class kul::Thread;
        friend class httplus::App;
};
}

namespace https{
class Server : public kul::https::Server{
    private:
        const Pages& ps;
        http::Confs confs; 
        void operator()(){
            kul::https::Server::start();
        }
        const kul::http::AResponse response(const std::string& res, const kul::http::ARequest& req){
            kul::http::_1_1Response r;
            http::Responder::INSTANCE().response(r, res, req, ps, confs, 0);
            return kul::http::Server::response(r);
        }
   public:
        Server(const uint16_t& p, const Pages& ps, const kul::File& crt, const kul::File& key) 
        	: kul::https::Server(p, crt, key), ps(ps){}
        void stop(){
            kul::https::Server::stop();
        }
        friend class kul::Thread;
        friend class httplus::App;
};

}}
#endif /* _HTTPLUS_HTTP_HPP_ */
