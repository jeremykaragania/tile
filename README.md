# Tile
An ARM CoreTile Express A15x2 operating system.

## Installation
```bash
git clone https://github.com/jeremykaragania/tile.git
cd tile/tile
make
```
## Usage
```bash
./mkfs [-b blocks-count] [-i init] device
qemu-system-arm -machine vexpress-a15 -cpu cortex-a15 -drive if=sd,driver=file,filename=device -kernel tile -nographic
```

## License
[MIT](LICENSE)
