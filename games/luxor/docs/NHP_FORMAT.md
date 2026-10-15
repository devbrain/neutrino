# NHP (NovaHackPack) File Format Documentation

## Overview
The NHP (NovaHackPack) file format is similar to NPK but with a different structure. The main difference is that the file directory is stored at the end of the file rather than at the beginning, and it includes a file count.

## File Structure

### Header (16 bytes)
```
Offset  Size  Description
0x00    12    Magic signature: "NovaHackPack" (ASCII)
0x0C    4     Dictionary offset (little-endian uint32) - points to where the directory starts
```

### Data Marker (13 bytes)
```
Offset  Size  Description
0x10    13    Data section marker: "LumpDataBegin" (ASCII)
```

### File Data Section
The actual file data starts immediately after "LumpDataBegin" (at offset 0x1D) and continues until the "LumpDataEnd" marker (11 bytes, ASCII: "LumpDataEnd") right before the dictionary offset. Files are stored sequentially in this section.

### Dictionary/Directory Section
Located at the offset specified in the header (typically near the end of the file).

```
Structure:
1. Dictionary marker: "DictionaryBegin" (15 bytes)
2. Entry count: Number of file entries (4 bytes, little-endian uint32)
3. File entries (repeated for each file):
   - Name length (2 bytes, little-endian uint16)
   - File name (variable length ASCII string, no null terminator)
   - File offset (4 bytes, little-endian uint32) - offset from start of NHP file
   - File size (4 bytes, little-endian uint32)
4. End marker: "DictionaryEnd" (13 bytes)
```

## Key Differences from NPK Format

1. **Magic signature**: "NovaHackPack" vs "NovaPack"
2. **Dictionary location**: At the end of file (NHP) vs near the beginning (NPK)
3. **Entry count**: NHP explicitly stores the number of entries, NPK doesn't
4. **Markers**: NHP uses "LumpDataBegin", "DictionaryBegin", and "DictionaryEnd" markers
5. **File paths**: NHP entries typically include "data/" prefix in filenames

## Implementation Notes

1. **Byte Order**: All multi-byte integers are stored in little-endian format
2. **File Names**: File names are stored as ASCII strings without null terminators
3. **No Compression**: Files appear to be stored uncompressed
4. **Directory at End**: The directory being at the end allows for easier appending of files

## Example Usage

To extract a file from an NHP archive:

1. Read the header to find the dictionary offset
2. Seek to the dictionary offset
3. Skip the "DictionaryBegin" marker
4. Read the entry count
5. Parse the directory entries to find the desired file
6. Seek to the file's offset
7. Read the specified number of bytes

## Sample Code (Python)

```python
import struct

def extract_file_from_nhp(nhp_path, target_filename):
    with open(nhp_path, 'rb') as f:
        # Read header
        magic = f.read(12)
        if magic != b'NovaHackPack':
            raise ValueError('Not a valid NHP file')

        dict_offset = struct.unpack('<I', f.read(4))[0]

        # Seek to dictionary
        f.seek(dict_offset)
        dict_marker = f.read(15)  # Skip "DictionaryBegin"

        if dict_marker != b'DictionaryBegin':
            raise ValueError('Invalid dictionary marker')

        # Read number of entries
        num_entries = struct.unpack('<I', f.read(4))[0]

        # Parse directory entries
        for i in range(num_entries):
            name_len = struct.unpack('<H', f.read(2))[0]
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

def list_files_in_nhp(nhp_path):
    files = []
    with open(nhp_path, 'rb') as f:
        # Read header
        f.seek(12)  # Skip magic
        dict_offset = struct.unpack('<I', f.read(4))[0]

        # Seek to dictionary
        f.seek(dict_offset + 15)  # Skip to after "DictionaryBegin"
        num_entries = struct.unpack('<I', f.read(4))[0]

        # Read all entries
        for i in range(num_entries):
            name_len = struct.unpack('<H', f.read(2))[0]
            name = f.read(name_len).decode('ascii', errors='replace')
            offset = struct.unpack('<I', f.read(4))[0]
            size = struct.unpack('<I', f.read(4))[0]
            files.append({'name': name, 'offset': offset, 'size': size})

    return files
```

## File Structure in data.nhp

The data.nhp file from Luxor 4 contains:
- Total entries: 3,588 files
- File types include: .txt configuration files, .effect files, and other game assets
- All file paths are prefixed with "data/"
- Dictionary starts at offset 0x023809c6 (near the 37MB mark)
- Total file size: 37,400,270 bytes (approximately 35.7 MB)