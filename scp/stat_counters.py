"""
Statistic metrics is stored in this module
"""

class StatCounters:
    """
    Request-related statistics
    """
    class Requests:
        received = 0    # Total amount of requests received by the script
        processed = 0   # Amount of successfully processed requests
        rejected = 0    # Amount of requests that script could not process and requeued them
        requeued = 0    # Amount of requests that were rejected for any reason
        # Amount of failed request = rejected + requeued

        malformed = 0   # wrongly formatted requests

    class Errors:
        downloading = 0
        audio = 0
        frame = 0
        mixing = 0
        uploading = 0