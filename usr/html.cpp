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
#include "httplus/yaml.hpp"

class Head : public httplus::Page{
    public:
        Head(){
            auto css(std::make_shared<kul::html4::tag::Named>("link"));
            css->attribute("rel" , "stylesheet");
            css->attribute("type", "text/css");
            css->attribute("href", "res/css.css");
            head(css);
            head(std::make_shared<kul::html4::Text>("<link rel=\"icon\" type=\"image/png\" href=\"data:;base64,iVBORw0KGgo=\" >"));
        }
};

class Index : public Head{
    public:
        Index(){
            // A BASIC SETUP FOR ALL INSTANCES OF THIS TYPE
            head(std::make_shared<kul::html4::tag::Named>("title", "Index"));

            std::shared_ptr<kul::html4::Tag> div(std::make_shared<kul::html4::tag::Named>("div"));
            div->attribute("class", "body");
            std::shared_ptr<kul::html4::Tag> txt(std::make_shared<kul::html4::tag::Label>("SOME TEXT HERE"));
            std::string e("& < > \" ' / / ' \" > < &");
            kul::HTML::ESC(e);
            div->tag(txt).br().tag(std::make_shared<kul::html4::tag::Label>(e));
            body(div);
            std::shared_ptr<kul::html4::Tag> div1(std::make_shared<kul::html4::tag::Named>("div"));
            div1->attribute("class", "body").tag(std::make_shared<kul::html4::esc::Text>("& < > \" ' / / ' \" > < &"));

            std::shared_ptr<kul::html4::Tag> p(std::make_shared<kul::html4::tag::Named>("p", "Enter the competition by "));
            std::shared_ptr<kul::html4::Tag> r(std::make_shared<kul::html4::tag::Named>("mark", "January 30, 2011"));
            r->attribute("class", "red");
            std::shared_ptr<kul::html4::Tag> b(std::make_shared<kul::html4::tag::Named>("mark", "summer"));
            b->attribute("class", "blue");
            
            p->tag(r).text(" and you could win up to $$$$ — including amazing ")
                .tag(b).text(" trips!")
                .esc("& < > \" ' / / ' \" > < &");
            body(p).body(div);
        }
        virtual void pre (const kul::http::ARequest& req){
            // called before render
            // "this" is a now a copy of the original
        }
        virtual void post(const kul::http::ARequest& req, kul::http::AResponse& res){
            // called after render
            // "this" is disposed of shortly after this method
        }
};

class CSS : public httplus::Page{
    private:
        std::string s;
    public:
        CSS(){
s = R"(/* CSS */

body {
    background: #111111;
}

p .body{
    font-size:14px;
    color:#538b01;
    font-weight:bold;
    font-style:italic;
}

.body { 
  height: auto; 
  width: 440px; 
  padding: 15px 15px 15px 15px; 
  margin: 0px auto 0px auto; 
  display: block; 
  background: #FFFFFF; 
  color: #000000; 
}

mark.red {
    color:#ff0000;
    background: none;
}

mark.blue {
    color:#0000A0;
    background: none;
})";
        }
        virtual const std::string* render(){
            return &s;
        }
        virtual void post(const kul::http::ARequest& req, kul::http::AResponse& res){
            res.header("Content-Type", "text/css; charset=utf-8");
        }
};

// This method is to setup generated pages
void httplus::yaml::Conf::LOAD(Sites& sites){
    auto css = std::make_shared<CSS>();
    auto index = std::make_shared<Index>();
    
    auto global = std::make_shared<Pages>();
    global->insert("index", index);
    global->insert("res/css.css", css);

    auto local = std::make_shared<Pages>();
    local->insert("index", index);
    local->insert("res/css.css", css);
    
    sites.insert(std::to_string(std::hash<std::string>()("/var/www/global")), global);
    sites.insert(std::to_string(std::hash<std::string>()("/var/www/local")) , local);
}
