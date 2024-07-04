#pragma once

#include <http_client/cl_http_client.hpp>
#include <smacc2/smacc.hpp>

namespace cl_http
{
class CbHttpPostRequest : publc CbHttpRequestBase
{
public:
  CbHttpPostRequest() : CbHttpGetRequest(ClHttp::kHttpRequestMethod::POST) {}
};
}  // namespace cl_http
