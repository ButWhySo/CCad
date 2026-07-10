#include "undo_redo.hpp"

namespace ccad {

void UndoRedoEngine::pushCommand(std::unique_ptr<UndoCommand> command) {
    command->redo();
    undoStack_.push_back(std::move(command));
    redoStack_.clear();
}

void UndoRedoEngine::undo() {
    if (!undoStack_.empty()) {
        auto cmd = std::move(undoStack_.back());
        undoStack_.pop_back();
        cmd->undo();
        redoStack_.push_back(std::move(cmd));
    }
}

void UndoRedoEngine::redo() {
    if (!redoStack_.empty()) {
        auto cmd = std::move(redoStack_.back());
        redoStack_.pop_back();
        cmd->redo();
        undoStack_.push_back(std::move(cmd));
    }
}

bool UndoRedoEngine::canUndo() const {
    return !undoStack_.empty();
}

bool UndoRedoEngine::canRedo() const {
    return !redoStack_.empty();
}

void UndoRedoEngine::clear() {
    undoStack_.clear();
    redoStack_.clear();
}

} // namespace ccad
