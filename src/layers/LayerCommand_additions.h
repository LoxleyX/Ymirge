
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
    std::unique_ptr<LayerBase> group_;  // Store group for undo
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
    std::vector<size_t> layerIndices_;  // Indices of layers to group (sorted descending)
    std::vector<std::unique_ptr<LayerBase>> removedLayers_;  // Backup for undo
    size_t groupInsertIndex_;  // Where group was inserted
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
    std::unique_ptr<LayerBase> removedGroup_;  // Store removed group
    bool executed_;
};

