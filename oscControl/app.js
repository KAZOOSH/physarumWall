// app.js - Main server file with fixed MIME types and live route
const express = require('express');
const path = require('path');
const osc = require('node-osc');
const app = express();
const PORT = process.env.PORT || 3000;

// Create OSC client for sending messages
const client = new osc.Client('localhost', 8888);

// Create OSC server for receiving messages
const server = new osc.Server(3333, 'localhost', () => {
  console.log('OSC Server is listening on port 3333');
});

// Store current parameter values for broadcasting to connected clients
let currentParameterValues = {};

// Store WebSocket connections for real-time updates
const WebSocket = require('ws');
const wss = new WebSocket.Server({ port: 8080 });

wss.on('connection', (ws) => {
  console.log('WebSocket client connected');
  
  // Send current values to newly connected client
  ws.send(JSON.stringify({
    type: 'parameterUpdate',
    values: currentParameterValues
  }));
  
  ws.on('close', () => {
    console.log('WebSocket client disconnected');
  });
});

// OSC message handler for valueUpdate
server.on('message', (msg) => {
  const [address, ...args] = msg;
  
  if (address === '/valueUpdate' && args.length >= 2) {
    const paramName = args[0];
    const value = args[1];
    
    currentParameterValues[paramName] = value;
    
    // Broadcast to all connected WebSocket clients
    const updateMessage = JSON.stringify({
      type: 'parameterUpdate',
      paramName,
      value
    });
    
    wss.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(updateMessage);
      }
    });
    
    console.log(`Received parameter update: ${paramName} = ${value}`);
  }
});

// Explicitly set MIME types
app.use(express.static(path.join(__dirname, 'public'), {
  setHeaders: (res, path) => {
    if (path.endsWith('.js')) {
      res.setHeader('Content-Type', 'application/javascript');
    } else if (path.endsWith('.css')) {
      res.setHeader('Content-Type', 'text/css');
    }
  }
}));

app.use(express.json());

// Serve the live control page
app.get('/live', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'live.html'));
});

// API endpoint for sending OSC messages
app.post('/send-osc', (req, res) => {
  const { address } = req.body;
  
  if (!address) {
    return res.status(400).json({ error: 'OSC address required' });
  }

  client.send(address, () => {
    console.log(`Sent OSC message to ${address}`);
  });

  res.json({ success: true, message: `OSC message sent to ${address}` });
});

// API endpoint for sending OSC messages with arguments
app.post('/send-osc-with-args', (req, res) => {
  const { address, args } = req.body;
  
  if (!address) {
    return res.status(400).json({ error: 'OSC address required' });
  }

  client.send(address, args, () => {
    console.log(`Sent OSC message to ${address} with args: ${JSON.stringify(args)}`);
  });

  res.json({ success: true, message: `OSC message sent to ${address} with args: ${JSON.stringify(args)}` });
});

// Start the server
app.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
  console.log(`Live controls available at http://localhost:${PORT}/live`);
  console.log(`WebSocket server running on port 8080`);
});