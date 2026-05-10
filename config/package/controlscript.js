function Controller() {
    // Auto-select all components by default
    installer.setDefaultPageVisible(QInstaller.ComponentSelection, true);
}

Controller.prototype.createOperationsForArchive = function(archive) {
    // Add default installation operations
}
