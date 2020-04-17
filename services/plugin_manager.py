from plugin import Plugin

class PluginManager:
    """
    Class keeps a list of separate plugins and gives them CPU time for executing
    """
    def __init__(self):
        """
        Constructor, initiate empty plugin list
        """
        self.plugins = []

    def add(self, plugin):
        """
        Adds plugin to the list of plugins that are executed
        """
        assert isinstance(plugin, Plugin)
        self.plugins.append(plugin)

    def processPlugins(self):
        """
        Gives CPU time to plugins to execute their part of work.
        Work that are being done in every plugin by single call to plugin's update() should be short enough in order not to block the main thread
        """
        for plugin in self.plugins:
            plugin.update()
