import serial
import threading
import queue

# Initialize serial connection
ser = serial.Serial('/dev/ttyAMA1', 115200, 8, serial.PARITY_NONE, 1, 1)

def send_event(event_code : str):
    ser.write((event_code + '\n').encode())

# Thread-safe queue for incoming commands
command_queue = queue.Queue()

# Flag to control the polling thread
_running = False
_poll_thread = None


def _uart_poll_loop():
    """Internal function that continuously polls for UART messages"""
    global _running
    buffer = ""
    
    while _running:
        try:
            if ser.in_waiting > 0:
                # Read available bytes
                data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                buffer += data
                
                # Process complete lines (messages ending with newline)
                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    line = line.strip()
                    
                    if line:  # Only process non-empty lines
                        command_queue.put(line.upper())
        except Exception as e:
            print(f"UART polling error: {e}")

def start_polling():
    """Start the UART polling thread"""
    global _running, _poll_thread
    
    if _running:
        return  # Already running
    
    _running = True
    _poll_thread = threading.Thread(target=_uart_poll_loop, daemon=True)
    _poll_thread.start()

def stop_polling():
    """Stop the UART polling thread"""
    global _running, _poll_thread
    
    _running = False
    if _poll_thread:
        _poll_thread.join(timeout=1.0)
        _poll_thread = None

def get_command():
    """
    Get the next command from the queue (non-blocking)
    Returns the command string or None if queue is empty
    """
    try:
        return command_queue.get_nowait()
    except queue.Empty:
        return None

def clear_commands():
    """Clear all pending commands from the queue"""
    while not command_queue.empty():
        try:
            command_queue.get_nowait()
        except queue.Empty:
            break
