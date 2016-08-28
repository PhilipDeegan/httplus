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

void httplus::App::load(kul::hash::map::S2T<std::shared_ptr<http::Server>>& http,
            kul::hash::map::S2T<std::shared_ptr<https::Server>>& https, Sites& sites) throw(httplus::Exception){

    std::shared_ptr<http::Conf> defHttp;
    if(config.root()["http"])
        for(const YAML::Node& c : config.root()["http"]){
            if(defHttp && !c["host"]) KEXCEPTION("Only one http allowed without 'host' parameter");
            const std::string& port(c["port"] ? c["port"].Scalar() : "80");
            kul::Dir d(c["root"].Scalar());
            if(!d) KEXCEPTION("Directory does not exist: " + c["root"].Scalar());
            kul::Dir p("pub", d);
            if(!p && !p.mk()) KEXCEPTION("Invalid access on directory: " + d.real());
            http::Server* ser = {0};
            std::string home(c["home"] ? c["home"].Scalar() : "");
            const std::string txt(c["text"] ? c["text"].Scalar() : "");
            if(!c["host"]){
                defHttp = std::make_shared<http::Conf>(c["root"].Scalar(), home, txt);
            }else if(sites.count(std::to_string(std::hash<std::string>()(d.real())))){
                const Pages& pages((*sites.find(std::to_string(std::hash<std::string>()(d.real())))).second);
                http.insert(port, std::make_shared<http::Server>(kul::String::UINT16(port), pages));
                ser = http[port].get();
                ser->confs.insert(c["host"].Scalar(), std::make_shared<http::Conf>(c["root"].Scalar(), home, txt));
            }else
                KERR << "WARN: NO GENERATORS FOR HTTP ROOT: " << d;

        }
    for(const auto& p : http) p.second->def = defHttp;
    if(config.root()["https"])
        for(const YAML::Node& c : config.root()["https"]){
            kul::Dir d(c["root"].Scalar());
            if(!d) KEXCEPTION("Directory does not exist: " + c["root"].Scalar());
            kul::Dir p("pub", d);
            if(!p && !p.mk()) KEXCEPTION("Invalid access on directory: " + d.real());
            kul::File crt(c["crt"].Scalar());
            kul::File key(c["key"].Scalar());
            if(!crt) KEXCEPTION("File does not exist: " + crt.full());
            if(!key) KEXCEPTION("File does not exist: " + key.full());
            https::Server* ser = {0};
            const std::string& port(c["port"] ? c["port"].Scalar() : "443");
            const std::string hsh(std::to_string(std::hash<std::string>()(d.real())));
            if(!sites.count(hsh)) {
                KERR << "WARN: NO GENERATORS FOR HTTPS ROOT: " << d;
                continue; 
            }
            const Pages& pages((*sites.find(hsh)).second);
            std::string ssls(c["ssls"] ? c["ssls"].Scalar() 
                : config.root()["ssls"] ? config.root()["ssls"].Scalar() : "");
            https.insert(port, std::make_shared<https::Server>(kul::String::UINT16(port), pages, crt, key, ssls));
            ser = https[port].get();
            ser->init();
            if(c["chain"]) ser->setChain(c["chain"].Scalar());
            std::string home(c["home"] ? c["home"].Scalar() : "");
            const std::string txt(c["text"] ? c["text"].Scalar() : "");
            ser->confs.insert(c["host"].Scalar(), std::make_shared<http::Conf>(c["root"].Scalar(), home, txt));
        }
}