# Wi-Fi and Bluetooth Enabled Pet Feeder

## Overview

This project is a Wi-Fi and Bluetooth-enabled pet feeder controlled by an ESP32. Based on
the [FAT PET FEEDER](https://www.youtube.com/watch?v=sCOkuyH7CPo)
by [Mom Will Be Proud • DIY channel](https://www.youtube.com/channel/UCqVfFr35soUMdDQoXc3hoOw), this version extends
functionality with Wi-Fi and Bluetooth capabilities, allowing for remote control and monitoring via a mobile app.

## Features

- **Network Scanning**: Scans for available Wi-Fi networks.
- **Wi-Fi and Bluetooth Connectivity**: Connects to Wi-Fi for remote control and supports Bluetooth for local setup and
  management.
- **MQTT Integration**: Connects to an MQTT broker for remote management.
- **Time Synchronization**: Synchronizes time via NTP.
- **Scheduled Feeding**: Dispenses food based on predefined schedules.
- **Manual Feeding**: Allows manual feeding via a physical button or mobile app.

## Components

- **ESP32-WROOM-32D**
- **SpringRC SM-S4303R Continuous Rotation Servo**
- **Wi-Fi/Bluetooth Connectivity**
- **HomeLink Integration**

## Setup Instructions

### Hardware Requirements

- ESP32-WROOM-32D
- Continuous rotation servo motor (SpringRC SM-S4303R)
- Button for manual feeding
- Power supply

### Wiring

1. **ESP32 Pin Connections**:
    - Connect the servo signal wire to GPIO 23.
    - Connect the button to GPIO 18.

2. **Power Supply**:
    - Ensure the servo and ESP32 are powered appropriately.

### Software Requirements

- PlatformIO with CLion (or VSCode)
- Arduino framework for PlatformIO
- Required Libraries: WiFi, PubSubClient, Time, NTPClient, etc.

### Code

1. **Clone the Repository**:
    ```bash
    git clone https://github.com/menezmethod/pet-feeder.git
    cd pet-feeder
    ```

2. **Open the Project in PlatformIO**:
    - Open your IDE (CLion or VSCode) with PlatformIO installed.
    - Open the `pet_feeder` folder in your IDE.

3. **Configure Wi-Fi and MQTT**:
    - Update your Wi-Fi SSID and password in the `main.cpp` file.
    - Configure the MQTT broker details.

4. **Build and Upload the Code**:
    - Use PlatformIO to build and upload the code to your ESP32.

## Usage

### Initial Setup

1. **Power On**:
    - Power on the ESP32. It will start by scanning for Wi-Fi networks.

2. **Connect to Wi-Fi**:
    - Send the SSID and password of your Wi-Fi network to the device via Bluetooth. You can use any Bluetooth terminal
      app to send the following commands:
        - To set the SSID: `ssid:<your_ssid>`
        - To set the password: `pass:<your_password>`

3. **Bluetooth Communication**:
    - The ESP32 uses the Bluetooth Low Energy (BLE) characteristic `FF01` to receive commands. This characteristic
      allows the device to receive the SSID and password for Wi-Fi setup.
    - Use a Bluetooth terminal app to connect to the ESP32 and write to the `FF01` characteristic with the SSID and
      password commands as described above.

4. **MQTT Connection**:
    - Ensure the ESP32 connects to the MQTT broker.

### Scheduled Feeding

- The feeder will dispense food based on the schedule defined in the code.

### Manual Feeding

- Press the button connected to GPIO 18 to manually dispense food.

## Acknowledgments

This project is based on the [FAT PET FEEDER](https://www.youtube.com/watch?v=sCOkuyH7CPo) created
by [Mom Will Be Proud • DIY channel](https://www.youtube.com/channel/UCqVfFr35soUMdDQoXc3hoOw). Their design and
open-source contribution served as the foundation for this enhanced version with added Wi-Fi and Bluetooth capabilities.

## Contributing

1. Fork the repository.
2. Create a new branch (`git checkout -b feature-branch`).
3. Make your changes.
4. Commit your changes (`git commit -am 'Add new feature'`).
5. Push to the branch (`git push origin feature-branch`).
6. Open a Pull Request.

## License

This project is licensed under the MIT License. See the `LICENSE` file for more details.

## Contact

**Luis Gimenez**  
Email: luisgimenezdev@gmail.com  
GitHub: [github.com/menezmethod](https://github.com/menezmethod)