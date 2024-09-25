# Light Therapy Sequencer

## Overview
This project implements a light therapy sequencer using PWM-controlled LEDs to simulate different effects such as sunrise, sunset, pulsing, and color cycling. The sequencer allows control of various light therapy modes by adjusting parameters such as PWM period, amplitude, and duration for each effect.

## Sequencer Operation Table

| S/N | Sequencer (Effect)       | Function Description                      | PWM Period (ms) | Amplitude (%) | Duration (s) |
|-----|--------------------------|-------------------------------------------|-----------------|---------------|--------------|
| 1   | Static Color              | Set static color on LED                   | 1000            | 100           | 60           |
| 2   | Breathing Effect          | Gradual increase and decrease in intensity| 2000            | 10-100        | 30           |
| 3   | Pulse Effect              | Rapid on-off pulsing of light             | 500             | 0-100         | 45           |
| 4   | Color Cycle               | Cycle through RGB colors                  | 1000            | 100           | 120          |
| 5   | Sunrise Simulation        | Slow intensity increase mimicking sunrise | 5000            | 0-100         | 300          |
| 6   | Sunset Simulation         | Gradual decrease mimicking sunset         | 5000            | 100-0         | 300          |
| 7   | Wave Effect               | Rolling wave-like intensity changes       | 1500            | 20-100        | 60           |
| 8   | Strobe Effect             | Rapid flashing effect                     | 100             | 100           | 15           |
| 9   | Color Gradient Fade       | Smooth transition between colors          | 3000            | 100           | 180          |
| 10  | Blink Mode                | Quick blink on and off                    | 250             | 0-100         | 20           |

## Setup

### Prerequisites
- Microcontroller (e.g., Arduino, ESP32, etc.)
- PWM-compatible LEDs
- Power supply
- Necessary libraries for controlling PWM signals

### Installation
1. Clone this repository:
        git clone https://github.com/your-username/light-therapy-sequencer.git
    
2. Navigate into the project directory:
        cd light-therapy-sequencer
    
3. Upload the code to your microcontroller using the IDE (e.g., Arduino IDE).

### Usage
1. Connect the PWM-capable LEDs to your microcontroller.
2. Upload the desired light therapy effect from the sequencer table by modifying the code.
3. Run the program and observe the LED behavior for each effect.
4. Adjust parameters such as PWM_PERIOD, AMPLITUDE, and DURATION in the code for custom light effects.

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License
[MIT](LICENSE)

## Contact
For inquiries, please reach out to [admin@dillirialibeku.online].
