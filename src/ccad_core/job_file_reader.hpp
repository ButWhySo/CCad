#ifndef CCAD_CORE_JOB_FILE_READER_HPP
#define CCAD_CORE_JOB_FILE_READER_HPP

#include <string>

namespace ccad {

// Reads Gerber Job Files (gbrjob) which contain project metadata.
class JobFileReader {
public:
    JobFileReader() = default;

    bool parse(const std::string& filepath);
};

} // namespace ccad

#endif // CCAD_CORE_JOB_FILE_READER_HPP
