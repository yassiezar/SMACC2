#pragma once

#include <http_client/cl_http_client.hpp>
#include <http_client/client_behaviors/cb_http_request.hpp>
#include <smacc2/smacc.hpp>

namespace cl_http
{
class CbHttpGetRequest : public CbHttpRequestBase
{
public:
  CbHttpGetRequest() : CbHttpRequestBase(ClHttp::kHttpRequestMethod::GET) {}
};
}  // namespace cl_http
