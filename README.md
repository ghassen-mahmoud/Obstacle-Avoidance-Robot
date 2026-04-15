# 🤖 Obstacle Avoidance Robot (MSP430)

## 📌 Description

This project implements an autonomous obstacle avoidance robot using an MSP430 microcontroller. The robot is capable of detecting obstacles using infrared sensors and navigating automatically by controlling two motors. It also uses a light sensor (LDR) to detect ambient light conditions.

---

## 🛠️ Hardware Components

* MSP430 Microcontroller
* 2 × Infrared Sensors (Obstacle Detection)
* 1 × LDR (Light Dependent Resistor)
* 2 × DC Motors
* Motor Driver (L298N or similar)
* Power Supply

---

## ⚙️ Features

* Autonomous obstacle detection and avoidance
* Real-time navigation
* Light intensity detection using LDR
* Control of two motors for movement (forward, left, right, stop)

---

## 🔌 System Overview

The robot continuously reads data from:

* Infrared sensors → detect obstacles
* LDR sensor → measure light intensity

Based on sensor inputs, the MSP430 decides:

* Move forward (no obstacle)
* Turn left/right (obstacle detected)
* Stop (critical condition)

---

## 🧠 Working Principle

1. Read IR sensors
2. Detect obstacle presence
3. Read LDR value
4. Decide movement:

   * No obstacle → move forward
   * Obstacle on left → turn right
   * Obstacle on right → turn left
   * Obstacles on both sides → stop or reverse

---

## 💻 Software

* Language: Embedded C
* IDE: Code Composer Studio
* Microcontroller: MSP430

---

## 📂 Project Structure

* `main.c` → Main control logic
* `ADC.c / ADC.h` → Sensor reading (LDR)
* Configuration files → MSP430 setup
## 📸 Robot Demo

![Robot](robot1.mp4)

