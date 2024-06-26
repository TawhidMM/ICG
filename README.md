
# C compiler

this C compiler can compile a subset of C language and generate the target code in 8086 assembly. The compiler uses flex as lexical analyzer and yacc as syntax analyzer.

[ICG Assignment Spec](https://github.com/TawhidMM/ICG/blob/master/sample_inputs/CSE_310_July_2023_ICG_Spec.pdf)


## Requirements

- **Flex :** a tool for tokenizing the input
- **yacc :** a tool for generating parser
- **EMU8086 :** emulator to run the target 8086 assembly


## Installation
#### 1. update wsl/linux software packages
```bash
  sudo apt update
```
#### 2. install flex

```bash
  sudo apt install flex
```
#### 3. install yacc

```bash
  sudo apt install byacc
```
#### 4. download the EMU8086
[EMU8086](https://emu8086-microprocessor-emulator.en.softonic.com/download)

[EMU8086 licence key](https://deeprajbhujel.blogspot.com/2014/01/emu8086-license-key.html)




## How to run

#### 1. put the c code **input.c**

#### 2. now run the command to generate the assembly code

```bash
  ./compileInput.sh
```

#### 3. copy and paste the assembly in  [EMU8086](https://emu8086-microprocessor-emulator.en.softonic.com/download) and emulate

#### 4. run **input.c** directly  using g++ 

```bash
  ./executeInput.sh
```
    
