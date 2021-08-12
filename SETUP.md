# SETUP

### Install OBS

```bash
sudo add-apt-repository ppa:obsproject/obs-studio
sudo apt update
sudo apt install obs-studio
```

### Install OBS WebSocket plugin

https://github.com/Palakis/obs-websocket/releases

### Install NVIDIA Video SDK
Requires an NVIDIA developer account

#### Video SDK
https://developer.nvidia.com/maxine-getting-started
```
sudo tar -xvf VideoFX-<version>.tar.gz -C /usr/local
```

#### CUDA
Install CUDA Toolkit (11.1) (ubuntu >= 20.04):
```
wget https://developer.download.nvidia.com/compute/cuda/11.1.1/local_installers/cuda_11.1.1_455.32.00_linux.run
sudo sh cuda_11.1.1_455.32.00_linux.run
```

Install CUDA Toolkit (11.3) (ubuntu >= 20.04):
```
wget https://developer.download.nvidia.com/compute/cuda/11.3.1/local_installers/cuda_11.3.1_465.19.01_linux.run
sudo sh cuda_11.3.1_465.19.01_linux.run
```

Check CUDA version with (>=11.1):
```
nvcc --version // this is the version that cmake will find; requires CUDA Toolkit
nvidia-smi // this is a backup method that might not give the correct CUDA version
```
https://developer.nvidia.com/cuda
```
sudo sh cuda_<version>_linux.run
```

#### TensorRT
Check TensorRT version with (==7.2.2.3):
```
echo $LD_LIBRARY_PATH

dpkg -l | grep tensorrt // not reliable
```

https://developer.nvidia.com/tensorrt
```
sudo tar -xvf TensorRT.<version>.Ubuntu-18.04.x86_64-gnu.cuda-<version>.cudnn<version>.tar.gz -C /usr/local
export PATH=$PATH:/usr/local/TensorRT-<version>/lib
```

#### cudnn
https://developer.nvidia.com/cudnn
```
sudo tar -xvf cudnn-<version>-linux-x64-v<version>.tgz -C /usr/local
```

#### OpenCV
https://docs.opencv.org/4.5.2/d7/d9f/tutorial_linux_install.html
```
# Install minimal prerequisites (Ubuntu 18.04 as reference)
sudo apt update && sudo apt install -y cmake g++ wget unzip
# Download and unpack sources
wget -O opencv.zip https://github.com/opencv/opencv/archive/master.zip
unzip opencv.zip
# Create build directory
mkdir -p build && cd build
# Configure
cmake  ../opencv-master
# Build
cmake --build .

sudo make install
```

# OLD/DEPRECATED

### Install OBS Streamlink plugin

https://github.com/dd-center/obs-streamlink

##### Building OBS Streamlink for Linux

Build OBS from source: https://github.com/obsproject/obs-studio/wiki/Install-Instructions#debian-based-build-directions

```bash
sudo apt install libpipewire-0.3-dev
```

Then build Streamlink

```bash
sudo apt install libavcodec-dev libavfilter-dev libavdevice-dev libavutil-dev libswscale-dev libavformat-dev libswresample-dev
```

```bash
mkdir build
cd build
cmake ..
```