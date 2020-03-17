"""
This module provides base class for plugins implementation.
Plugins are modules that implements some functions and can be attached to main module, so that plugin's code can run independently
and in parallel together with main module code.
To be attachable to main module, plugins must be dervived from Plugin class and must override it's update() function.
Update() function when called, give's CPU time to plugin in the main thread. Plugin should not take any long-running activities inside update() 
in order not to block main thread. If long-running activities is required, then plugin should create it's own separate thread and use update() 
only to communicate/syncronize with main thread.
Example could be a plugin that runs an external application. The application can be started in separate thread/process, and update() would be
used to check if the application already completed or not. When application is completed, update() report's to main module, preferrable by invoking 
callback function.
"""

from abc import ABCMeta, abstractmethod

class Plugin(metaclass=ABCMeta):
    """
    class Plugin works as interface for plugins - modules that can be attached to main module.
    All plugins should be derived from this class
    """

    @abstractmethod
    def update(self):
        """
        This method is called periodically from main thread and used for giving to plugin CPU time and/or synchrization with the main thread.
        Because code is executed in the main thread, it should be completed as fast as possible.
        If plugin performs long-running task, use separate thread for such task, and use update() to check if the task is completed and reporting
        to main module via callback function.
        """
        pass