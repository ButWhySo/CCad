#ifndef CCAD_CORE_API_BRIDGE_HPP
#define CCAD_CORE_API_BRIDGE_HPP

#include <string>

namespace ccad {

// Core interface for IPC/gRPC API endpoints bridging UI and CLI to the kernel.
class ApiBridge {
public:
    ApiBridge() = default;
    virtual ~ApiBridge() = default;

    virtual std::string dispatchCommand(const std::string& json_rpc_payload) = 0;
};

// Concrete implementation of ApiBridge for local, in-process routing.
class LocalApiBridge : public ApiBridge {
public:
    LocalApiBridge() = default;
    ~LocalApiBridge() override = default;

    std::string dispatchCommand(const std::string& json_rpc_payload) override;
};

} // namespace ccad

#endif // CCAD_CORE_API_BRIDGE_HPP
