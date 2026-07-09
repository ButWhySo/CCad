#ifndef CCAD_CORE_DATABASE_BASE_HPP
#define CCAD_CORE_DATABASE_BASE_HPP

#include <string>

namespace ccad {

// Base interface for library and project database indexing.
class DatabaseBase {
public:
    DatabaseBase() = default;
    virtual ~DatabaseBase() = default;

    virtual bool connect(const std::string& uri) = 0;
    virtual void disconnect() = 0;
};

} // namespace ccad

#endif // CCAD_CORE_DATABASE_BASE_HPP
