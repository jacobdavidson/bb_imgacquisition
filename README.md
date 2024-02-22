# bb_imgacquisition with Basler Camera Support

This is an edited version of bb_imgacquisition that includes support for Basler cameras. It is recommended to install the latest version of Pylon (currently 7.4 at the time of writing). The code also compiles with Pylon version 5 (tested with version 5.2).

## Instructions for compiling and running

These were tested on Ubuntu 20.04 and 22.04.

### Prerequisites

First, ensure that Nvidia drivers are installed (needed for hardware acceleration with ffmpeg). Note that CUDA is not needed.

```bash
sudo add-apt-repository ppa:graphics-drivers/ppa
sudo apt update
sudo apt install nvidia-driver-545
```

### Dependencies

Install the necessary dependencies:

```bash
sudo apt install git cmake g++ libavcodec-dev libavformat-dev libavutil-dev libfmt-dev qtbase5-dev libboost-all-dev libopencv-dev libglademm-2.4-1v5 libgtkmm-2.4-dev libglademm-2.4-dev libgtkglextmm-x11-1.2-dev libfmt-dev libfdk-aac-dev nasm libass-dev libmp3lame-dev libopus-dev libvorbis-dev libx264-dev libx265-dev libxcb-xinput0
```

### Install Pylon (Basler Cameras)

Download and install Pylon for Basler cameras:
Can download from the [Basler website](https://www2.baslerweb.com/en/downloads/software-downloads/software-pylon-7-4-0-linux-x86-64bit-debian/) to search versions, or use the command here:
```bash
wget https://www2.baslerweb.com/media/downloads/software/pylon_software/pylon_7_4_0_14900_linux_x86_64_debs.tar.gz
tar -zxvf pylon_7_4_0_14900_linux_x86_64_debs.tar.gz 
sudo dpkg -i pylon_7.4.0.14900-deb0_amd64.deb
```

### Compile FFmpeg from Source

Download and compile FFmpeg from source with the required packages enabled. If you have FFmpeg installed through Conda or a package manager, you need to remove it first.

```bash
git clone --depth 1 https://git.ffmpeg.org/ffmpeg.git ffmpeg
cd ffmpeg
./configure --prefix=/usr/local --enable-gpl --enable-nonfree --enable-libass --enable-libfreetype --enable-zlib --enable-libmp3lame --enable-libopus --enable-libvorbis --enable-libx264 --enable-libx265 --enable-libfdk-aac --extra-libs=-lpthread --extra-libs=-lm
make
sudo make install
cd ..
```

### Clone and Build bb_imgacquisition

```bash
git clone https://github.com/jacobdavidson/bb_imgacquisition.git
cd bb_imgacquisition
git fetch origin basler_support_update2024
git checkout basler_support_update2024

mkdir build && cd build
cmake ..
make
```

### Running

Execute the application:

```bash
./bb_imgacquisition
```

Note that the first time running will generate a blank `config.json` located at `~/.config/bb_imgacquisition/config.json`. This needs to be edited to include the camera serial numbers and other parameters. See the included `config.json` for an example that can be adapted.

### Optional - Install these before building to have XIMEA and/or FLIR camera support

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

#### Install FlyCapture2

Download and install FlyCapture2:

```bash
git clone https://github.com/ErnestDeiven/flycapture2
cd flycapture2/
tar -zxvf flycapture2-2.13.3.31-amd64-pkg_Ubuntu16.04.tgz
cd flycapture2-2.13.3.31-amd64/
sudo bash install_flycapture.sh
cd ../../..
```
