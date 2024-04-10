# Thesis Project: CPU Scheduling Simulator

## Overview
This project aims to enhance the CPU scheduling simulator presented at lesson. The modifications include adding support for multiple CPUs, implementing preemptive Shortest Job First (SJF) scheduling with quantum prediction, and integrating stochastic event generation using inverse transform sampling.

## Features
### Multi-CPU System
The simulator allows for concurrent processing of tasks across different processors.

### Shortest Job First (SJF)
The scheduling algorithm has been changed from RR to SJF, prioritizing tasks based on their burst time and executing shortest job first among the available processes.

### Quantum Prediction
Since it is not possible to predict which process will occupy the resource for the shortest time, it is necessary to calculate the next quantum for each process based on historical data. The formula is q(t+1) = a * q_current + (1-a) * q(t), where 'a' is a configurable parameter.

### Stochastic Event Generation
The events for each process are generated simulating an inverse transform sampling. This technique allows for the random generation of events based on their probability distributon. After sampling the input events, they are normalized to represent a probability. The cumulative function is then applied to this distribution, from which we simulate inverse transform sampling by generating a random number between 0 and 1, and then mapping it through the inverse of the CDF to obtain samples.

## Completed Tasks
- [x] multi-CPU system
- [x] preemptive SJF algorithm
- [x] stochastic event generation simulating inverse transform sampling
