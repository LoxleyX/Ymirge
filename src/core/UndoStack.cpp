#include "UndoStack.h"
#include <iostream>

UndoStack::UndoStack(size_t maxCommands, size_t maxMemoryMB)
    : maxCommands_(maxCommands)
    , maxMemoryBytes_(maxMemoryMB * 1024 * 1024) {
}

void UndoStack::push(std::unique_ptr<UndoCommand> command) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Execute the command
    command->execute();

    // Clear redo stack (branching timeline)
    redoStack_.clear();

    // Add to undo stack
    undoStack_.push_back(std::move(command));

    // Enforce limits
    enforceMemoryLimit();
    enforceCommandLimit();

    std::cout << "Undo: Pushed command '" << undoStack_.back()->getDescription()
              << "' (stack size: " << undoStack_.size() << ")" << std::endl;
}

bool UndoStack::undo() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (undoStack_.empty()) {
        return false;
    }

    // Pop from undo stack
    std::unique_ptr<UndoCommand> command = std::move(undoStack_.back());
    undoStack_.pop_back();

    // Undo the command
    command->undo();

    // Push to redo stack
    redoStack_.push_back(std::move(command));

    std::cout << "Undo: Reverted '" << redoStack_.back()->getDescription() << "'" << std::endl;

    return true;
}

bool UndoStack::redo() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (redoStack_.empty()) {
        return false;
    }

    // Pop from redo stack
    std::unique_ptr<UndoCommand> command = std::move(redoStack_.back());
    redoStack_.pop_back();

    // Re-execute the command
    command->execute();

    // Push back to undo stack
    undoStack_.push_back(std::move(command));

    std::cout << "Redo: Re-applied '" << undoStack_.back()->getDescription() << "'" << std::endl;

    return true;
}

bool UndoStack::canUndo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !undoStack_.empty();
}

bool UndoStack::canRedo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !redoStack_.empty();
}

void UndoStack::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    undoStack_.clear();
    redoStack_.clear();
    std::cout << "Undo: Cleared all history" << std::endl;
}

const char* UndoStack::getUndoDescription() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (undoStack_.empty()) {
        return nullptr;
    }
    return undoStack_.back()->getDescription();
}

const char* UndoStack::getRedoDescription() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (redoStack_.empty()) {
        return nullptr;
    }
    return redoStack_.back()->getDescription();
}

size_t UndoStack::getMemoryUsage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getMemoryUsageUnlocked();
}

size_t UndoStack::getMemoryUsageUnlocked() const {
    size_t total = 0;
    for (const auto& cmd : undoStack_) {
        total += cmd->getMemoryUsage();
    }
    for (const auto& cmd : redoStack_) {
        total += cmd->getMemoryUsage();
    }

    return total;
}

size_t UndoStack::getUndoCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return undoStack_.size();
}

size_t UndoStack::getRedoCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return redoStack_.size();
}

void UndoStack::enforceMemoryLimit() {
    // Remove oldest commands if memory usage exceeds limit
    // Note: Called from within locked context, so use unlocked version
    while (!undoStack_.empty() && getMemoryUsageUnlocked() > maxMemoryBytes_) {
        std::cout << "Undo: Memory limit exceeded, removing oldest command" << std::endl;
        undoStack_.erase(undoStack_.begin());
    }
}

void UndoStack::enforceCommandLimit() {
    // Remove oldest commands if count exceeds limit
    while (undoStack_.size() > maxCommands_) {
        std::cout << "Undo: Command limit exceeded, removing oldest command" << std::endl;
        undoStack_.erase(undoStack_.begin());
    }
}
