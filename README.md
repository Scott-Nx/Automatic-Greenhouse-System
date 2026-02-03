# Automatic Greenhouse System Getting Started Guide

🌐 **[ภาษาไทย (Thai Version)](README.th.md)**

## Prerequisites

### Hardware Components

1. **Arduino Uno** (1 board)
2. **Soil Moisture Sensor** (1 unit)
3. **4-Channel Relay** Active-Low type (1 module)
4. **DC Water Pump** with water tubing (1 unit)
5. **DC Fan** (1 unit)
6. **LCD Display 16x2 (I2C)** for status display (1 unit)
7. **Jumper Wires** (multiple)
8. **USB Type-B Cable** for connecting Arduino to computer
9. **Power Supply** for water pump and fan

### Software Requirements

- **Arduino IDE** (for writing and uploading code)
- **LiquidCrystal I2C Library** (library for controlling LCD)

---

## Step 1: Wiring the Circuit

### 1.1 Connect the Soil Moisture Sensor

| Sensor Pin | Connect to Arduino |
| ---------- | ------------------ |
| VCC        | 5V                 |
| GND        | GND                |
| AO         | A0                 |
| DO         | Not connected      |

### 1.2 Connect the 4-Channel Relay (12V)

| Relay Pin (Input) | Connect         | Info                                           |
| ----------------- | --------------- | ---------------------------------------------- |
| VCC               | Arduino 5V (+)  | Use positive from Arduino power                |
| JD-VCC            | Battery 12V (+) | **Do not connect +12V to Arduino**             |
| GND               | Battery 12V (-) | **Arduino GND must be connect only to sensor** |
| IN1               | Arduino D2      | (Reserved)                                     |
| IN2               | Arduino D3      | (Reserved)                                     |
| IN3               | Arduino D4      | Water Pump                                     |
| IN4               | Arduino D5      | Fan                                            |

### 1.3 Connect the LCD Display 16x2 (I2C)

| LCD Pin | Connect to Arduino | Description      |
| ------- | ------------------ | ---------------- |
| VCC     | 5V                 | 5V Power Supply  |
| GND     | GND                | Ground           |
| SDA     | A4                 | Data Line (I2C)  |
| SCL     | A5                 | Clock Line (I2C) |

> [!TIP]
> **Note**: The I2C address of the LCD is typically `0x27` or `0x3F` depending on the model.
> If unsure, you can use the I2C Scanner program to check the address.

### 1.4 Connect the Water Pump and Fan

- **Water Pump**: Connect to Relay channel 3 (IN3)
- **Fan**: Connect to Relay channel 4 (IN4)

**Relay Output Connections (Screw Terminals):**

| Terminal | Description     | Connection                         |
| -------- | --------------- | ---------------------------------- |
| **COM**  | Common          | Connect to **Power Supply (+)**    |
| **NO**   | Normally Open   | Connect to **Device Positive (+)** |
| **NC**   | Normally Closed | **Do Not Connect** (Leave empty)   |

_The Device Negative (-) connects directly to the Power Supply (-)._

> [!WARNING]
> Be careful about polarity, and use a separate power supply for the water pump and fan.

---

## Step 2: Install Arduino IDE

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

## Step 3: Install LiquidCrystal I2C Library

### 3.1 Open Library Manager

1. Open **Arduino IDE**
2. Click **Tools** > **Manage Libraries...** (or press `Ctrl + Shift + I`)
3. The Library Manager window will open

### 3.2 Search and Install Library

1. In the search box, type **"LiquidCrystal I2C"**
2. Look for the library named **"LiquidCrystal I2C"** by **Marco Schwartz**
3. Click on the library and click the **"Install"** button
4. Wait until installation is complete (it will show "INSTALLED")
5. Click **"Close"** to close the window

### 3.3 Verify Installation

1. Click **File** > **Examples**
2. Scroll down and you will see **LiquidCrystal I2C** in the list
3. If you see it, the installation was successful

### 3.4 (Optional) Check LCD I2C Address

1. Download the **I2C Scanner** code from Arduino Examples
2. Click **File** > **Examples** > **Wire** > **i2c_scanner**
3. Upload the code to Arduino (with LCD connected)
4. Open **Serial Monitor** (`Ctrl + Shift + M`)
5. View the I2C address displayed (e.g., `0x27` or `0x3F`)
6. Use this address in your code (if not `0x27`, modify line 33 in main.cpp)

```cpp
constexpr uint8_t LCD_I2C_ADDRESS = 0x27;  // Change to 0x3F if using this address
```

---

## Step 4: Open the Code in Arduino IDE

### 4.1 Open Arduino IDE

Double-click the Arduino IDE icon on the desktop or in the program menu.

### 4.2 Create a New File

1. Click **File** > **New Sketch** (or press `Ctrl + N`)
2. A new window will open

### 4.3 Copy the Code

1. Open the [main.cpp](https://github.com/Scott-Nx/Automatic-Greenhouse-System/blob/main/main.cpp) file in this project
2. Select all (press `Ctrl + A`)
3. Copy (press `Ctrl + C`)
4. Go back to Arduino IDE
5. Delete all existing code, then paste (press `Ctrl + V`)

### 4.4 Save the File

1. Click **File** > **Save** (or press `Ctrl + S`)
2. Name it **"GreenhouseSystem"**
3. Choose the save location and click **Save**

---

## Step 5: Connect Arduino to Computer

### 5.1 Plug in the USB Cable

Use a USB Type-B cable to connect the Arduino Uno to your computer.

### 5.2 Select the Board

1. Click **Tools** > **Board** > **Arduino AVR Boards** > **Arduino Uno**

### 5.3 Select the Port

1. Click **Tools** > **Port**
2. Select the Port with **"Arduino Uno"** appended
   - Windows: Will be `COM3`, `COM4`, or other `COM` ports
   - macOS: Will be `/dev/cu.usbmodem...`
   - Linux: Will be `/dev/ttyACM0` or `/dev/ttyUSB0`

> [!TIP]
> **If you don't see the Port**: Try unplugging and replugging the USB cable, or try a different USB port.

---

## Step 6: Upload the Code to Arduino

### 6.1 Verify the Code

1. Click the **✓ (checkmark)** button in the top left corner, or press `Ctrl + R`
2. Wait until **"Done compiling."** appears at the bottom
3. If there are errors, they will be displayed in orange/red text

### 6.2 Upload the Code

1. Click the **→ (arrow)** button next to the Verify button, or press `Ctrl + U`
2. Wait until **"Done uploading."** appears
3. The LED on the Arduino will blink during upload

---

## Step 7: View the Output

### 7.1 View LCD Display Output

After uploading the code, the LCD will display:

**Startup Screen (first 2 seconds):**

```
🌱 Greenhouse
System v2.1
```

**Status Screen:**

```
🌱M: 65% OK
IDLE       123s
```

**Display Explanation:**

- **Row 1**:
  - Icon (🌱=Normal, 💧=Watering, 🌀=Ventilating, ⚠=Error)
  - `M:XX%` = Moisture level as percentage
  - Status (`OK`=Normal, `DRY`=Dry, `WET`=Too Wet)
- **Row 2**:
  - System state (`IDLE`, `WATERING`, `VENT`, `COOLDOWN`)
  - Elapsed time in current state (seconds)

### 7.2 Open Serial Monitor

1. Click **Tools** > **Serial Monitor** or press `Ctrl + Shift + M`
2. A new window will open

### 7.3 Set the Baud Rate

In the bottom right corner of the Serial Monitor, select **"9600 baud"**

### 7.4 Read the Values

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

## Step 8: Customize Values (Optional)

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

### Problem: LCD Not Displaying

**Symptom**: LCD shows nothing or only shows squares

**Solution**:

1. **Check Wiring**:
   - VCC connected to 5V
   - GND connected to GND
   - SDA connected to A4
   - SCL connected to A5

2. **Check Backlight**:
   - Rotate the contrast potentiometer (blue component on the back of LCD)
   - If you see backlight but no text, rotate in the opposite direction

3. **Check I2C Address**:
   - Use I2C Scanner code to check the address (see Step 3.4)
   - If address is not `0x27`, modify the code accordingly

4. **Check Library**:
   - Verify LiquidCrystal I2C Library is installed
   - Try uninstalling and reinstalling the library

### Problem: LCD Shows Garbled Text

**Symptom**: LCD displays unreadable characters

**Solution**:

1. Rotate the contrast potentiometer on the back of the LCD
2. Check that SDA and SCL wires are connected correctly
3. Ensure no other wires are interfering with the I2C signal

### Problem: I2C Scanner Cannot Find LCD

**Symptom**: I2C Scanner shows "No I2C devices found"

**Solution**:

1. Check SDA (A4) and SCL (A5) wiring
2. Verify LCD is receiving power (VCC, GND)
3. Try using different jumper wires
4. Check if the I2C module on the LCD is not damaged

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
