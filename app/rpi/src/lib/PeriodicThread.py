import threading
import time

class PeriodicThread(threading.Thread):
    def __init__(self, interval, target_function, *args, **kwargs):
        """
        :param interval: Time in seconds between executions
        :param target_function: The function to execute periodically
        """
        super().__init__()
        self.interval = interval
        self.target_function = target_function
        self.args = args
        self.kwargs = kwargs
        self._stop_event = threading.Event()
        # Set as daemon so the main program can exit if this thread is running
        self.daemon = True 

    def run(self):
        """Runs the continuous loop in a background thread."""
        while not self._stop_event.is_set():
            start_time = time.time()
            
            # Execute the targeted task
            try:
                self.target_function(*self.args, **self.kwargs)
            except Exception as e:
                print(f"Error executing periodic task: {e}")
            
            # Account for execution time to prevent cumulative drift
            elapsed = time.time() - start_time
            remaining_sleep = max(0.0, self.interval - elapsed)
            
            # Wait for the next cycle OR until stop() is called
            # wait() returns True if the flag was set, False if it timed out
            if self._stop_event.wait(timeout=remaining_sleep):
                break

    def stop(self):
        """Signals the thread to stop and breaks the wait loop immediately."""
        self._stop_event.set()