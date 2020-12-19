#pragma once
#ifndef UITSL_FETCH_FETCH_WEB_HPP_INCLUDE
#define UITSL_FETCH_FETCH_WEB_HPP_INCLUDE

#include "../polyfill/filesystem.hpp"

#include "make_temp_filepath.hpp"
#include "resolve_fetched_filename.hpp"

namespace uitsl {

/*
 * Synchronously fetch data from a url to a temporary file,
 * return temporary file path. Requires `xmlhttprequest` package when running
 * with Node.js.
 */
std::filesystem::path fetch_web( const std::string& url ) {

  const std::filesystem::path bodypath{ uitsl::make_temp_filepath() };
  const std::filesystem::path headerpath{ uitsl::make_temp_filepath() };

  EM_ASM({

    var url = UTF8ToString($0);
    var bodypath = UTF8ToString($1);
    var headerpath = UTF8ToString($2);

    var body;
    var headers;
    if ( typeof XMLHttpRequest == "undefined" ) { // nodejs
      var request = require("sync-request");
      var res = request("GET", url, { gzip: false });
      body = res.body;
      headers = Object.entries(res.headers).map(
        (kv, i) => `${kv[0]}: ${kv[1]}`
      ).join("\n");
    } else { // browser
      var xhr = new XMLHttpRequest();
      xhr.open("GET", url, false);  // synchronous request
      xhr.responseType = "arraybuffer";
      xhr.send();
      body = xhr.response;
      headers = xhr.getAllResponseHeaders();
    }

    var bodystream = FS.open(bodypath, 'w');
    FS.write(bodystream, body, 0, body.length, 0);
    FS.close(bodystream);

    FS.writeFile( headerpath, headers );

  }, url.c_str(), bodypath.c_str(), headerpath.c_str() );

  return uitsl::resolve_fetched_filename( url, bodypath, headerpath );

}

} // namespace uitsl

#endif // #ifndef UITSL_FETCH_FETCH_WEB_HPP_INCLUDE
