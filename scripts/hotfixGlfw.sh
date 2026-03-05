# only for arch linux installing old glfw

cd ~/Downloads
wget https://github.com/glfw/glfw/releases/download/3.3.10/glfw-3.3.10.zip
unzip glfw-3.3.10.zip
cd glfw-3.3.10
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr          -DBUILD_SHARED_LIBS=ON          -DGLFW_BUILD_WAYLAND=OFF          -DGLFW_BUILD_X11=ON
make -j$(nproc)
sudo make install
