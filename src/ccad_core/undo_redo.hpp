#ifndef CCAD_CORE_UNDO_REDO_HPP
#define CCAD_CORE_UNDO_REDO_HPP

#include <vector>
#include <memory>
#include <string>

namespace ccad {

// Represents a single transaction or change that can be reverted
class UndoCommand {
public:
    virtual ~UndoCommand() = default;
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual std::string getDescription() const = 0;
};

// Manages the stack of undoable/redoable transactions for the board state
class UndoRedoEngine {
public:
    UndoRedoEngine() = default;
    ~UndoRedoEngine() = default;

    void pushCommand(std::unique_ptr<UndoCommand> command);

    void undo();
    void redo();

    bool canUndo() const;
    bool canRedo() const;

    void clear();

private:
    std::vector<std::unique_ptr<UndoCommand>> undoStack_;
    std::vector<std::unique_ptr<UndoCommand>> redoStack_;
};

} // namespace ccad

#endif // CCAD_CORE_UNDO_REDO_HPP
