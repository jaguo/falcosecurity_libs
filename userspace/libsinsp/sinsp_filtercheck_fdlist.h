// SPDX-License-Identifier: Apache-2.0
/*
Copyright (C) 2023 The Falco Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#pragma once

#include "sinsp_filtercheck.h"

class sinsp_filter_check_fdlist : public sinsp_filter_check
{
public:
	enum check_type
	{
		TYPE_FDNUMS = 0,
		TYPE_FDNAMES = 1,
		TYPE_CLIENTIPS = 2,
		TYPE_SERVERIPS = 3,
		TYPE_CLIENTPORTS = 4,
		TYPE_SERVERPORTS = 5,
	};

	sinsp_filter_check_fdlist();
	sinsp_filter_check* allocate_new();
	uint8_t* extract(sinsp_evt *evt, OUT uint32_t* len, bool sanitize_strings = true);

private:
	std::string m_strval;
	char m_addrbuff[100];
};
