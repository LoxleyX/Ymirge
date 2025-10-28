#pragma once

/**
 * UndoCommand - Base class for undoable commands
 *
 * Implements the Command pattern for undo/redo functionality.
 * Each command must implement execute() and undo() methods.
 */
class UndoCommand {
public:
    virtual ~UndoCommand() = default;

    /**
     * Execute the command (apply changes)
     */
    virtual void execute() = 0;

    /**
     * Undo the command (revert changes)
     */
    virtual void undo() = 0;

    /**
     * Get command description for UI display
     */
    virtual const char* getDescription() const = 0;

    /**
     * Get memory usage in bytes
     */
    virtual size_t getMemoryUsage() const = 0;
};
