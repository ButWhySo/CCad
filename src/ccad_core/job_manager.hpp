#ifndef CCAD_CORE_JOB_MANAGER_HPP
#define CCAD_CORE_JOB_MANAGER_HPP

#include <string>

namespace ccad {

// Core background job manager for multi-threaded tasks (DRC, rendering, routing).
class JobManager {
public:
    JobManager() = default;

    void startJob(const std::string& name);
    void waitAll();
};

} // namespace ccad

#endif // CCAD_CORE_JOB_MANAGER_HPP
