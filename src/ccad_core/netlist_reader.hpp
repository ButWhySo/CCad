#ifndef CCAD_CORE_NETLIST_READER_HPP
#define CCAD_CORE_NETLIST_READER_HPP

#include <string>

namespace ccad {

// Base class for importing different netlist formats (IPC-D-356, etc).
class NetlistReader {
public:
    NetlistReader() = default;
    virtual ~NetlistReader() = default;

    virtual bool read(const std::string& filepath) = 0;
};

} // namespace ccad

#endif // CCAD_CORE_NETLIST_READER_HPP
