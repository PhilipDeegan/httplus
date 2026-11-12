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
#include "httplus/html.hpp"

#include <memory>
#include <string>

using namespace mkn::ram;

class Head : public httplus::Page {
 public:
  Head() {
    auto css = std::make_shared<html4::tag::Named>("link");
    css->attribute("rel", "stylesheet");
    css->attribute("type", "text/css");
    css->attribute("href", "res/css.css");
    head(css);
    head(std::make_shared<html4::Text>(
        R"(<link rel="icon" type="image/png" href="data:;base64,iVBORw0KGgo=" >)"));
  }
};

class Index : public httplus::PageT<Index, Head> {
 public:
  Index() {
    // A BASIC SETUP FOR ALL INSTANCES OF THIS TYPE
    head(std::make_shared<html4::tag::Named>("title", "Index"));

    std::shared_ptr<html4::Tag> div = std::make_shared<html4::tag::Named>("div");
    div->attribute("class", "body");
    std::string e = R"(& < > " ' / / ' " > < &)";
    HTML::ESC(e);
    div->add(std::make_shared<html4::tag::Label>("SOME TEXT HERE"))
        .br()
        .add(std::make_shared<html4::tag::Label>(e));
    body(div);

    std::shared_ptr<html4::Tag> div1 = std::make_shared<html4::tag::Named>("div");
    div1->attribute("class", "body")
        .add(std::make_shared<html4::esc::Text>(R"(& < > " ' / / ' " > < &)"));

    std::shared_ptr<html4::Tag> p =
        std::make_shared<html4::tag::Named>("p", "Enter the competition by ");
    std::shared_ptr<html4::Tag> r = std::make_shared<html4::tag::Named>("mark", "January 30, 2011");
    r->attribute("class", "red");
    std::shared_ptr<html4::Tag> b = std::make_shared<html4::tag::Named>("mark", "summer");
    b->attribute("class", "blue");

    p->add(r)
        .text(" and you could win up to $$$$ — including amazing ")
        .add(b)
        .text(" trips!")
        .esc(R"(& < > " ' / / ' " > < &)");
    body(p).body(div);
  }
  void pre(httplus::Request const& /*req*/) override {
    // called before render on a fresh instance
  }
  void post(httplus::Request const& /*req*/, httplus::Response& /*res*/) override {
    // called after render, the instance is discarded after this
  }
};

class NotFound : public httplus::PageT<NotFound, Head> {
 public:
  NotFound() {
    std::shared_ptr<html4::Tag> div = std::make_shared<html4::tag::Named>("div");
    div->attribute("class", "body");
    div->text("NOT FOUND - 404");
    body(div);
  }
};

class CSS : public httplus::PageT<CSS> {
 public:
  std::string const* render() override { return &s; }
  void post(httplus::Request const& /*req*/, httplus::Response& res) override {
    res.header("Content-Type", "text/css; charset=utf-8");
  }

 private:
  std::string const s = R"(/* CSS */

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
};
