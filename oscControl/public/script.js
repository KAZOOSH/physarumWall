document.addEventListener('DOMContentLoaded', function() {
    const minHandle = document.getElementById('min-handle');
    const maxHandle = document.getElementById('max-handle');
    const sliderTrack = document.querySelector('.slider-track');
    const sliderRange = document.querySelector('.slider-range');
    const minValueDisplay = document.getElementById('min-value');
    const maxValueDisplay = document.getElementById('max-value');
    
    const minValue = 10;
    const maxValue = 240;
    let currentMinValue = 30;
    let currentMaxValue = 80;
    
    // Initialize positions
    updateHandlePositions();
    
    // Handle dragging
    let activeHandle = null;
    
    function onStart(e, handle) {
      activeHandle = handle;
      document.addEventListener('mousemove', onMove);
      document.addEventListener('mouseup', onEnd);
      document.addEventListener('touchmove', onMove);
      document.addEventListener('touchend', onEnd);
      e.preventDefault();
    }
    
    function onMove(e) {
      if (!activeHandle) return;
      
      const trackRect = sliderTrack.getBoundingClientRect();
      const x = (e.clientX || e.touches[0].clientX) - trackRect.left;
      const percent = Math.min(Math.max(x / trackRect.width, 0), 1);
      const value = Math.round(minValue + percent * (maxValue - minValue));
      
      if (activeHandle === minHandle) {
        currentMinValue = Math.min(value, currentMaxValue - 1);
        sendValue("/physarum/setMinTimeScenarioChange",currentMinValue);
      } else {
        currentMaxValue = Math.max(value, currentMinValue + 1);
        sendValue("/physarum/setMaxTimeScenarioChange",currentMaxValue);
      }
      
      updateHandlePositions();
    }
    
    function onEnd() {
      activeHandle = null;
      document.removeEventListener('mousemove', onMove);
      document.removeEventListener('mouseup', onEnd);
      document.removeEventListener('touchmove', onMove);
      document.removeEventListener('touchend', onEnd);
    }
    
    function updateHandlePositions() {
      const minPercent = (currentMinValue - minValue) / (maxValue - minValue);
      const maxPercent = (currentMaxValue - minValue) / (maxValue - minValue);
      
      minHandle.style.left = minPercent * 100 + '%';
      maxHandle.style.left = maxPercent * 100 + '%';
      sliderRange.style.left = minPercent * 100 + '%';
      sliderRange.style.width = (maxPercent - minPercent) * 100 + '%';
      
      minValueDisplay.textContent = currentMinValue;
      maxValueDisplay.textContent = currentMaxValue;
    }
    
    minHandle.addEventListener('mousedown', (e) => onStart(e, minHandle));
    maxHandle.addEventListener('mousedown', (e) => onStart(e, maxHandle));
    minHandle.addEventListener('touchstart', (e) => onStart(e, minHandle));
    maxHandle.addEventListener('touchstart', (e) => onStart(e, maxHandle));
  });






  async function sendValue(address,value){
    const statusElement = document.getElementById('status');
    try {
      // Visual feedback while sending
      statusElement.textContent = `Sending: ${address} with value: ${value}...`;
      statusElement.className = 'status';
      
      // Send OSC message with argument
      const response = await fetch('/send-osc-with-args', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ 
          address,
          args: [value] 
        }),
      });
      
      const data = await response.json();
      
      if (data.success) {
        statusElement.textContent = `Sent: ${address} with value: ${value}`;
        statusElement.className = 'status success';
      } else {
        throw new Error(data.error || 'Failed to send OSC message');
      }
    } catch (error) {
      console.error('Error sending OSC message:', error);
      statusElement.textContent = `Error: ${error.message}`;
      statusElement.className = 'status error';
    } finally {
      // Clear status message after a delay
      setTimeout(() => {
        statusElement.textContent = 'Ready';
        statusElement.className = 'status';
      }, 3000);
    }
  }