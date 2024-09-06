# Tile
An ARM CoreTile Express A15x2 operating system.

## Installation
Clone the repository.
```bash
git clone https://github.com/jeremykaragania/tile.git
```
## Usage
```bash
cd tile
make
./mkfs file 4096
qemu-system-arm -machine vexpress-a15 -cpu cortex-a15 -drive if=sd,driver=file,filename=file -kernel tile -nographic
```

## License
[MIT](LICENSE)
