from PyQt5.QtCore import QObject, QThread, QCoreApplication, pyqtSignal, pyqtSlot

from backend.core.event_manager import event_manager


class _GuiBridge(QObject):
    """Lives in the GUI thread; events emitted from other threads are queued to it."""
    publish_requested = pyqtSignal(str, tuple)

    def __init__(self):
        super().__init__()
        self.publish_requested.connect(self._publish)

    @pyqtSlot(str, tuple)
    def _publish(self, event_type, args):
        event_manager.publish(event_type, *args)


# created at import time, which happens in the GUI thread during start-up
_bridge = _GuiBridge()


def publish_gui(event_type, *args):
    """event_manager.publish() that is safe to call from a worker thread.

    Qt widgets may only be touched from the GUI thread. From the GUI thread the
    event is published directly, from any other thread it is queued to the GUI
    thread (so it runs asynchronously and returns nothing).
    """
    app = QCoreApplication.instance()
    if app is None or QThread.currentThread() == app.thread():
        event_manager.publish(event_type, *args)
    else:
        _bridge.publish_requested.emit(event_type, args)
