# Universal transmitter firmware

`MycoLogger-Transmitter-Universal.hex` is the standard image used by the local
provisioner. It reserves flash page 15 (`0x08007800`) for the node identity and
boots with Node ID 0 when that page does not contain a valid configuration.

Node 0 never transmits. Four repeating LED flashes indicate that a board still
needs to be provisioned. The provisioner flashes this common application image,
then writes the board-specific 32-byte configuration page separately.

Rebuild the image from the CubeIDE transmitter project before publishing a new
firmware version. The linked application must remain below `0x08007800`.
