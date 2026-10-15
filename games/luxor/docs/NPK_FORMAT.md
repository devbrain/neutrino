# NPK (NovaPack) File Format Documentation

## Overview
The NPK (NovaPack) file format is a simple archive format used by Luxor 4 to store game assets. The file contains a header, a directory of file entries, and the actual file data.

## File Structure

### Header (29 bytes)
```
Offset  Size  Description
0x00    8     Magic signature: "NovaPack" (ASCII)
0x08    4     Unknown field 1 (always 0x00000000)
0x0C    4     Unknown field 2 (always 0x00000010, possibly version)
0x10    9     Directory marker: "DictStart" (ASCII)
0x19    4     Directory offset (little-endian uint32)
```

### File Directory
The file directory starts immediately after the header (at offset 0x1D) and continues until the directory offset specified in the header. Each entry has the following structure:

```
Offset  Size  Description
0x00    4     Name length (little-endian uint32)
0x04    N     File name (ASCII string, no null terminator)
0x04+N  4     File data offset (little-endian uint32)
0x08+N  4     File size in bytes (little-endian uint32)
```

### File Data
The actual file data is stored sequentially after the directory. Each file's data starts at the offset specified in its directory entry and continues for the specified size.

## Implementation Notes

1. **Byte Order**: All multi-byte integers are stored in little-endian format
2. **File Names**: File names are stored as ASCII strings without null terminators
3. **Directory Structure**: The archive preserves the directory structure in file names (e.g., "UIScreens/Credits.txt")
4. **No Compression**: Files appear to be stored uncompressed
5. **Alignment**: No special alignment requirements observed

## Example Usage

To extract a file from an NPK archive:

1. Read the header to find the directory offset
2. Parse the directory entries to find the desired file
3. Seek to the file's offset
4. Read the specified number of bytes

## Sample Code (Python)

```python
import struct

def extract_file_from_npk(npk_path, target_filename):
    with open(npk_path, 'rb') as f:
        # Read header
        magic = f.read(8)
        if magic != b'NovaPack':
            raise ValueError('Not a valid NPK file')

        f.seek(0x19)  # Skip to directory offset field
        dict_offset = struct.unpack('<I', f.read(4))[0]

        # Parse directory
        while f.tell() < dict_offset:
            name_len = struct.unpack('<I', f.read(4))[0]
            if name_len == 0 or name_len > 1000:
                break

            name = f.read(name_len).decode('ascii')
            offset = struct.unpack('<I', f.read(4))[0]
            size = struct.unpack('<I', f.read(4))[0]

            if name == target_filename:
                # Found the file, extract it
                current_pos = f.tell()
                f.seek(offset)
                data = f.read(size)
                return data

        return None  # File not found
```

## File Types in Luxor 4 data.npk

The data.npk file contains various types of game assets:
- Configuration files (.cfg)
- Text-based data files (.txt)
- UI layout definitions
- Effect definitions (.effect)
- Animation definitions
- Game variables and rules
- Sound definitions
- And more...

Total number of files in data.npk: 116