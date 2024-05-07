# Tile
An ARM CoreTile Express A15x2 operating system.

## Installation
Clone the repository.
```bash
git clone https://github.com/jeremykaragania/tile.git
```
## Usage
```bash
cd tile/kernel
make
qemu-system-arm -machine vexpress-a15 -cpu cortex-a15 -kernel kernel -nographic
```

## License
[MIT](LICENSE)
