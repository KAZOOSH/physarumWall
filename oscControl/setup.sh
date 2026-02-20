sudo mkdir /var/www
sudo cp osc-control.service /etc/systemd/system/

chmod +x start.sh

cd ..
sudo cp -r oscControl /var/www

# Reload systemd so it recognizes the new service
sudo systemctl daemon-reload

# Enable the service to start on boot
sudo systemctl enable osc-control

# Start the service immediately
sudo systemctl start osc-control

# Check if the service is running
systemctl status osc-control

