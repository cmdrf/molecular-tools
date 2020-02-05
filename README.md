# molecular-tools
Tools that operate on assets used by the molecular engine

## Tools
Currently only contains `etccompress`

### etccompress
An ETC2 texture compression command line tool based on
(modified) code by Jean-Philippe Andre and Rich Geldreich. Also uses the popular stb_image
library by Sean Barrett. Many thanks to all of you!

#### Usage
```
etccompress <input file> <output file>
```
Reads all image formats supported by stb_image, which include PNG, JPEG, TGA and many others.
Can output .dds or .ktx files.

## Building

This is designed to be included into a bigger CMake project via `add_subdirectory(molecular-tools)`.
The outer project also needs to include [molecular-util](https://github.com/cmdrf/molecular-util).
See the top-level [molecular](https://github.com/cmdrf/molecular) project for an example.


## Licensing
MIT License. See `LICENSE` file for details. Important: The code within the `3rdparty` directory
is individually licensed. See comments in the file headers for details.
