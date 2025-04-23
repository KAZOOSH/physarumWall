// app.js - Main server file with fixed MIME types
const express = require('express');
const path = require('path');
const osc = require('node-osc');
const app = express();
const PORT = process.env.PORT || 3000;

// Create OSC client for sending messages
const client = new osc.Client('localhost', 8888); // Default SuperCollider port, adjust if needed

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

// API endpoint for sending OSC messages
app.post('/send-osc', (req, res) => {
  const { address } = req.body;
  
  if (!address) {
    return res.status(400).json({ error: 'OSC address required' });
  }

  // Send the OSC message (without arguments)
  client.send(address, () => {
    console.log(`Sent OSC message to ${address}`);
  });

  res.json({ success: true, message: `OSC message sent to ${address}` });
});

// Start the server
app.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
});