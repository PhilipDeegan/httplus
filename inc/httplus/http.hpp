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

class Reponder;
class Conf{
    protected:
        const std::string home, root;
        kul::File a ,e;
        kul::io::Writer acc ,err;
    public:
        Conf(const std::string& r, const std::string& h = "") : 
            home(h), root(r),
            a(kul::File("access.log", kul::Dir(r).join("log"))),
            e(kul::File("error.log", kul::Dir(r).join("log"))),
            acc(a, 1), err(e, 1){}
		friend class Reponder;
};
typedef kul::hash::map::S2T<std::shared_ptr<Conf>> Confs;

class Reponder{
    private:
        static std::vector<std::string> TXT;
        static const kul::http::AResponse& response(
                kul::http::AResponse& r, 
                const std::string& res, 
                const kul::http::ARequest& req,
                const Pages& ps,
                const Confs& confs, 
                Conf* def);
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
            Reponder::response(r, res, req, ps, confs, def.get());
            return kul::http::Server::response(r);
        }
    public:
        Server(const uint16_t& p, const Pages& ps) 
        	: kul::http::Server(p), ps(ps){}
        void stop(){
            KLOG(DBG) << "STOPPING SERVER!";
            kul::http::Server::stop();
        }
        friend class kul::ThreadRef<Server>;
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
            http::Reponder::response(r, res, req, ps, confs, 0);
            return kul::http::Server::response(r);
        }
   public:
        Server(const uint16_t& p, const Pages& ps, const kul::File& crt, const kul::File& key) 
        	: kul::https::Server(p, crt, key), ps(ps){}
        void stop(){
            KLOG(DBG) << "STOPPING SERVER!";
            kul::https::Server::stop();
        }
        friend class kul::ThreadRef<Server>;
        friend class httplus::App;
};

}}
#endif /* _HTTPLUS_HTTP_HPP_ */
