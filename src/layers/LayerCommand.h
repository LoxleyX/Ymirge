#pragma once

#include "LayerStack.h"
#include "TerrainLayer.h"
#include "LayerGroup.h"
#include <memory>
#include <string>
#include <vector>

/**
 * Base class for layer operation commands (Command Pattern)
 *
 * Supports undo/redo for layer operations like:
 * - Property changes (opacity, blend mode, visibility)
 * - Layer operations (add, remove, move, merge)
 * - Group operations (create, group, ungroup)
 *
 * Separate from HeightMapEditCommand to avoid mixing concerns.
 */
class LayerCommand {
public:
    virtual ~LayerCommand() = default;

    /**
     * Execute the command (redo)
     */
    virtual void execute() = 0;

    /**
     * Undo the command
     */
    virtual void undo() = 0;

    /**
     * Get command description for UI
     */
    virtual std::string getDescription() const = 0;

    /**
     * Get memory usage in bytes
     */
    virtual size_t getMemoryUsage() const = 0;
};

/**
 * Command: Change layer property (opacity, blend mode, visibility, lock)
 */
class LayerPropertyCommand : public LayerCommand {
public:
    enum class PropertyType {
        OPACITY,
        BLEND_MODE,
        VISIBILITY,
        LOCKED,
        NAME
    };

    LayerPropertyCommand(LayerStack* stack, size_t layerIndex, PropertyType type);

    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    size_t getMemoryUsage() const override;

    // Set new value (call before execute)
    void setOpacity(float newOpacity);
    void setBlendMode(BlendMode newMode);
    void setVisibility(bool newVisible);
    void setLocked(bool newLocked);
    void setName(const std::string& newName);

private:
    LayerStack* stack_;
    size_t layerIndex_;
    PropertyType type_;

    // Store old and new values
    float oldOpacity_, newOpacity_;
    BlendMode oldBlendMode_, newBlendMode_;
    bool oldVisibility_, newVisibility_;
    bool oldLocked_, newLocked_;
    std::string oldName_, newName_;

    void captureOldValue();
};

/**
 * Command: Add new layer
 */
class AddLayerCommand : public LayerCommand {
public:
    AddLayerCommand(LayerStack* stack, std::unique_ptr<LayerBase> layer, size_t insertIndex);

    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    size_t getMemoryUsage() const override;

private:
    LayerStack* stack_;
    std::unique_ptr<LayerBase> layer_;
    size_t insertIndex_;
    bool executed_;
};

/**
 * Command: Remove layer
 */
class RemoveLayerCommand : public LayerCommand {
public:
    RemoveLayerCommand(LayerStack* stack, size_t layerIndex);

    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    size_t getMemoryUsage() const override;

private:
    LayerStack* stack_;
    size_t layerIndex_;
    std::unique_ptr<LayerBase> removedLayer_;
    bool executed_;
};

/**
 * Command: Move layer to different position
 */
class MoveLayerCommand : public LayerCommand {
public:
    MoveLayerCommand(LayerStack* stack, size_t fromIndex, size_t toIndex);

    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    size_t getMemoryUsage() const override;

private:
    LayerStack* stack_;
    size_t fromIndex_;
    size_t toIndex_;
};

/**
 * Command: Merge layer down
 */
class MergeLayerCommand : public LayerCommand {
public:
    MergeLayerCommand(LayerStack* stack, size_t layerIndex);

    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    size_t getMemoryUsage() const override;

private:
    LayerStack* stack_;
    size_t layerIndex_;
    std::unique_ptr<LayerBase> topLayerBackup_;
    HeightMap bottomLayerBackup_;
    bool executed_;
};

/**
 * Command: Create new empty group
 */
class CreateGroupCommand : public LayerCommand {
public:
    CreateGroupCommand(LayerStack* stack, const std::string& groupName, size_t insertIndex);

    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    size_t getMemoryUsage() const override;

private:
    LayerStack* stack_;
    std::string groupName_;
    size_t insertIndex_;
    std::unique_ptr<LayerBase> group_;
    bool executed_;
};

/**
 * Command: Group selected layers together
 */
class GroupLayersCommand : public LayerCommand {
public:
    GroupLayersCommand(LayerStack* stack, const std::string& groupName,
                      const std::vector<size_t>& layerIndices);

    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    size_t getMemoryUsage() const override;

private:
    LayerStack* stack_;
    std::string groupName_;
    std::vector<size_t> layerIndices_;
    std::vector<std::unique_ptr<LayerBase>> removedLayers_;
    size_t groupInsertIndex_;
    bool executed_;
};

/**
 * Command: Ungroup - move group's children to parent level
 */
class UngroupCommand : public LayerCommand {
public:
    UngroupCommand(LayerStack* stack, size_t groupIndex);

    void execute() override;
    void undo() override;
    std::string getDescription() const override;
    size_t getMemoryUsage() const override;

private:
    LayerStack* stack_;
    size_t groupIndex_;
    std::unique_ptr<LayerBase> removedGroup_;
    bool executed_;
};

/**
 * Undo stack specifically for layer operations
 * Separate from heightmap undo to avoid confusion
 */
class LayerUndoStack {
public:
    LayerUndoStack(size_t maxCommands = 50);

    /**
     * Push new command and execute it
     */
    void push(std::unique_ptr<LayerCommand> command);

    /**
     * Undo last command
     */
    bool undo();

    /**
     * Redo previously undone command
     */
    bool redo();

    /**
     * Clear all commands
     */
    void clear();

    /**
     * Check if can undo/redo
     */
    bool canUndo() const { return currentIndex_ > 0; }
    bool canRedo() const { return currentIndex_ < commands_.size(); }

    /**
     * Get current command descriptions for UI
     */
    std::string getUndoDescription() const;
    std::string getRedoDescription() const;

    /**
     * Get memory usage
     */
    size_t getMemoryUsage() const;

private:
    std::vector<std::unique_ptr<LayerCommand>> commands_;
    size_t currentIndex_;
    size_t maxCommands_;
};
