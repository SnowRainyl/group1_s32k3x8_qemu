- # QEMU S32K3X8 Emulation with FreeRTOS Demo

  ## Description
  This project provides an emulation environment. It uses QEMU. The project focuses on the NXP S32K3X8 series microcontroller. Specifically, it targets the S32K358EVB board. The emulation includes an LPUART peripheral. A FreeRTOS operating system runs on this emulated hardware. The FreeRTOS application demonstrates multitasking. It also shows serial communication via the LPUART. This project helps developers. They can develop and test embedded software. They do not need physical hardware for this. This is useful for early-stage verification and debugging.

  **Key Features:**
  * The project includes a QEMU board model. This model is for the S32K358EVB.
  * It features LPUART peripheral emulation.
  * A FreeRTOS port for the Cortex-M7 core is available.
  * The project contains an example FreeRTOS application. This application shows multiple tasks and UART communication.
  * It provides startup code for the S32K3 microcontroller. System initialization is also included.

  ## Installation and Setup
  These steps will guide you through setting up the project. You will also build QEMU.

  1.  **Prepare Project Directory and Clone Repository**
      * First, choose or create a new directory. This directory will hold your cloned project. Let's call it `your_project_path`.
          ```bash
          mkdir your_project_path
          cd your_project_path
          ```
      * Next, clone the Git repository. Use the specified branch. Include its submodules.
          ```bash
          git clone --recurse-submodules [https://baltig.polito.it/eos2024/group1.git](https://baltig.polito.it/eos2024/group1.git)
          ```

  2.  **Install Dependencies**
      * You need to install various build tools and libraries. Open your terminal. Run the following command:
          ```bash
          sudo apt update
          sudo apt install build-essential zlib1g-dev libglib2.0-dev \
              libfdt-dev libpixman-1-dev ninja-build python3-sphinx \
              python3-sphinx-rtd-theme pkg-config libgtk-3-dev \
              libvte-2.91-dev libaio-dev libbluetooth-dev \
              libbrlapi-dev libbz2-dev libcap-dev libcap-ng-dev \
              libcurl4-gnutls-dev python3-venv gcc-arm-none-eabi cmake git
          ```

  3.  **Build and Install QEMU**
      
      * Navigate into the QEMU directory. This directory is inside your cloned project.
          ```bash
          cd group1/qemu
          ```
          *(The `group1` directory is created by the `git clone` command above.)*
      * Configure the build. Target the ARM softmmu.
          ```bash
          ./configure --target-list=arm-softmmu
          ```
      * Compile QEMU. Then, install QEMU.
          ```bash
          make
          sudo make install
          ```
      
  4.  **Verify QEMU Installation (Optional)**
      * You can check the available ARM machine types. This helps confirm your board is listed.
          ```bash
          qemu-system-arm -M ?
          ```
          *(You should see `s32k3x8evb` or a similar name in the output list.)*

  ## Usage
  Follow these steps to build and run the firmware on the emulated board.

  1.  **Build and Run Firmware**
      * Navigate to the firmware directory. This is likely `Demo/Firmware` within your cloned `group1` repository.
          ```bash
          cd ../Demo/Firmware 
          ```
          *(If you are in `group1/qemu`, use `cd ../../Demo/Firmware`. If in `group1`, use `cd Demo/Firmware`.)*
      * Compile the firmware.
          ```bash
          make
          ```
      * Run the compiled firmware using QEMU.
          ```bash
          qemu-system-arm -M s32k3x8evb -nographic -kernel firmware.elf
          ```

  **Expected Output Example:**
  The console should display messages similar to these:

![image-20250525202512245](/home/yuqi/.config/Typora/typora-user-images/image-20250525202512245.png)

## Project Structure
This is the main directory structure of the cloned repository (`group1/`):

├── Demo/                   # Contains demo code and FreeRTOS files
│   ├── Firmware/           # Firmware source code and build files
│   ├── FreeRTOS/           # FreeRTOS kernel source
│   └── Headers/            # Shared project header files
├── materials/              # Contains related reference materials and tools
│   ├── fmstr_uart_s32k358.zip  # FreeMASTER UART S32K358 related files
│   └── split_rm.zip        # Other material archive
├── qemu/                   # QEMU source code (includes board/peripheral modifications)
├── README.md               # This project description file
└── work.docx               # Project-related work document


**Key Directory Explanations:**
* **`Demo/`**: This directory holds the demonstration application code. It runs on the emulated hardware.
    * `Demo/Firmware/`: Contains core firmware logic. This includes `main.c`, startup code, linker scripts, and the `Makefile` for firmware compilation.
    * `Demo/FreeRTOS/`: May contain FreeRTOS kernel source files. It could also be a Git submodule.
    * `Demo/Headers/`: Stores common header files for the firmware project.
* **`materials/`**: This directory includes supplementary materials. These could be documents or third-party tools related to the project.
* **`qemu/`**: This is the QEMU source code directory. This QEMU version is already modified. It includes emulation code for the S32K3X8EVB board and LPUART peripheral.
* **`README.md`**: This file. It provides an overview, setup guide, and usage instructions for the project.
* **`work.docx`**: A Word document. It might contain project reports or design notes.

*(The `qemu/` directory is very large. The list above highlights parts relevant to this project's context. Refer to the "Installation and Setup" section for build and integration details.)*

## Support
Tell people where they can find help. This can be an issue tracker. It could also be a chat room or an email address.
* Please submit issues via the project's GitLab Issues page.
* (List any other support channels you offer.)

## Roadmap
If you plan future releases, list them here. This gives people an idea of where the project is going.
* Example:
    * Emulate more S32K3X8 peripherals (SPI, I2C, CAN).
    * Integrate more complex demo applications.

## Contributing
Contributions are welcome. Clearly state your requirements for accepting contributions.
1.  Fork this repository.
2.  Create a new branch for your feature or bugfix (e.g., `git checkout -b feature/YourFeature`).
3.  Make your changes. Commit them with a clear message (e.g., `git commit -m 'Add some feature'`).
4.  Push your branch to your forked repository (e.g., `git push origin feature/YourFeature`).
5.  Create a new Merge Request.

**Development Setup:**
Document how contributors can set up their local development environment. Mention any scripts to run or environment variables to set. These instructions are helpful for others and your future self. You can also include commands for linting code or running tests.

## Authors and Acknowledgment
Authors:

* s338247 - Rebecca Burico(s338247@studenti.polito.it)
* s336721 - Yuqi Li(s336721@studenti.polito.it)

Acknowledgment:

* Stefano Di Carlo(stefano.dicarlo@polito.it): Professor 
* Carpegna Alessio(alessio.carpegna@polito.it): Co-lectures
* Eftekhari Moghadam Vahid(vahid.eftekhari@polito.it): Co-lectures
* Magliano Enrico(enrico.magliano@polito.it): Co-lectures

## License

This project, including the source code and any accompanying documents (such as presentations, reports, etc.), is distributed under the **Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0) License**.

* **Attribution (BY):** You must give appropriate credit, provide a link to the license, and indicate if changes were made. You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
* **NonCommercial (NC):** You may not use the material for commercial purposes.

