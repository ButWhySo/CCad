#include "api_bridge.hpp"
#include "job_manager.hpp"
#include <iostream>

namespace ccad {

static JobManager g_job_manager;

std::string LocalApiBridge::dispatchCommand(const std::string& json_rpc_payload) {
    // Basic kernel reflection and asynchronous job integration
    if (json_rpc_payload.find("\"method\": \"ping\"") != std::string::npos) {
        return "{\"jsonrpc\": \"2.0\", \"result\": \"pong\"}";
    } else if (json_rpc_payload.find("\"method\": \"start_job\"") != std::string::npos) {
        g_job_manager.startJob("api_requested_job");
        return "{\"jsonrpc\": \"2.0\", \"result\": \"job_started\"}";
    }
    
    return "{\"jsonrpc\": \"2.0\", \"error\": {\"code\": -32601, \"message\": \"Method not found\"}}";
}

} // namespace ccad
