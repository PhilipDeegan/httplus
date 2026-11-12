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
#include "httplus/http.hpp"

#include "mkn/kul/time.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <iterator>
#include <sstream>
#include <utility>

#ifdef HTTPLUS_ACCEPT_GZIP
#include <zlib.h>
#endif /* HTTPLUS_ACCEPT_GZIP */

namespace fs = std::filesystem;

namespace {

constexpr std::array<std::pair<std::string_view, std::string_view>, 24> MIME_TYPES{{
    {"html", "text/html; charset=utf-8"},
    {"htm", "text/html; charset=utf-8"},
    {"css", "text/css; charset=utf-8"},
    {"js", "text/javascript; charset=utf-8"},
    {"mjs", "text/javascript; charset=utf-8"},
    {"json", "application/json"},
    {"txt", "text/plain; charset=utf-8"},
    {"xml", "application/xml"},
    {"svg", "image/svg+xml"},
    {"png", "image/png"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"gif", "image/gif"},
    {"ico", "image/x-icon"},
    {"webp", "image/webp"},
    {"woff", "font/woff"},
    {"woff2", "font/woff2"},
    {"wasm", "application/wasm"},
    {"pdf", "application/pdf"},
    {"zip", "application/zip"},
    {"doc", "application/msword"},
    {"xls", "application/vnd.ms-excel"},
    {"ppt", "application/vnd.ms-powerpoint"},
    {"exe", "application/octet-stream"},
}};

std::optional<std::string_view> mime_type(std::string_view const ext) {
  auto const it = std::find_if(MIME_TYPES.begin(), MIME_TYPES.end(),
                               [&](auto const& kv) { return kv.first == ext; });
  if (it == MIME_TYPES.end()) return std::nullopt;
  return it->second;
}

bool iequals(std::string_view const a, std::string_view const b) {
  return std::ranges::equal(
      a, b, [](unsigned char x, unsigned char y) { return std::tolower(x) == std::tolower(y); });
}

std::string_view header(httplus::Request const& req, std::string_view const name) {
  for (auto const& [k, v] : req.headers())
    if (iequals(k, name)) return v;
  return {};
}

// "example.com:8080" -> "example.com", "[::1]:8080" -> "[::1]"
std::string_view host_name(std::string_view host) {
  auto const colon = host.rfind(':');
  if (colon != std::string_view::npos && host.find(']', colon) == std::string_view::npos)
    host = host.substr(0, colon);
  return host;
}

std::optional<std::string> percent_decode(std::string_view const s) {
  std::string out;
  out.reserve(s.size());
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '%' && i + 2 < s.size()) {
      unsigned char c = 0;
      auto const [p, ec] = std::from_chars(s.data() + i + 1, s.data() + i + 3, c, 16);
      if (ec == std::errc{} && p == s.data() + i + 3) {
        if (c == '\0') return std::nullopt;
        out += static_cast<char>(c);
        i += 2;
        continue;
      }
    }
    out += s[i];
  }
  return out;
}

std::string read_file(fs::path const& path) {
  std::ifstream in(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

#ifdef HTTPLUS_ACCEPT_GZIP
std::string gzip(std::string const& in) {
  z_stream z{};
  // 15 + 16: max window, gzip wrapper rather than zlib
  if (deflateInit2(&z, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
    KEXCEPT(httplus::http::Exception, "zlib deflateInit2 failed");
  std::string out(deflateBound(&z, in.size()), '\0');
  z.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(in.data()));
  z.avail_in = static_cast<uInt>(in.size());
  z.next_out = reinterpret_cast<Bytef*>(out.data());
  z.avail_out = static_cast<uInt>(out.size());
  auto const e = deflate(&z, Z_FINISH);
  out.resize(z.total_out);
  deflateEnd(&z);
  if (e != Z_STREAM_END)
    KEXCEPT(httplus::http::Exception, "zlib compression failed: " + std::to_string(e));
  return out;
}
#endif /* HTTPLUS_ACCEPT_GZIP */

void serve_file(httplus::http::Conf const& conf, fs::path const& file, httplus::Response& res) {
  auto const ext = file.has_extension() ? file.extension().string().substr(1) : std::string{};
  if (ext.empty())
    res.header("Content-Type", "text/html; charset=utf-8");
  else if (std::ranges::find(conf.text, ext) != conf.text.end())
    res.header("Content-Type", "text/plain; charset=utf-8");
  else if (auto const mime = mime_type(ext))
    res.header("Content-Type", std::string(*mime));
  else {
    res.header("Content-Type", "application/octet-stream");
    res.header("Content-Disposition", "attachment; filename=\"" + file.filename().string() + "\"");
  }
  res.body(read_file(file));
}

void status(httplus::Response& res, std::uint16_t const code, std::string const& reason) {
  res.status(code);
  res.reason(reason);
}

void render_page(httplus::http::Conf& conf, httplus::Page const& prototype,
                 httplus::Request const& req, httplus::Response& res) {
  auto const fail = [&](std::string const& what) {
    res = httplus::Response{};
    status(res, 500, "Internal Server Error");
    res.body("Internal Server Error");
    conf.error(what);
  };

  auto const page = prototype.clone();
  try {
    try {
      page->pre(req);
      res.body(*page->render());
      page->post(req, res);
    } catch (httplus::XXXError& e) {
      e.recover(*page);
      res.body(*page->render());
    }
  } catch (mkn::kul::Exception const& e) {
    std::ostringstream ss;
    ss << e.stack();
    fail(ss.str());
  } catch (std::exception const& e) {
    fail(e.what());
  } catch (...) {
    fail("Unknown exception rendering: " + req.path());
  }
  if (!res.header("Content-Type")) res.header("Content-Type", "text/html; charset=utf-8");
}

}  // namespace

httplus::http::Conf::Conf(fs::path const& root_, Pages pages_, std::string home_,
                          std::vector<std::string> text_,
                          std::unordered_map<std::string, std::string> headers_)
    : root(fs::canonical(root_)),
      pub(root / "pub"),
      home(std::move(home_)),
      text(std::move(text_)),
      headers(std::move(headers_)),
      pages(std::move(pages_)) {
  fs::create_directories(pub);
  fs::create_directories(root / "log");
  _acc.open(root / "log" / "access.log", std::ios::app);
  _err.open(root / "log" / "error.log", std::ios::app);
  if (!_acc || !_err) KEXCEPT(Exception, "Cannot open logs in: " + (root / "log").string());
}

std::optional<fs::path> httplus::http::Conf::file(std::string_view const resource) const {
  if (resource.empty()) return std::nullopt;
  std::error_code ec;
  auto const path = fs::weakly_canonical(pub / fs::path(resource).relative_path(), ec);
  if (ec || !fs::is_regular_file(path, ec)) return std::nullopt;
  // reject anything resolving outside pub/, e.g. "../" or symlinks
  auto const [p, _] = std::mismatch(pub.begin(), pub.end(), path.begin(), path.end());
  if (p != pub.end()) return std::nullopt;
  return path;
}

void httplus::http::Conf::access(std::string_view const line) {
  std::lock_guard<std::mutex> lock(_log_mutex);
  _acc << line << std::endl;
}

void httplus::http::Conf::error(std::string_view const line) {
  std::lock_guard<std::mutex> lock(_log_mutex);
  _err << mkn::kul::DateTime::NOW() << " " << line << std::endl;
}

void httplus::http::AServer::add(std::string const& host, std::shared_ptr<Conf> const& conf) {
  if (_confs.count(host))
    KEXCEPT(Exception, host.empty() ? "Only one entry per port may omit 'host'"
                                    : "Duplicate host on port: " + host);
  _confs.emplace(host, conf);
}

httplus::Response httplus::http::respond(Request const& req, Confs const& confs) {
  MKN_KUL_DBG_FUNC_ENTER;
  Response res;

  auto it = confs.find(std::string(host_name(header(req, "Host"))));
  if (it == confs.end()) it = confs.find("");
  if (it == confs.end()) {
    status(res, 404, "Not Found");
    res.body("Unknown host");
    return res.withDefaultHeaders();
  }
  auto& conf = *it->second;

  auto resource = percent_decode(req.path());
  if (!resource) {
    status(res, 400, "Bad Request");
    res.body("Bad Request");
    return res.withDefaultHeaders();
  }
  if (resource->starts_with('/')) resource->erase(0, 1);
  if (resource->empty()) *resource = conf.home;

  if (auto const file = conf.file(*resource))
    serve_file(conf, *file, res);
  else if (auto const page = conf.pages.find(*resource); page != conf.pages.end())
    render_page(conf, *page->second, req, res);
  else {
    if (auto const nf = conf.pages.find("404"); nf != conf.pages.end())
      render_page(conf, *nf->second, req, res);
    else
      res.body("Not Found");
    if (res.status() == 200) status(res, 404, "Not Found");
  }

  for (auto const& [k, v] : conf.headers)
    if (!res.header(k)) res.header(k, v);

#ifdef HTTPLUS_ACCEPT_GZIP
  if (!res.body().empty() && header(req, "Accept-Encoding").find("gzip") != std::string::npos) {
    res.body(gzip(res.body()));
    res.header("Content-Encoding", "gzip");
    res.header("Vary", "Accept-Encoding");
  }
#endif /* HTTPLUS_ACCEPT_GZIP */

  std::ostringstream line;
  line << mkn::kul::DateTime::NOW() << " " << req.ip() << " " << req.method() << " \"" << req.path()
       << "\" " << res.status() << " " << res.body().size();
  conf.access(line.str());

  return res.withDefaultHeaders();
}
