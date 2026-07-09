#ifndef CCAD_CORE_CONNECTION_GRAPH_HPP
#define CCAD_CORE_CONNECTION_GRAPH_HPP

namespace ccad {

// Core connectivity graph algorithm for schematic nets.
class ConnectionGraph {
public:
    ConnectionGraph() = default;
    virtual ~ConnectionGraph() = default;

    virtual void buildGraph() = 0;
};

} // namespace ccad

#endif // CCAD_CORE_CONNECTION_GRAPH_HPP
