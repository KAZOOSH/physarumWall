from pythonosc.dispatcher import Dispatcher
from pythonosc import osc_server
import mido

print(mido.get_output_names())

# Configure your MIDI output
midi_output = mido.open_output("PythonToAbleton 1") # This will open the default output

note_buffer = {}#{"1":{"note":123,"ts":3434}

# Function to handle incoming OSC messages and send MIDI notes
def osc_to_midi_note_on(address, *args):
    #print(f"Received OSC message at {address} with args: {args}")
    
    # OSC sends note, velocity, and channel
    try:
        touch_id = int(args[0])
        note = int(args[1])
        velocity = int(args[2])
        channel = int(args[3]) if len(args) > 3 else 0
        # send note off if note has changed
        # print (note_buffer)
        if (touch_id in note_buffer and note_buffer[touch_id]["note"] != note):
            msg = mido.Message('note_on', note=note_buffer[touch_id]["note"], velocity=0, channel=channel)
            midi_output.send(msg)
            print(f"Sent MIDI Note Off: {msg} {touch_id}")

            # Create and send MIDI Note On
            msg = mido.Message('note_on', note=note, velocity=velocity, channel=channel)
            midi_output.send(msg)
            print(f"Sent MIDI Note On: {msg} {touch_id}")

        if not touch_id in note_buffer:
            # Create and send MIDI Note On
            msg = mido.Message('note_on', note=note, velocity=velocity, channel=channel)
            midi_output.send(msg)
            print(f"Sent MIDI Note On: {msg} {touch_id}")
            
        note_buffer[touch_id] = {}
        note_buffer[touch_id]["note"] = note

        

        
    except Exception as e:
        print(f"Error handling OSC to MIDI: {e}")
        print (note_buffer)

def osc_to_midi_note_off(address, *args):
    #print(f"Received OSC message at {address} with args: {args}")
    
    # OSC sends note and channel
    try:
        touch_id = int(args[0])
        note = int(args[1])
        velocity = int(args[2])
        channel = int(args[3]) if len(args) > 3 else 0
        del note_buffer[touch_id]
        # send Note Off
        msg = mido.Message('note_off', note=note, velocity=0, channel=channel)
        midi_output.send(msg)
        #print(f"Sent MIDI Note Off: {msg}")

    except Exception as e:
        print(f"Error handling OSC to MIDI: {e}")
        print (note_buffer)

def osc_to_midi_cc(address, *args):
    try:
        cc_number = int(args[0])
        value = int(args[1])
        channel = int(args[2]) if len(args) > 2 else 0
        if channel == 3:
            msg = mido.Message('control_change', control=0, value=0, channel=4)

            msg = mido.Message('control_change', control=cc_number, value=value, channel=channel)
            midi_output.send(msg)
            note_buffer = {}
            print(f"Sent MIDI CC: {msg}")
        else:            
            msg = mido.Message('control_change', control=cc_number, value=value, channel=channel)
            midi_output.send(msg)
#        print(f"Sent MIDI CC: {msg}")
    except Exception as e:
        print(f"Error sending MIDI CC: {e}")

# Setup dispatcher
dispatcher = Dispatcher()
dispatcher.map("/midi/note_on", osc_to_midi_note_on) # Bind OSC path to handler
dispatcher.map("/midi/note_off", osc_to_midi_note_off) # Bind OSC path to handler
dispatcher.map("/midi/cc", osc_to_midi_cc) # Bind OSC path to handler

# Start OSC server
ip = "0.0.0.0"
port = 8000
print(f"Listening for OSC on {ip}:{port}")
server = osc_server.BlockingOSCUDPServer((ip, port), dispatcher)
server.serve_forever()
