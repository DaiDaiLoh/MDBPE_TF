#pragma once

// TODO: because everything is https, we need a more heavyweight solution

// request api inspired from https://github.com/fnc12/embeddedRest/

namespace babel::net
{

struct http_response
{
};

/// A http1.1 request
///
/// allows to specify:
/// - http method
/// - domain
/// - port
/// - get paramters
/// - http headers
/// 
/// TODO: post
/// 
/// NOTE: this is lightweight and synchronous
/// 
/// Usage example:
/// 
///     auto req = http_request()
///			.host("https://github.com")
///			.uri("/project-arcana/babel-serializer")
///			.
/// 
///     // super short version:
///     auto res = babel::net::execute_http_request("https://github.com/project-arcana/babel-serializer");
struct http_request
{
};
}
