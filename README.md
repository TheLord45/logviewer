# Logviewer

This is a small tool to analyze logfiles. It was primarily invented to
analyze log files for my `tpanel` project. But you can use it for any kind 
of log file as long as every line of it has the same format. There are 
settings available to set all possible aspects necessary to analyze a file.

The program is able to read plain files or files compressed with `gzip` 
(extension `*.gz`).

**Here are some features**:

* Graphical GUI
* Support for reading compressed files
* Colored presentation of log files
* Validation of constructors and destructors (as far as this is part of the logfile)
* Search for all occurrences of exceptions
* Free search for any string
* Number of columns can be set
* Column titles can be set individual
* Column delimiter can be set
* JSON formatted files can be parsed

The tool has a GUI which make the usage very easy.

# Where to download

The program is available on GitHub. There you'll find the source. Look at
[logviewer][https://github.com/TheLord45/logviewer]

# How to compile

The program is written in C++ and uses the QT framework in version 6. So
this is a precondition to compile it.

## Prerequisites

- Qt 6
  - This may be already part of your distribution. But you may need to install
  the developer packages. It depends on your distribution.
  - In doubt you can get the framework from [Qt](https://www.qt.io/download-qt-installer) 
  directly (recomended!).
- C++ compiler
  - Either CLang or Gnu C++. At least the compiler must support C++ 17 standard!

| **Hint**: If you're brave you can even try to compile it on Windows. Qt is 
available also for this so called OS. But be aware that the code does not
support proprietary systems!

## How to compile

1. Check out the repository:
   - `git clone https://github.com/TheLord45/logviewer.git`
2. Go to the directory `logviewer` and type the following commands:
   ```
   mkdir build
   cd build
   cmake ..
   make -j
   sudo make install
   ```
This will produce a binary called `logviewer` and installs it in `/usr/local/bin`.
