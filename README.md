# RF Modulator/Signal Generator

This repository contains the source code and documentation for an RF Modulator Baseband Signal generator. The modulator is capable of transmitting and receiving data using various RF modulation techniques, including QPSK, 8PSK, 16QAM, and BPSK (OPSK and other modulation methods have been implemented but not tested yet).

## Project Overview

The RF Modulator project aims to provide a practical understanding of RF modulation techniques and their implementation. By leveraging the power of Software Defined Radio (SDR), the project explores the design and construction of an various types of signal Modulation from scratch in C.

## Key Components of the RF Modulator project
Modulator Design: _(Baseband Signal Generator)_
- Texas Instruments Tiva C TM4C123GH6PM evaluation board
- MCP4822 Dual channel DAC
- MAC660 Voltage Converter
- TLC072 Dual Channel Op-Amp
Testing: _(After Baseband Signal is generated to RF Frequency 900 MHz)_
- Quorvo SBB5089Z InGaP HBT Active Bias MMIC Amplifier.
- 20 dB DC Block (RF) 
- SMA Male to Male Connector bullets
- UTiFlex UFA 210B 1m Cabling
- 890 to 915 MHz 5 Dipole Yagi-Uda Antenna

## Repository Structure
The repository is organized as follows:

- `src/`: Contains the source code files for the Baseband Signal Modulator.
- `docs/`: Includes project documentation/datasheets on equipment used.
- `images/`: Holds images and visual assets related to the project.
- `LICENSE`: Specifies the licensing terms for the project.
- `README.md`: Provides an overview and instructions for the project.

## Design Constrains
Modulations on the baseband generator can be generated till 1Hz to 50kHz which will be the span of the RF generated signal.  
## Getting Started

To get started with the RF Modulator project, follow these steps:

1. Clone the repository to your local machine using the following command:

```bash
git clone https://github.com/n43ee7/RF_Modulator.git
```

2. Navigate to the project directory:

```bash
cd RF_Modulator
```

3. Explore the `src/` directory to access the source code files. NOTE: In order to compile this you will need to acquire the tm4c123gh6pm.h Header. This can be downloaded at [here](https://github.com/yuvadm/tiva-c/blob/master/inc/tm4c123gh6pm.h)

4. Refer to the documentation in the `docs/` directory for additional information on the project, including setup instructions, usage guidelines, and technical details.

## Contribution Guidelines

Contributions to the RF Modulator project are welcome! If you would like to contribute, please follow these guidelines:

1. Fork the repository to your GitHub account.

2. Create a new branch for your feature or bug fix:

```bash
git checkout -b my-feature
```

3. Make your modifications and commit them with descriptive commit messages:

```bash
git commit -m "Add new feature: XYZ"
```

4. Push your changes to your forked repository:

```bash
git push origin my-feature
```

5. Open a pull request on the original repository, explaining your changes and their purpose.

Please ensure that your contributions align with the project's coding style, guidelines, and licensing terms.

## License

The RF Modulator project is licensed under the [MIT License](LICENSE). Any usage is the user's sole responsibility. This repository may not be used for any accademic work. Accademic integrity policies may apply and owner does accept any liability on the misuse of the contents of repository. This repository is solely for documentation purpose of the owner's research. Please Look into [FCC RF Low Power emission and transmission Guidelines]() before implementing any radio application
