# rft-sensor-ros

**Alchemist Project (2023–2025)** · Soft Robotics and Bionics Lab (SRBL), Seoul National University

C++ / ROS1 code for reading a **Robotous RFT60-HA01 6-axis force/torque sensor (load cell)**, and for logging it together with our 3-axis tactile sensor.

**Sensor:** [Robotous RFT60-HA01](https://www.robotous.com/), a hollow-core 6-axis F/T sensor with a 60 mm housing. This repo uses the **USB interface**, which works as a serial (UART) port and shows up as `/dev/ttyUSB*`.

The logged data was used as ground truth to calibrate the tactile sensor with an MLP. See [alche-act-feedback](https://github.com/omletkang/alche-act-feedback).

## Contents

```
rft/              ROS package
├─ src/sensor_RFT.cpp     rft_node: reads the RFT sensor, publishes Fx Fy Fz Tx Ty Tz → /sensorRFT
├─ src/sensor_logger.cpp  sensor_logger: logs /sensorRFT + /sensor_data (tactile) → sensor_data_log.csv
└─ src/RFT_UART.h
RFT_UART_study/   Standalone version, written to learn the sensor's UART protocol
└─ main.cpp, RFT_UART.cpp/.h, compile.sh
```

## Usage

**ROS node** (Ubuntu 20.04, ROS Noetic):

```bash
cp -r rft <catkin_ws>/src/ && cd <catkin_ws> && catkin_make && source devel/setup.bash
rosrun rft rft_node        # publishes /sensorRFT
rosrun rft sensor_logger   # needs /sensor_data from alche_srbl's sensor_node.py
```

**Standalone test:**

```bash
cd RFT_UART_study && sh compile.sh && ./app   # press 'm' to measure, ESC to quit
```

- **Serial settings:** 921600 baud, 8N1 on `/dev/ttyUSB<portnum>`. Change `portnum` in the source to match your device.
- **Dividers:** force 50, torque 2000 (see the RFT sensor manual).
- **WSL2:** attach the USB device with `usbipd-win` first.

## Note

The UART protocol handling was written with reference to the manufacturer's official RFT UART sample code (Rev1.2). That sample code is not included in this repository.
