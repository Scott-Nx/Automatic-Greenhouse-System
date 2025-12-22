# Automatic Greenhouse System Getting Started Guide

🌐 **[ภาษาไทย (Thai Version)](README.th.md)**

## Prerequisites

### Hardware Components

1. **Arduino Uno** (1 board)
2. **Soil Moisture Sensor** (1 unit)
3. **4-Channel Relay** Active-Low type (1 module)
4. **DC Water Pump** with water tubing (1 unit)
5. **DC Fan** (1 unit)
6. **Jumper Wires** (multiple)
7. **USB Type-B Cable** for connecting Arduino to computer
8. **Power Supply** for water pump and fan

### Software Requirements

- **Arduino IDE** (for writing and uploading code)

---

## 🔌 Step 1: Wiring the Circuit

### 1.1 Connect the Soil Moisture Sensor

| Sensor Pin | Connect to Arduino |
| ---------- | ------------------ |
| VCC        | 5V                 |
| GND        | GND                |
| AO         | A0                 |
| DO         | Not connected      |

### 1.2 Connect the 4-Channel Relay (12V)

| Relay Pin | Connect to Arduino |
| --------- | ------------------ |
| VCC       | 12V                |
| GND       | GND                |
| IN1       | D2 (Water Pump)    |
| IN2       | D3 (Fan)           |
| IN3       | D4 (Reserved)      |
| IN4       | D5 (Reserved)      |

### 1.3 Connect the Water Pump and Fan

- **Water Pump**: Connect to Relay channel 1 (IN1)
- **Fan**: Connect to Relay channel 2 (IN2)

> [!WARNING]
> Be careful about polarity, and use a separate power supply for the water pump and fan.

---

## 💻 Step 2: Install Arduino IDE

### For Windows

1. Go to https://www.arduino.cc/en/software
2. Click **"Windows Win 10 and newer, 64 bits"**
3. Click **"Just Download"** (donation is optional)
4. Open the downloaded file and follow the installation steps
5. Click **"I Agree"** > **"Next"** > **"Install"**
6. Wait for installation to complete, then click **"Close"**

### For macOS

1. Go to https://www.arduino.cc/en/software
2. Click **"macOS"**
3. Click **"Just Download"**
4. Open the downloaded .dmg file
5. Drag Arduino IDE to the Applications folder

### For Linux

1. Go to https://www.arduino.cc/en/software
2. Click **"Linux AppImage 64 bits"**
3. Open Terminal and type:
   ```
   chmod +x arduino-ide_*_Linux_64bit.AppImage
   ./arduino-ide_*_Linux_64bit.AppImage
   ```

---

## 📝 Step 3: Open the Code in Arduino IDE

### 3.1 Open Arduino IDE

Double-click the Arduino IDE icon on the desktop or in the program menu.

### 3.2 Create a New File

1. Click **File** > **New Sketch** (or press `Ctrl + N`)
2. A new window will open

### 3.3 Copy the Code

1. Open the [main.cpp](https://github.com/Scott-Nx/Automatic-Greenhouse-System/blob/main/main.cpp) file in this project
2. Select all (press `Ctrl + A`)
3. Copy (press `Ctrl + C`)
4. Go back to Arduino IDE
5. Delete all existing code, then paste (press `Ctrl + V`)

### 3.4 Save the File

1. Click **File** > **Save** (or press `Ctrl + S`)
2. Name it **"GreenhouseSystem"**
3. Choose the save location and click **Save**

---

## Step 4: Connect Arduino to Computer

### 4.1 Plug in the USB Cable

Use a USB Type-B cable to connect the Arduino Uno to your computer.

### 4.2 Select the Board

1. Click **Tools** > **Board** > **Arduino AVR Boards** > **Arduino Uno**

### 4.3 Select the Port

1. Click **Tools** > **Port**
2. Select the Port with **"Arduino Uno"** appended
   - Windows: Will be `COM3`, `COM4`, or other `COM` ports
   - macOS: Will be `/dev/cu.usbmodem...`
   - Linux: Will be `/dev/ttyACM0` or `/dev/ttyUSB0`

> [!TIP]
> **If you don't see the Port**: Try unplugging and replugging the USB cable, or try a different USB port.

---

## Step 5: Upload the Code to Arduino

### 5.1 Verify the Code

1. Click the **✓ (checkmark)** button in the top left corner, or press `Ctrl + R`
2. Wait until **"Done compiling."** appears at the bottom
3. If there are errors, they will be displayed in orange/red text

### 5.2 Upload the Code

1. Click the **→ (arrow)** button next to the Verify button, or press `Ctrl + U`
2. Wait until **"Done uploading."** appears
3. The LED on the Arduino will blink during upload

---

## Step 6: View the Output (Serial Monitor)

### 6.1 Open Serial Monitor

1. Click **Tools** > **Serial Monitor** or press `Ctrl + Shift + M`
2. A new window will open

### 6.2 Set the Baud Rate

In the bottom right corner of the Serial Monitor, select **"9600 baud"**

### 6.3 Read the Values

You will see messages displaying moisture values and system status, such as:

```
=====================================
ระบบโรงเรือนอัตโนมัติ เริ่มทำงาน
Automatic Greenhouse System Started
=====================================
เริ่มต้นระบบสำเร็จ!

ค่าความชื้น (Moisture): 450 | สถานะ: ปกติ (NORMAL)
  ปั๊มน้ำ: ปิด | พัดลม: ปิด
```

---

## Step 7: Customize Values (Optional)

### Configurable Values

Open the [main.cpp](https://github.com/Scott-Nx/Automatic-Greenhouse-System/blob/main/main.cpp) file and find the following lines:

```cpp
#define MOISTURE_DRY_THRESHOLD    700   // Dry soil threshold
#define MOISTURE_WET_THRESHOLD    300   // Wet soil threshold
#define PUMP_RUN_TIME             5000  // Water pump runtime (milliseconds)
#define FAN_RUN_TIME              10000 // Fan runtime (milliseconds)
```

### How to Adjust Values

| Value                    | Meaning                          | Recommendation                                        |
| ------------------------ | -------------------------------- | ----------------------------------------------------- |
| `MOISTURE_DRY_THRESHOLD` | Value considered as dry soil     | Higher value = soil must be drier to trigger watering |
| `MOISTURE_WET_THRESHOLD` | Value considered as too wet soil | Lower value = soil must be wetter to trigger fan      |
| `PUMP_RUN_TIME`          | Water pump runtime               | 5000 = 5 seconds                                      |
| `FAN_RUN_TIME`           | Fan runtime                      | 10000 = 10 seconds                                    |

### How to Find Optimal Values

1. Open Serial Monitor to view moisture values
2. Test by inserting the Sensor in dry soil, note the value
3. Test by inserting the Sensor in wet soil, note the value
4. Adjust the `THRESHOLD` values as needed

---

## Troubleshooting

### Problem: Cannot Upload

**Symptom**: Shows message "avrdude: stk500_recv(): programmer is not responding"

**Solution**:

1. Check that Board is set to "Arduino Uno"
2. Check that Port is correctly selected
3. Unplug and replug the USB cable
4. Try a different USB cable

### Problem: Moisture Value Not Changing

**Symptom**: Value always shows 0 or 1023

**Solution**:

1. Check the Sensor wiring
2. Verify VCC is connected to 5V, GND is connected to GND
3. Verify AO wire is connected to A0

### Problem: Relay Not Working

**Symptom**: No clicking sound from Relay

**Solution**:

1. Check VCC and GND wiring of the Relay
2. Verify IN1-IN4 wires are connected to correct pins
3. Check that Relay is receiving adequate power

### Problem: Water Pump/Fan Not Working

**Symptom**: Relay clicks but water pump/fan doesn't spin

**Solution**:

1. Check the water pump/fan wiring to Relay
2. Check the power supply for water pump/fan
3. Test the water pump/fan directly with the power supply
