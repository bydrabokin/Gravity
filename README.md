# Orbital

A real-time orbital mechanics simulator written in C using SDL2.

## Overview

This project is a physics simulation that models gravitational interactions and orbital motion using Newtonian mechanics.

The project includes:
- Planetary motion simulation
- Gravity calculations
- Orbital parameters calculation
- Real-time visualization
- Hohmann transfer calculations

## Features

- 2 Body and 1 body simulations
- Newtonian gravity simulation
- Velocity and position integration
- Orbital energy calculation
- Semi-major axis calculation
- Eccentricity calculation
- Interactive controls
- SDL2 graphics, audio, fonts, and images

## Screenshots

(Add screenshots here)

## Installation

### Windows

1. Download the Windows release.
2. Extract all files.
3. Run:

```
gravity.exe
```

Make sure the required DLL files are in the same directory.

### Linux

Install dependencies:

```bash
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libsdl2-mixer-dev
```

Run:

```bash
./gravity
```

## Building from source

Clone the repository:

```bash
git clone https://github.com/username/orbital.git
cd orbital
```

Compile:

```bash
gcc src/main.c -o gravity \
-lSDL2 \
-lSDL2_ttf \
-lSDL2_image \
-lSDL2_mixer \
-lm
```

## Controls

| Key | Action |
|---|---|
| Space | Start simulation |
| R | Reset |
| H | Hide UI |

## Physics

The simulation uses Newton's law of universal gravitation:

\[
F = G\frac{m_1m_2}{r^2}
\]

Orbital parameters are calculated using:

- Specific orbital energy
- Angular momentum
- Eccentricity vector
- Semi-major axis equations

More details:
`extraInfo/basicGravitationPhysicsExplanation.md`

## Dependencies

- C
- SDL2
- SDL2_image
- SDL2_ttf
- SDL2_mixer
