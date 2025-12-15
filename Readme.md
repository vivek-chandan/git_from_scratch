# MyGit

A lightweight version control system (VCS) implemented in C++, inspired by Git. MyGit supports essential VCS functions, including repository initialization, file staging, committing, and viewing commit history. Objects are stored in a `.git` directory structure using SHA-1 hashing for integrity.

## Features

- **Initialize Repository**: Sets up a `.git` directory to track files.
- **Add Files**: Stages files for the next commit.
- **Commit Changes**: Saves staged files with a commit message.
- **Write Tree**: Creates a tree object representing the directory structure.
- **Log**: Displays the commit history.
- **View Objects**: Inspects objects' types and contents.

## Requirements

- **C++17 or higher**
- **OpenSSL** (for SHA-1 hashing)
- **Zlib** (for data compression)

## Installation

1. Install required libraries: OpenSSL, Zlib.
2. Clone the repository and navigate to the directory.
3. Use the Makefile to compile the project.

## Usage

1. **Initialize**: Create a new `.git` directory.
2. **Add**: Stage files or directories for the next commit.
3. **Commit**: Save the staged changes with a commit message.
4. **Log**: View commit history in chronological order.
5. **Write Tree**: Generate a tree object for a directory.
6. **Inspect Objects**: View details of stored objects (e.g., blobs, trees).

## Example Commands

- `./mygit init` – Initializes a new repository.
- `./mygit add <file>` – Stages a file for the next commit.
- `./mygit commit -m "<message>"` – Commits staged changes.
- `./mygit log` – Displays commit history.

## Project Structure

- **src/** – Contains the main source files (`.cpp`, `.hpp`) for functionality.
- **.git/** – Stores repository data, including objects and references.
- **Makefile** – Builds and runs the project.

## Notes

- Each command must be run from the root of the initialized repository.
- Logs and object inspection require an initialized repository and committed objects.

## License

This project is open-source and available for modification and distribution.
