#pragma once

#include "../Log.hpp"
#include "CCStructs.hpp"

namespace CrowdControl {
	void StartServer();
	void StartServerLoop();
	Response RunCommand(const Request& request);

	namespace Structs {
		void to_json_request(json& j, const Request& p);
		void from_json_request(const json& j, Request& p);
		void to_json_response(json& j, const Response& p);
		void from_json_response(const json& j, Response& p);
	}
}