/**
Copyright (c) 2013, Philip Deegan.
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

std::vector<std::string> httplus::http::Reponder::TXT = {"xml", "txt", "html", "css"};

const kul::http::AResponse& httplus::http::Reponder::response(
        kul::http::AResponse& res, 
        const std::string& resource, 
        const kul::http::ARequest& req,
        const Pages& ps,
        const http::Confs& confs, 
        http::Conf* def){
    http::Conf* conf = def;
    if(req.headers().count("Host")){
        std::vector<std::string> bits;
        std::string host((*req.headers().find("Host")).second);
        kul::String::split(host, ':', bits);
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
        kul::File f(r, pub);
        if(f && f.dir().real().find(pub.real()) != std::string::npos){
            bool bin = 0;
            if(f.name().find('.') != std::string::npos && f.name().rfind('.') + 1 < f.name().size()){
                const std::string& ft(f.name().substr(f.name().rfind('.') + 1));
                if(std::find(TXT.begin(), TXT.end(), ft) == TXT.end()) bin = 1;
                if(ft == "css") ct = "text/css; charset=utf-8";
            }
            std::shared_ptr<kul::io::AReader> rr;
            if(bin) rr = std::make_shared<kul::io::BinaryReader>(f);
            else    rr = std::make_shared<kul::io::Reader>(f);
            const std::string*s = 0;
            while((s = rr->read(1024))) ss << *s;
            res.body(ss.str());
        }else
        if(ps.count(r)) {
            auto p = (*ps.find(r)).second;
            p->pre(req);
            res.body(*p->render());
            p->post(req, res);
        } else e = 1;
        if(!e) conf->acc << "REALLY BIG SHOE!" << kul::os::EOL() << std::flush;
    }else KEXCEPT(kul::http::Exception, "HTTP RESPONSE DENIED");
    if(e) res.body("ERROR");
    if(!res.header("Content-Type"))res.header("Content-Type", ct);
    return res;
}
