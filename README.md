# bb_imgacquisition with Basler Camera Support

This is an edited version of bb_imgacquisition that includes support for Basler cameras. It is recommended to install the latest version of Pylon (currently 7.4 at the time of writing). The code also compiles with Pylon version 5 (tested with version 5.2).

## Instructions for compiling and running

These were tested on Ubuntu 20.04, 22.04, and 24.04 (see note for FLIR camera / Flycapture2 compatibility for 24.04)

### Dependencies

First install the necessary dependencies:

```bash
sudo apt install git cmake g++ libavcodec-dev libavformat-dev libavutil-dev libfmt-dev qtbase5-dev libboost-all-dev libopencv-dev libfdk-aac-dev nasm libass-dev libmp3lame-dev libopus-dev libvorbis-dev libx264-dev libx265-dev libxcb-xinput0 yasm libtool libc6 libc6-dev unzip wget libnuma1 libnuma-dev
```
### Nvidia drivers, CUDA, and Video Codec SDK for ffmpeg

Ensure that Nvidia drivers are installed (needed for hardware acceleration with ffmpeg).  CUDA is also needed.  There may be a newer driver than 550 available now.

```bash
sudo add-apt-repository ppa:graphics-drivers/ppa
sudo apt update
sudo apt install nvidia-driver-550
sudo reboot
sudo apt install nvidia-cuda-toolkit
```

The ffnvcodec is also needed.  The install instructions can also be found [on Nvidia's website](https://docs.nvidia.com/video-technologies/video-codec-sdk/11.1/ffmpeg-with-nvidia-gpu/index.html#setup) or [here](https://www.cyberciti.biz/faq/how-to-install-ffmpeg-with-nvidia-gpu-acceleration-on-linux/).

```bash
git clone https://git.videolan.org/git/ffmpeg/nv-codec-headers.git
cd nv-codec-headers && sudo make install
cd ..
```


### Install Pylon (Basler Cameras)

Download and install Pylon for Basler cameras:
Can download from the [Basler website](https://www2.baslerweb.com/en/downloads/software-downloads/software-pylon-7-4-0-linux-x86-64bit-debian/) to search versions, or use the command here:
```bash
wget https://www2.baslerweb.com/media/downloads/software/pylon_software/pylon_7_4_0_14900_linux_x86_64_debs.tar.gz
tar -zxvf pylon_7_4_0_14900_linux_x86_64_debs.tar.gz 
sudo dpkg -i pylon_7.4.0.14900-deb0_amd64.deb
```

### Run the Basler USB script for system settings
This increases the USB kernel spade and file handle limit.  These commands automatically find the installed location and run the script
```bash
# Find the path to the pylon installation directory
PYLON_PATH=$(dpkg -L pylon | grep 'pylon/share/pylon/setup-usb.sh' | sed 's|/share/pylon/setup-usb.sh||')

# Run the setup script
if [ -f "$PYLON_PATH/share/pylon/setup-usb.sh" ]; then
  sudo bash "$PYLON_PATH/share/pylon/setup-usb.sh"
else
  echo "Setup script not found or not executable. Please check your installation."
fi
```

### Compile FFmpeg from Source

Download and compile FFmpeg from source with the required packages enabled. If you have FFmpeg installed through Conda or a package manager, you need to remove it first.  Cuda library locations need to be changed if they are installed other than the default locations

```bash
git clone --depth 1 https://git.ffmpeg.org/ffmpeg.git ffmpeg
cd ffmpeg
./configure --prefix=/usr/local --enable-gpl --enable-nonfree --enable-libass --enable-libfreetype --enable-zlib --enable-libmp3lame --enable-libopus --enable-libvorbis --enable-libx264 --enable-libx265 --enable-libfdk-aac --extra-libs=-lpthread --extra-libs=-lm --enable-nvenc --enable-cuda-nvcc --enable-libnpp --extra-cflags=-I/usr/local/cuda/include --extra-ldflags=-L/usr/local/cuda/lib64
make -j 8
sudo make install
cd ..
```

After compiling, verify that the nvenc encoder is installed.  This should highlight the encoder.
```bash
ffmpeg -encoders | grep hevc_nvenc
```

### Clone and Build bb_imgacquisition

```bash
git clone https://github.com/jacobdavidson/bb_imgacquisition.git
cd bb_imgacquisition
git fetch origin basler_support_update2024
git checkout basler_support_update2024

mkdir build && cd build
cmake ..
make -j 8
```

### Set permission to enable high priority runing
```
d# sudo nano /etc/security/limits.conf
# add this line:
# beesbook  -  nice    -20
```

Restart in order for changes in USB settings and permissions to take effect


## Running

Execute the application for the first time:

```bash
cd build
./bb_imgacquisition
```

The first time running will generate a blank `config.json` located at `~/.config/bb_imgacquisition/config.json`. This needs to be edited to include the camera serial numbers and other parameters. See the included `config.json` for an example that can be adapted.

Note!  For high resolutions, the ffmpeg encoding only supports multiples of 64 (for example, 5312x4608).  Other resolutions will lead to jumbled videos due to the encoder. 

Running for long times:  Use the script 'run_bb_imgacquisition.sh', to automatically restart with a date of crashes
```bash
./run_bb_imgacquisition.sh
```


## Optional - Install these before building to have XIMEA and/or FLIR camera support

#### Install XIMEA SDK

Dependencies:

```bash
sudo apt install libxcb-cursor0
```

Installation notes are available at [XIMEA Linux Software Package](https://www.ximea.com/support/wiki/apis/ximea_linux_software_package).

```bash
wget https://www.ximea.com/downloads/recent/XIMEA_Linux_SP.tgz
tar -zxvf XIMEA_Linux_SP.tgz
cd package
bash install
cd ..
```

#### Install FlyCapture2 for FLIR camera support

Download and install FlyCapture2:

Note!  For Ubuntu 24, there are compatibility issues, because flycapture2 is no longer being updated.  So, need to add Ubuntu 22 repositories in order to install.
```bash
## Needed for Ubunt 24.04 only
sudo add-apt-repository 'deb http://archive.ubuntu.com/ubuntu jammy main universe'
sudo apt update
```

```bash
git clone https://github.com/ErnestDeiven/flycapture2
cd flycapture2/
tar -zxvf flycapture2-2.13.3.31-amd64-pkg_Ubuntu16.04.tgz
cd flycapture2-2.13.3.31-amd64/
sudo bash install_flycapture.sh
sudo systemctl restart udev  # this will restart udev, in case the error " /etc/init.d/udev: not found" comes up
cd ../../..
```
