#pragma once

#include "UndoCommand.h"
#include <memory>
#include <vector>
#include <mutex>

// Command pattern undo/redo with delta storage and memory limits
class UndoStack {
public:
    UndoStack(size_t maxCommands = 50, size_t maxMemoryMB = 100);

    void push(std::unique_ptr<UndoCommand> command);
    bool undo();
    bool redo();

    bool canUndo() const;
    bool canRedo() const;
    void clear();

    const char* getUndoDescription() const;
    const char* getRedoDescription() const;

    size_t getMemoryUsage() const;
    size_t getUndoCount() const;
    size_t getRedoCount() const;

private:
    void enforceMemoryLimit();
    void enforceCommandLimit();
    size_t getMemoryUsageUnlocked() const;

    std::vector<std::unique_ptr<UndoCommand>> undoStack_;
    std::vector<std::unique_ptr<UndoCommand>> redoStack_;

    size_t maxCommands_;
    size_t maxMemoryBytes_;

    mutable std::mutex mutex_;
};
