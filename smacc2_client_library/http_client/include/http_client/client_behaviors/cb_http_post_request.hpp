// Copyright 2023 RobosoftAI Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*****************************************************************************************************************
 *
 * 	 Authors: Jaycee Lock
 *
 ******************************************************************************************************************/

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
